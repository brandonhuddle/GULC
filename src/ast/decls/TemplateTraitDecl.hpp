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

        bool getInstantiation(std::vector<Expr*> const& templateArguments, TemplateTraitInstDecl** result);

        Decl* deepCopy() const override;

        ~TemplateTraitDecl() override {
            for (TemplateParameterDecl* templateParameter : _templateParameters) {
                delete templateParameter;
            }

            for (TemplateTraitInstDecl* templateInstantiation : _templateInstantiations) {
                delete templateInstantiation;
            }
        }

        // This is used to allow us to split contract instantiation off into its own function within `DeclInstantiator`
        bool contractsAreInstantiated = false;

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
