#include <ast/types/ReferenceType.hpp>
#include "SignatureComparer.hpp"
#include "TypeHelper.hpp"
#include "TypeCompareUtil.hpp"
#include <algorithm>

using namespace gulc;

SignatureComparer::CompareResult SignatureComparer::compareFunctions(const FunctionDecl* left,
                                                                     const FunctionDecl* right,
                                                                     bool checkSimilar) {
    if (left->isStatic() != right->isStatic() || left->isMutable() != right->isMutable() ||
        left->identifier().name() != right->identifier().name()) {
        return CompareResult::Different;
    }

    // If `checkSimilar` is enabled then we will still check similarity even if the functions have different numbers of
    // parameters
    if (left->parameters().size() != right->parameters().size() && !checkSimilar) {
        // They are only `Different` if `checkSimilar` is false
        return CompareResult::Different;
    }

    std::size_t parameterSize = std::max(left->parameters().size(), right->parameters().size());

    for (std::size_t i = 0; i < parameterSize; ++i) {
        if (i >= left->parameters().size()) {
            if (!checkSimilar) {
                return CompareResult::Different;
            }

            // If the right parameter is optional then the functions are similar
            if (right->parameters()[i]->defaultValue != nullptr) {
                return CompareResult::Similar;
            } else {
                return CompareResult::Different;
            }
        } else if (i >= right->parameters().size()) {
            if (!checkSimilar) {
                return CompareResult::Different;
            }

            // If the left parameter is optional then the functions are similar
            if (left->parameters()[i]->defaultValue != nullptr) {
                return CompareResult::Similar;
            } else {
                return CompareResult::Different;
            }
        } else {
            // If one side is optional and the other isn't then they are different BUT they could still be similar
            // so we only return `Different` if we're not checking for similarities
            if (!checkSimilar && (left->parameters()[i]->defaultValue == nullptr) !=
                                 (right->parameters()[i]->defaultValue == nullptr)) {
                return CompareResult::Different;
            }

            auto leftParam = left->parameters()[i];
            auto rightParam = right->parameters()[i];

            // If the labels aren't the same then the functions aren't the same. ("_" must be for both for that to
            // affect it)
            if (leftParam->argumentLabel().name() != rightParam->argumentLabel().name()) {
                return CompareResult::Different;
            }

            Type* leftType = leftParam->type;
            Type* rightType = rightParam->type;

            // NOTE: `ref int` == `int` are the same BUT `ref mut int` != `int` and `ref mut int` != `ref int`
            //       This is because `func ex(_ param: ref int);` is callable as `ex(12)`
            if (leftType->qualifier() != Type::Qualifier::Mut && llvm::isa<ReferenceType>(leftType)) {
                leftType = llvm::dyn_cast<ReferenceType>(leftType)->nestedType;
            }

            if (rightType->qualifier() != Type::Qualifier::Mut && llvm::isa<ReferenceType>(rightType)) {
                rightType = llvm::dyn_cast<ReferenceType>(rightType)->nestedType;
            }

            TypeCompareUtil typeCompareUtil;
            if (!typeCompareUtil.compareAreSame(leftType, rightType)) {
                return CompareResult::Different;
            }
        }
    }

    // If we make it to this point the functions are exactly the same
    return CompareResult::Exact;
}

SignatureComparer::ArgMatchResult SignatureComparer::compareArgumentsToParameters(std::vector<ParameterDecl*> const& parameters,
                                                                                  std::vector<LabeledArgumentExpr*> const& arguments) {
    return compareArgumentsToParameters(parameters, arguments, {}, {});
}

SignatureComparer::ArgMatchResult SignatureComparer::compareArgumentsToParameters(std::vector<ParameterDecl*> const& parameters,
                                                                                  std::vector<LabeledArgumentExpr*> const& arguments,
                                                                                  std::vector<TemplateParameterDecl*> const& templateParameters,
                                                                                  std::vector<Expr*> const& templateArguments) {
    // If there are more arguments than parameters then we can immediately fail
    if (arguments.size() > parameters.size()) {
        return ArgMatchResult::Fail;
    }

    ArgMatchResult currentResult = ArgMatchResult::Match;

    TypeCompareUtil typeCompareUtil(&templateParameters, &templateArguments);

    for (std::size_t i = 0; i < parameters.size(); ++i) {
        if (i >= arguments.size()) {
            // If there are more parameters than arguments then the parameter immediately after the last argument MUST
            // be optional
            if (parameters[i]->defaultValue != nullptr) {
                return currentResult;
            } else {
                return ArgMatchResult::Fail;
            }
        } else {
            // If the argument label doesn't match the one provided by the parameter then we fail...
            // NOTE: For arguments without labels we implicitly add `_` to the argument expression as the label
            if (arguments[i]->label().name() != parameters[i]->argumentLabel().name()) {
                return ArgMatchResult::Fail;
            }

            // Compare the type of the argument to the type of the parameter...
            if (!typeCompareUtil.compareAreSame(arguments[i]->valueType, parameters[i]->type)) {
                // TODO: Check if the argument type can be casted to the parameter type...
                return ArgMatchResult::Fail;
            }
        }
    }

    return currentResult;
}
