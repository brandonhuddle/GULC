#ifndef GULC_FUNCTIONDECL_HPP
#define GULC_FUNCTIONDECL_HPP

#include <ast/Decl.hpp>
#include <ast/Type.hpp>
#include <vector>
#include <ast/stmts/CompoundStmt.hpp>
#include <ast/DeclModifiers.hpp>
#include <ast/Cont.hpp>
#include <ast/conts/ThrowsCont.hpp>
#include <llvm/Support/Casting.h>
#include "ParameterDecl.hpp"

namespace gulc {
    class FunctionDecl : public Decl {
    public:
        static bool classof(const Decl* decl) {
            Decl::Kind kind = decl->getDeclKind();

            return kind == Decl::Kind::CallOperator || kind == Decl::Kind::Constructor ||
                   kind == Decl::Kind::Destructor || kind == Decl::Kind::Function || kind == Decl::Kind::Operator ||
                   kind == Decl::Kind::TypeSuffix;
        }

        Type* returnType;

        FunctionDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                     bool isConstExpr, Identifier identifier, DeclModifiers declModifiers,
                     std::vector<ParameterDecl*> parameters, Type* returnType,
                     std::vector<Cont*> contracts, CompoundStmt* body,
                     TextPosition startPosition, TextPosition endPosition)
                : FunctionDecl(Decl::Kind::Function, sourceFileID, std::move(attributes), visibility,
                               isConstExpr, std::move(identifier), declModifiers, std::move(parameters),
                               returnType, std::move(contracts), body, startPosition, endPosition) {}

        DeclModifiers declModifiers() const { return _declModifiers; }
        std::vector<ParameterDecl*>& parameters() { return _parameters; }
        std::vector<ParameterDecl*> const& parameters() const { return _parameters; }
        std::vector<Cont*>& contracts() { return _contracts; }
        std::vector<Cont*> const& contracts() const { return _contracts; }
        CompoundStmt* body() const { return _body; }

        bool throws() const { return _throws; }
        bool hasContract() const { return !_contracts.empty(); }

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

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

            auto result = new FunctionDecl(_sourceFileID, copiedAttributes, _declVisibility, _isConstExpr,
                                           _identifier, _declModifiers, copiedParameters,
                                           copiedReturnType, copiedContracts,
                                           llvm::dyn_cast<CompoundStmt>(_body->deepCopy()),
                                           _startPosition, _endPosition);
            result->container = container;
            result->containedInTemplate = containedInTemplate;
            result->originalDecl = (originalDecl == nullptr ? this : originalDecl);
            return result;
        }

        ~FunctionDecl() override {
            for (ParameterDecl* parameter : _parameters) {
                delete parameter;
            }

            for (Cont* contract : _contracts) {
                delete contract;
            }

            delete returnType;
            delete _body;
        }

    protected:
        FunctionDecl(Decl::Kind declKind, unsigned int sourceFileID, std::vector<Attr*> attributes,
                     Decl::Visibility visibility, bool isConstExpr, Identifier identifier,
                     DeclModifiers declModifiers, std::vector<ParameterDecl*> parameters, Type* returnType,
                     std::vector<Cont*> contracts, CompoundStmt* body,
                     TextPosition startPosition, TextPosition endPosition)
                : Decl(declKind, sourceFileID, std::move(attributes), visibility, isConstExpr, std::move(identifier),
                       declModifiers),
                  _parameters(std::move(parameters)), returnType(returnType),
                  _contracts(std::move(contracts)), _body(body),
                  _startPosition(startPosition), _endPosition(endPosition), _throws(false) {
            for (Cont* contract : _contracts) {
                if (llvm::isa<ThrowsCont>(contract)) {
                    _throws = true;
                    break;
                }
            }
        }

        std::vector<ParameterDecl*> _parameters;
        std::vector<Cont*> _contracts;
        CompoundStmt* _body;
        TextPosition _startPosition;
        TextPosition _endPosition;
        bool _throws;

    };
}

#endif //GULC_FUNCTIONDECL_HPP
