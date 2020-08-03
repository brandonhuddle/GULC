#ifndef GULC_TEMPLATEFUNCTIONDECL_HPP
#define GULC_TEMPLATEFUNCTIONDECL_HPP

#include <ast/Decl.hpp>
#include "FunctionDecl.hpp"
#include "TemplateParameterDecl.hpp"
#include "TemplateFunctionInstDecl.hpp"

namespace gulc {
    class TemplateFunctionDecl : public FunctionDecl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::TemplateFunction; }

        std::vector<TemplateParameterDecl*>& templateParameters() { return _templateParameters; }
        std::vector<TemplateParameterDecl*> const& templateParameters() const { return _templateParameters; }
        std::vector<TemplateFunctionInstDecl*>& templateInstantiations() { return _templateInstantiations; }
        std::vector<TemplateFunctionInstDecl*> const& templateInstantiations() const { return _templateInstantiations; }

        TemplateFunctionDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                             bool isConstExpr, Identifier identifier, DeclModifiers declModifiers,
                             std::vector<ParameterDecl*> parameters, Type* returnType,
                             std::vector<Cont*> contracts, CompoundStmt* body,
                             TextPosition startPosition, TextPosition endPosition,
                             std::vector<TemplateParameterDecl*> templateParameters)
                : TemplateFunctionDecl(sourceFileID, std::move(attributes), visibility, isConstExpr,
                                       std::move(identifier), declModifiers, std::move(parameters), returnType,
                                       std::move(contracts), body, startPosition, endPosition,
                                       std::move(templateParameters), {}) {}

        bool getInstantiation(std::vector<Expr*> const& templateArguments, TemplateFunctionInstDecl** result);

        Decl* deepCopy() const override;

        ~TemplateFunctionDecl() override {
            for (TemplateParameterDecl* templateParameter : _templateParameters) {
                delete templateParameter;
            }
        }

        // This is used to allow us to split contract instantiation off into its own function within `DeclInstantiator`
        bool contractsAreInstantiated = false;

    protected:
        std::vector<TemplateParameterDecl*> _templateParameters;
        std::vector<TemplateFunctionInstDecl*> _templateInstantiations;

        TemplateFunctionDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                             bool isConstExpr, Identifier identifier, DeclModifiers declModifiers,
                             std::vector<ParameterDecl*> parameters, Type* returnType,
                             std::vector<Cont*> contracts, CompoundStmt* body,
                             TextPosition startPosition, TextPosition endPosition,
                             std::vector<TemplateParameterDecl*> templateParameters,
                             std::vector<TemplateFunctionInstDecl*> templateInstantiations)
                : FunctionDecl(Decl::Kind::TemplateFunction, sourceFileID, std::move(attributes), visibility,
                               isConstExpr, std::move(identifier), declModifiers, std::move(parameters), returnType,
                               std::move(contracts), body, startPosition, endPosition),
                  _templateParameters(std::move(templateParameters)),
                  _templateInstantiations(std::move(templateInstantiations)) {}

    };
}

#endif //GULC_TEMPLATEFUNCTIONDECL_HPP
