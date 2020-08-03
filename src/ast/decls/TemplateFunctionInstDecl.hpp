#ifndef GULC_TEMPLATEFUNCTIONINSTDECL_HPP
#define GULC_TEMPLATEFUNCTIONINSTDECL_HPP

#include "FunctionDecl.hpp"

namespace gulc {
    class TemplateFunctionDecl;

    class TemplateFunctionInstDecl : public FunctionDecl {
        friend TemplateFunctionDecl;

    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::TemplateFunctionInst; }

        std::vector<Expr*>& templateArguments() { return _templateArguments; }
        std::vector<Expr*> const& templateArguments() const { return _templateArguments; }
        TemplateFunctionDecl* parentTemplateStruct() const { return _parentTemplateStruct; }

        Decl* deepCopy() const override {
            std::vector<Attr*> copiedAttributes;
            copiedAttributes.reserve(_attributes.size());
            std::vector<ParameterDecl*> copiedParameters;
            copiedParameters.reserve(_parameters.size());
            std::vector<Cont*> copiedContracts;
            copiedContracts.reserve(_contracts.size());
            Type* copiedReturnType = nullptr;
            std::vector<Expr*> copiedTemplateArguments;
            copiedTemplateArguments.reserve(_templateArguments.size());

            for (Attr* attribute : _attributes) {
                copiedAttributes.push_back(attribute->deepCopy());
            }

            for (ParameterDecl* parameter : _parameters) {
                copiedParameters.push_back(llvm::dyn_cast<ParameterDecl>(parameter->deepCopy()));
            }

            for (Cont* contract : _contracts) {
                copiedContracts.push_back(contract->deepCopy());
            }

            if (returnType != nullptr) {
                copiedReturnType = returnType->deepCopy();
            }

            for (Expr* templateArgument : _templateArguments) {
                copiedTemplateArguments.push_back(templateArgument->deepCopy());
            }

            auto result = new TemplateFunctionInstDecl(_sourceFileID, copiedAttributes, _declVisibility, _isConstExpr,
                                                       _identifier, _declModifiers, copiedParameters,
                                                       copiedReturnType, copiedContracts,
                                                       llvm::dyn_cast<CompoundStmt>(_body->deepCopy()),
                                                       _startPosition, _endPosition,
                                                       _parentTemplateStruct, copiedTemplateArguments);
            result->container = container;
            result->containedInTemplate = containedInTemplate;
            result->originalDecl = (originalDecl == nullptr ? this : originalDecl);
            return result;
        }

        ~TemplateFunctionInstDecl() override {
            for (Expr* templateArgument : _templateArguments) {
                delete templateArgument;
            }
        }

    protected:
        TemplateFunctionDecl* _parentTemplateStruct;
        std::vector<Expr*> _templateArguments;

        TemplateFunctionInstDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                                 bool isConstExpr, Identifier identifier, DeclModifiers declModifiers,
                                 std::vector<ParameterDecl*> parameters, Type* returnType,
                                 std::vector<Cont*> contracts, CompoundStmt* body,
                                 TextPosition startPosition, TextPosition endPosition,
                                 TemplateFunctionDecl* parentTemplateStruct, std::vector<Expr*> templateArguments)
                : FunctionDecl(Decl::Kind::TemplateFunctionInst, sourceFileID, std::move(attributes), visibility,
                               isConstExpr, std::move(identifier), declModifiers, std::move(parameters),
                               returnType, std::move(contracts), body, startPosition, endPosition),
                  _parentTemplateStruct(parentTemplateStruct), _templateArguments(std::move(templateArguments)) {}

    };
}

#endif //GULC_TEMPLATEFUNCTIONINSTDECL_HPP
