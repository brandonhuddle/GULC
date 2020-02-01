#ifndef GULC_TEMPLATEFUNCTIONDECL_HPP
#define GULC_TEMPLATEFUNCTIONDECL_HPP

#include <ast/Decl.hpp>
#include "FunctionDecl.hpp"
#include "TemplateParameterDecl.hpp"

namespace gulc {
    class TemplateFunctionDecl : public FunctionDecl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::TemplateFunction; }

        std::vector<TemplateParameterDecl*>& templateParameters() { return _templateParameters; }
        std::vector<TemplateParameterDecl*> const& templateParameters() const { return _templateParameters; }

        TemplateFunctionDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                             bool isConstExpr, Identifier identifier, DeclModifiers declModifiers,
                             std::vector<ParameterDecl*> parameters, Type* returnType,
                             std::vector<Cont*> contracts, CompoundStmt* body,
                             TextPosition startPosition, TextPosition endPosition,
                             std::vector<TemplateParameterDecl*> templateParameters)
                : FunctionDecl(Decl::Kind::TemplateFunction, sourceFileID, std::move(attributes), visibility,
                               isConstExpr, std::move(identifier), declModifiers, std::move(parameters), returnType,
                               std::move(contracts), body, startPosition, endPosition),
                  _templateParameters(std::move(templateParameters)) {}

        ~TemplateFunctionDecl() override {
            for (TemplateParameterDecl* templateParameter : _templateParameters) {
                delete templateParameter;
            }
        }

    protected:
        std::vector<TemplateParameterDecl*> _templateParameters;

    };
}

#endif //GULC_TEMPLATEFUNCTIONDECL_HPP
