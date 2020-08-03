#include <iostream>
#include <ast/decls/CallOperatorDecl.hpp>
#include <ast/decls/OperatorDecl.hpp>
#include <ast/decls/SubscriptOperatorDecl.hpp>
#include <ast/decls/TemplateFunctionDecl.hpp>
#include "InheritUtil.hpp"
#include "SignatureComparer.hpp"

bool gulc::InheritUtil::overridesOrShadows(gulc::Decl* checkOverrides, gulc::Decl* checkAgainst) {
    Decl::Kind overrideKind = checkOverrides->getDeclKind();
    Decl::Kind againstKind = checkAgainst->getDeclKind();

    // The operators must be the same kind and must be one of the below to be checked in a more complex way. If the Decl
    // isn't one of the kinds we've checked against here then we just check if the names are the same for a shadow
    if (overrideKind != againstKind || (overrideKind != Decl::Kind::CallOperator &&
            overrideKind != Decl::Kind::Function && overrideKind != Decl::Kind::Operator &&
            overrideKind != Decl::Kind::SubscriptOperator && overrideKind != Decl::Kind::TemplateFunction)) {
        return checkOverrides->identifier().name() == checkAgainst->identifier().name();
    }

    switch (overrideKind) {
        case Decl::Kind::CallOperator: {
            auto overridesOperator = llvm::dyn_cast<CallOperatorDecl>(checkOverrides);
            auto againstOperator = llvm::dyn_cast<CallOperatorDecl>(checkAgainst);

            // For the `call` operator we just need to check if the parameters match exactly and check `mut`
            // NOTE: Call operators CANNOT be static so we don't need to check that.
            if (overridesOperator->isMutable() != againstOperator->isMutable()) {
                return false;
            }

            return SignatureComparer::compareParameters(overridesOperator->parameters(),
                                                        againstOperator->parameters(), false) ==
                   SignatureComparer::CompareResult::Exact;
        }
        case Decl::Kind::Function: {
            auto overridesFunction = llvm::dyn_cast<FunctionDecl>(checkOverrides);
            auto againstFunction = llvm::dyn_cast<FunctionDecl>(checkAgainst);

            // For functions name must match exactly then we will use the preexisting `compareFunctions` function to
            // check their similarity
            if (overridesFunction->identifier().name() != againstFunction->identifier().name()) {
                return false;
            }

            return SignatureComparer::compareFunctions(overridesFunction, againstFunction, false) ==
                   SignatureComparer::CompareResult::Exact;
        }
        case Decl::Kind::Operator: {
            auto overridesOperator = llvm::dyn_cast<OperatorDecl>(checkOverrides);
            auto againstOperator = llvm::dyn_cast<OperatorDecl>(checkAgainst);

            // For operators the operators must be the same then we will use the preexisting `compareFunctions` to
            // check their similarity
            if (overridesOperator->operatorType() != againstOperator->operatorType() ||
                    overridesOperator->operatorIdentifier().name() != againstOperator->operatorIdentifier().name()) {
                return false;
            }

            return SignatureComparer::compareFunctions(overridesOperator, againstOperator, false);
        }
        case Decl::Kind::SubscriptOperator: {
            auto overridesSubscript = llvm::dyn_cast<SubscriptOperatorDecl>(checkOverrides);
            auto againstSubscript = llvm::dyn_cast<SubscriptOperatorDecl>(checkOverrides);

            // For subscripts we need to check if `mut` matches, `static` matches, and parameters match exactly
            if (overridesSubscript->isStatic() != againstSubscript->isStatic() ||
                    overridesSubscript->isMutable() != againstSubscript->isMutable()) {
                return false;
            }

            return SignatureComparer::compareParameters(overridesSubscript->parameters(),
                                                        againstSubscript->parameters(), false);
        }
        case Decl::Kind::TemplateFunction: {
            auto overridesTemplateFunction = llvm::dyn_cast<TemplateFunctionDecl>(checkOverrides);
            auto againstTemplateFunction = llvm::dyn_cast<TemplateFunctionDecl>(checkAgainst);

            // For template functions their names must match, `mut` matches, `static` matches, parameters match, and
            // template parameters must match. If any of these are different then they aren't shadows
            if (overridesTemplateFunction->identifier().name() != againstTemplateFunction->identifier().name()) {
                return false;
            }

            return SignatureComparer::compareTemplateFunctions(overridesTemplateFunction, againstTemplateFunction,
                                                               false);
        }
        default:
            // This should NEVER be triggered.
            std::cerr << "internal error from unknown decl in `InheritUtil::overridesOrShadows`!" << std::endl;
            std::exit(1);
            return false;
    }
}
