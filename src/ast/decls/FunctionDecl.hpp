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
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::Function; }

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

        bool isStatic() const { return (_declModifiers & DeclModifiers::Static) == DeclModifiers::Static; }
        bool isMutable() const { return (_declModifiers & DeclModifiers::Mut) == DeclModifiers::Mut; }
        bool isVolatile() const { return (_declModifiers & DeclModifiers::Volatile) == DeclModifiers::Volatile; }
        bool isAbstract() const { return (_declModifiers & DeclModifiers::Abstract) == DeclModifiers::Abstract; }
        bool isVirtual() const { return (_declModifiers & DeclModifiers::Virtual) == DeclModifiers::Virtual; }
        bool isOverride() const { return (_declModifiers & DeclModifiers::Override) == DeclModifiers::Override; }

        bool throws() const { return _throws; }
        bool hasContract() const { return !_contracts.empty(); }

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

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
                : Decl(declKind, sourceFileID, std::move(attributes), visibility, isConstExpr, std::move(identifier)),
                  _declModifiers(declModifiers), _parameters(std::move(parameters)), returnType(returnType),
                  _contracts(std::move(contracts)), _body(body),
                  _startPosition(startPosition), _endPosition(endPosition), _throws(false) {
            for (Cont* contract : _contracts) {
                if (llvm::isa<ThrowsCont>(contract)) {
                    _throws = true;
                    break;
                }
            }
        }

        DeclModifiers _declModifiers;
        std::vector<ParameterDecl*> _parameters;
        std::vector<Cont*> _contracts;
        CompoundStmt* _body;
        TextPosition _startPosition;
        TextPosition _endPosition;
        bool _throws;

    };
}

#endif //GULC_FUNCTIONDECL_HPP
