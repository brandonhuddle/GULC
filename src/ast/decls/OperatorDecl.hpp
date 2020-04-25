#ifndef GULC_OPERATORDECL_HPP
#define GULC_OPERATORDECL_HPP

#include <string>
#include "FunctionDecl.hpp"

namespace gulc {
    enum class OperatorType {
        Unknown,
        Prefix,
        Infix,
        // TODO: We don't parse this right now. Only really used for '++' and '--', you can't make a custom one
        Postfix

    };

    std::string operatorTypeName(OperatorType operatorType);

    class OperatorDecl : public FunctionDecl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::Operator; }

        OperatorDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                     bool isConstExpr, OperatorType operatorType, Identifier operatorIdentifier,
                     DeclModifiers declModifiers, std::vector<ParameterDecl*> parameters, Type* returnType,
                     std::vector<Cont*> contracts, CompoundStmt* body,
                     TextPosition startPosition, TextPosition endPosition)
                : FunctionDecl(Decl::Kind::Operator, sourceFileID, std::move(attributes), visibility,
                               isConstExpr,
                               Identifier(operatorIdentifier.startPosition(),
                                          operatorIdentifier.endPosition(),
                                          operatorTypeName(operatorType) + "." + operatorIdentifier.name()),
                               declModifiers, std::move(parameters), returnType,
                               std::move(contracts), body, startPosition, endPosition),
                  _operatorType(operatorType), _operatorIdentifier(std::move(operatorIdentifier)) {}

        OperatorType operatorType() const { return _operatorType; }
        Identifier operatorIdentifier() const { return _operatorIdentifier; }

        Decl* deepCopy() const override {
            std::vector<Attr*> copiedAttributes;
            copiedAttributes.reserve(_attributes.size());
            std::vector<ParameterDecl*> copiedParameters;
            copiedParameters.reserve(_parameters.size());
            std::vector<Cont*> copiedContracts;
            copiedContracts.reserve(_contracts.size());
            Type* copiedReturnType = nullptr;

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

            return new OperatorDecl(_sourceFileID, copiedAttributes, _declVisibility, _isConstExpr,
                                    _operatorType, _operatorIdentifier, _declModifiers, copiedParameters,
                                    copiedReturnType, copiedContracts,
                                    llvm::dyn_cast<CompoundStmt>(_body->deepCopy()),
                                    _startPosition, _endPosition);
        }

    protected:
        OperatorType _operatorType;
        Identifier _operatorIdentifier;

    };
}

#endif //GULC_OPERATORDECL_HPP
