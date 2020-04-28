#ifndef GULC_TEMPLATESTRUCTDECL_HPP
#define GULC_TEMPLATESTRUCTDECL_HPP

#include <vector>
#include <iostream>
#include <utilities/ConstExprHelper.hpp>
#include <ast/exprs/TypeExpr.hpp>
#include <ast/exprs/ValueLiteralExpr.hpp>
#include "StructDecl.hpp"
#include "TemplateParameterDecl.hpp"
#include "TemplateStructInstDecl.hpp"

namespace gulc {
    class TemplateStructDecl : public StructDecl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::TemplateStruct; }

        std::vector<TemplateParameterDecl*>& templateParameters() { return _templateParameters; }
        std::vector<TemplateParameterDecl*> const& templateParameters() const { return _templateParameters; }
        std::vector<TemplateStructInstDecl*>& templateInstantiations() { return _templateInstantiations; }
        std::vector<TemplateStructInstDecl*> const& templateInstantiations() const { return _templateInstantiations; }

        TemplateStructDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                           bool isConstExpr, Identifier identifier,
                           TextPosition startPosition, TextPosition endPosition,
                           Kind structKind, std::vector<Type*> inheritedTypes, std::vector<Cont*> contracts,
                           std::vector<Decl*> ownedMembers, std::vector<ConstructorDecl*> constructors,
                           DestructorDecl* destructor, std::vector<TemplateParameterDecl*> templateParameters)
                : TemplateStructDecl(sourceFileID, std::move(attributes), visibility,
                                     isConstExpr, std::move(identifier), startPosition, endPosition, structKind,
                                     std::move(inheritedTypes), std::move(contracts), std::move(ownedMembers),
                                     std::move(constructors), destructor, std::move(templateParameters), {}) {}

        /**
         * Get an instantiation of the template for the provided template arguments
         *
         * NOTE: The instantiation is NOT guaranteed to be completely constructed.
         * NOTE: `templateArguments` MUST have default values included
         *
         * @param templateArguments - arguments for the template
         * @param result - the found struct instantiation
         *
         * @return true for new TemplateStructInstDecl, false for existing
         */
        bool getInstantiation(std::vector<Expr*> const& templateArguments, TemplateStructInstDecl** result) {
            // NOTE: We don't account for default parameters here. `templateArguments` MUST have the default values.
            if (templateArguments.size() != _templateParameters.size()) {
                std::cerr << "INTERNAL ERROR[`TemplateStructDecl::getInstantiation`]: `templateArguments.size()` MUST BE equal to `_templateParameters.size()`!" << std::endl;
                std::exit(1);
            }

            // We don't check if the provided template arguments are valid UNLESS the isn't an existing match
            // the reason for this is if the template arguments are invalid that is an error and it is okay to take
            // slightly longer on failure, it is not okay to take longer on successes (especially when the check is
            // just a waste of time if it already exists)
            for (TemplateStructInstDecl* templateStructInst : _templateInstantiations) {
                // NOTE: We don't deal with "isExact" here, there are only matches and non-matches.
                bool isMatch = true;

                for (std::size_t i = 0; i < templateStructInst->templateArguments().size(); ++i) {
                    if (!ConstExprHelper::compareAreSame(templateStructInst->templateArguments()[i], templateArguments[i])) {
                        isMatch = false;
                        break;
                    }
                }

                if (isMatch) {
                    *result = templateStructInst;
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
                        std::cerr << "INTERNAL ERROR: `TemplateStructDecl::getInstantiation` received non-type argument where type was expected!" << std::endl;
                        std::exit(1);
                    }
                } else {
                    // Const
                    if (llvm::isa<TypeExpr>(templateArguments[i])) {
                        std::cerr << "INTERNAL ERROR: `TemplateStructDecl::getInstantiation` received type argument where const literal was expected!" << std::endl;
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
            std::vector<ConstructorDecl*> copiedConstructors;
            copiedConstructors.reserve(_constructors.size());
            DestructorDecl* copiedDestructorDecl = nullptr;

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

            for (ConstructorDecl* constructor : _constructors) {
                copiedConstructors.push_back(llvm::dyn_cast<ConstructorDecl>(constructor->deepCopy()));
            }

            if (destructor != nullptr) {
                copiedDestructorDecl = llvm::dyn_cast<DestructorDecl>(destructor->deepCopy());
            }

            // Here we "steal" the `templateArguments` (since we don't have a way to deep copy)
            *result = new TemplateStructInstDecl(_sourceFileID, copiedAttributes, _declVisibility, _isConstExpr,
                                                 _identifier, _startPosition, _endPosition, _structKind,
                                                 copiedInheritedTypes, copiedContracts, copiedOwnedMembers,
                                                 copiedConstructors, copiedDestructorDecl,
                                                 this, copiedTemplateArguments);

            _templateInstantiations.push_back(*result);

            return true;
        }

        Decl* deepCopy() const override {
            std::vector<Attr*> copiedAttributes;
            copiedAttributes.reserve(_attributes.size());
            std::vector<Type*> copiedInheritedTypes;
            copiedInheritedTypes.reserve(_inheritedTypes.size());
            std::vector<Cont*> copiedContracts;
            copiedContracts.reserve(_contracts.size());
            std::vector<Decl*> copiedOwnedMembers;
            copiedOwnedMembers.reserve(_ownedMembers.size());
            std::vector<ConstructorDecl*> copiedConstructors;
            copiedConstructors.reserve(_constructors.size());
            DestructorDecl* copiedDestructorDecl = nullptr;
            std::vector<TemplateParameterDecl*> copiedTemplateParameters;
            copiedTemplateParameters.reserve(_templateParameters.size());
            std::vector<TemplateStructInstDecl*> copiedTemplateInstantiations;
            copiedTemplateInstantiations.reserve(_templateInstantiations.size());

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

            for (ConstructorDecl* constructor : _constructors) {
                copiedConstructors.push_back(llvm::dyn_cast<ConstructorDecl>(constructor->deepCopy()));
            }

            for (TemplateParameterDecl* templateParameter : _templateParameters) {
                copiedTemplateParameters.push_back(llvm::dyn_cast<TemplateParameterDecl>(templateParameter->deepCopy()));
            }

            for (TemplateStructInstDecl* templateStructInst : _templateInstantiations) {
                copiedTemplateInstantiations.push_back(llvm::dyn_cast<TemplateStructInstDecl>(templateStructInst->deepCopy()));
            }

            if (destructor != nullptr) {
                copiedDestructorDecl = llvm::dyn_cast<DestructorDecl>(destructor->deepCopy());
            }

            return new TemplateStructDecl(_sourceFileID, copiedAttributes, _declVisibility, _isConstExpr,
                                          _identifier, _startPosition, _endPosition,
                                          _structKind, copiedInheritedTypes, copiedContracts, copiedOwnedMembers,
                                          copiedConstructors, copiedDestructorDecl, copiedTemplateParameters,
                                          copiedTemplateInstantiations);
        }

        ~TemplateStructDecl() override {
            for (TemplateParameterDecl* templateParameter : _templateParameters) {
                delete templateParameter;
            }

            for (TemplateStructInstDecl* templateInstantiation : _templateInstantiations) {
                delete templateInstantiation;
            }
        }

    protected:
        std::vector<TemplateParameterDecl*> _templateParameters;
        std::vector<TemplateStructInstDecl*> _templateInstantiations;

        TemplateStructDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                           bool isConstExpr, Identifier identifier,
                           TextPosition startPosition, TextPosition endPosition,
                           Kind structKind, std::vector<Type*> inheritedTypes, std::vector<Cont*> contracts,
                           std::vector<Decl*> ownedMembers, std::vector<ConstructorDecl*> constructors,
                           DestructorDecl* destructor, std::vector<TemplateParameterDecl*> templateParameters,
                           std::vector<TemplateStructInstDecl*> templateInstantiations)
                : StructDecl(Decl::Kind::TemplateStruct, sourceFileID, std::move(attributes), visibility,
                             isConstExpr, std::move(identifier), startPosition, endPosition, structKind,
                             std::move(inheritedTypes), std::move(contracts), std::move(ownedMembers),
                             std::move(constructors), destructor),
                  _templateParameters(std::move(templateParameters)),
                  _templateInstantiations(std::move(templateInstantiations)) {}

    };
}

#endif //GULC_TEMPLATESTRUCTDECL_HPP
