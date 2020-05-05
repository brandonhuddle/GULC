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

        Decl* deepCopy() const override {
            std::vector<Attr*> copiedAttributes;
            copiedAttributes.reserve(_attributes.size());
            std::vector<ParameterDecl*> copiedParameters;
            copiedParameters.reserve(_parameters.size());
            std::vector<Cont*> copiedContracts;
            copiedContracts.reserve(_contracts.size());
            std::vector<TemplateParameterDecl*> copiedTemplateParameters;

            for (Attr* attribute : _attributes) {
                copiedAttributes.push_back(attribute->deepCopy());
            }

            for (ParameterDecl* parameter : _parameters) {
                copiedParameters.push_back(llvm::dyn_cast<ParameterDecl>(parameter->deepCopy()));
            }

            for (Cont* contract : _contracts) {
                copiedContracts.push_back(contract->deepCopy());
            }

            for (TemplateParameterDecl* templateParameter : _templateParameters) {
                copiedTemplateParameters.push_back(llvm::dyn_cast<TemplateParameterDecl>(templateParameter->deepCopy()));
            }

            auto result = new TemplateFunctionDecl(_sourceFileID, copiedAttributes, _declVisibility, _isConstExpr,
                                                   _identifier, _declModifiers, copiedParameters,
                                                   returnType->deepCopy(), copiedContracts,
                                                   llvm::dyn_cast<CompoundStmt>(_body->deepCopy()),
                                                   _startPosition, _endPosition, copiedTemplateParameters);
            result->container = container;
            result->containedInTemplate = containedInTemplate;
            return result;
        }

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
