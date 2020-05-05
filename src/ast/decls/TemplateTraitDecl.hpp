#ifndef GULC_TEMPLATETRAITDECL_HPP
#define GULC_TEMPLATETRAITDECL_HPP

#include <ast/decls/TraitDecl.hpp>
#include <llvm/Support/Casting.h>
#include <iostream>
#include <utilities/ConstExprHelper.hpp>
#include <ast/exprs/TypeExpr.hpp>
#include "TemplateParameterDecl.hpp"
#include "TemplateTraitInstDecl.hpp"

namespace gulc {
    class TemplateTraitDecl : public TraitDecl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::TemplateTrait; }

        std::vector<TemplateParameterDecl*>& templateParameters() { return _templateParameters; }
        std::vector<TemplateParameterDecl*> const& templateParameters() const { return _templateParameters; }
        std::vector<TemplateTraitInstDecl*>& templateInstantiations() { return _templateInstantiations; }
        std::vector<TemplateTraitInstDecl*> const& templateInstantiations() const { return _templateInstantiations; }

        TemplateTraitDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                          bool isConstExpr, Identifier identifier, DeclModifiers declModifiers,
                          TextPosition startPosition, TextPosition endPosition,
                          std::vector<Type*> inheritedTypes, std::vector<Cont*> contracts,
                          std::vector<Decl*> ownedMembers, std::vector<TemplateParameterDecl*> templateParameters)
                : TemplateTraitDecl(sourceFileID, std::move(attributes), visibility,
                                    isConstExpr, std::move(identifier), declModifiers, startPosition, endPosition,
                                    std::move(inheritedTypes), std::move(contracts), std::move(ownedMembers),
                                    std::move(templateParameters), {}) {}

        bool getInstantiation(std::vector<Expr*> const& templateArguments, TemplateTraitInstDecl** result) {
            // NOTE: We don't account for default parameters here. `templateArguments` MUST have the default values.
            if (templateArguments.size() != _templateParameters.size()) {
                std::cerr << "INTERNAL ERROR[`TemplateTraitDecl::getInstantiation`]: `templateArguments.size()` MUST BE equal to `_templateParameters.size()`!" << std::endl;
                std::exit(1);
            }

            // We don't check if the provided template arguments are valid UNLESS the isn't an existing match
            // the reason for this is if the template arguments are invalid that is an error and it is okay to take
            // slightly longer on failure, it is not okay to take longer on successes (especially when the check is
            // just a waste of time if it already exists)
            for (TemplateTraitInstDecl* templateTraitInst : _templateInstantiations) {
                // NOTE: We don't deal with "isExact" here, there are only matches and non-matches.
                bool isMatch = true;

                for (std::size_t i = 0; i < templateTraitInst->templateArguments().size(); ++i) {
                    if (!ConstExprHelper::compareAreSame(templateTraitInst->templateArguments()[i], templateArguments[i])) {
                        isMatch = false;
                        break;
                    }
                }

                if (isMatch) {
                    *result = templateTraitInst;
                    return false;
                }
            }

            // While checking the arguments are valid we will also create a copy of the arguments
            std::vector<Expr*> copiedTemplateArguments;
            copiedTemplateArguments.reserve(templateArguments.size());

            // Once we've reached this point it means no match was found. Before we do anything else we need to make
            // sure the provided arguments are valid for the provided parameters
            for (std::size_t i = 0; i < _templateParameters.size(); ++i) {
                if (_templateParameters[i]->templateParameterKind() == TemplateParameterDecl::TemplateParameterKind::Typename) {
                    if (!llvm::isa<TypeExpr>(templateArguments[i])) {
                        std::cerr << "INTERNAL ERROR: `TemplateTraitDecl::getInstantiation` received non-type argument where type was expected!" << std::endl;
                        std::exit(1);
                    }
                } else {
                    // Const
                    if (llvm::isa<TypeExpr>(templateArguments[i])) {
                        std::cerr << "INTERNAL ERROR: `TemplateTraitDecl::getInstantiation` received type argument where const literal was expected!" << std::endl;
                        std::exit(1);
                    }
                }

                copiedTemplateArguments.push_back(templateArguments[i]->deepCopy());
            }

            std::vector<Attr*> copiedAttributes;
            copiedAttributes.reserve(_attributes.size());
            std::vector<Type*> copiedInheritedTypes;
            copiedInheritedTypes.reserve(_inheritedTypes.size());
            std::vector<Cont*> copiedContracts;
            copiedContracts.reserve(_contracts.size());
            std::vector<Decl*> copiedOwnedMembers;
            copiedOwnedMembers.reserve(_ownedMembers.size());

            for (Attr* attribute : _attributes) {
                copiedAttributes.push_back(attribute->deepCopy());
            }

            for (Type* inheritedType : _inheritedTypes) {
                copiedInheritedTypes.push_back(inheritedType->deepCopy());
            }

            for (Cont* contract : _contracts) {
                copiedContracts.push_back(contract->deepCopy());
            }

            for (Decl* ownedMember : _ownedMembers) {
                copiedOwnedMembers.push_back(ownedMember->deepCopy());
            }

            // Here we "steal" the `templateArguments` (since we don't have a way to deep copy)
            *result = new TemplateTraitInstDecl(_sourceFileID, copiedAttributes, _declVisibility, _isConstExpr,
                                                _identifier, _declModifiers,
                                                _startPosition, _endPosition,
                                                copiedInheritedTypes, copiedContracts, copiedOwnedMembers,
                                                this, copiedTemplateArguments);

            _templateInstantiations.push_back(*result);

            return true;
        }

        Decl* deepCopy() const override;

        ~TemplateTraitDecl() override {
            for (TemplateParameterDecl* templateParameter : _templateParameters) {
                delete templateParameter;
            }

            for (TemplateTraitInstDecl* templateInstantiation : _templateInstantiations) {
                delete templateInstantiation;
            }
        }

    protected:
        std::vector<TemplateParameterDecl*> _templateParameters;
        std::vector<TemplateTraitInstDecl*> _templateInstantiations;

        TemplateTraitDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                           bool isConstExpr, Identifier identifier, DeclModifiers declModifiers,
                           TextPosition startPosition, TextPosition endPosition,
                           std::vector<Type*> inheritedTypes, std::vector<Cont*> contracts,
                           std::vector<Decl*> ownedMembers, std::vector<TemplateParameterDecl*> templateParameters,
                           std::vector<TemplateTraitInstDecl*> templateInstantiations)
                : TraitDecl(Decl::Kind::TemplateTrait, sourceFileID, std::move(attributes), visibility,
                            isConstExpr, std::move(identifier), declModifiers, startPosition, endPosition,
                            std::move(inheritedTypes), std::move(contracts), std::move(ownedMembers)),
                  _templateParameters(std::move(templateParameters)),
                  _templateInstantiations(std::move(templateInstantiations)) {}

    };
}

#endif //GULC_TEMPLATETRAITDECL_HPP
