#ifndef GULC_VARIABLEDECL_HPP
#define GULC_VARIABLEDECL_HPP

#include <ast/Decl.hpp>
#include <ast/Type.hpp>
#include <ast/Expr.hpp>
#include <ast/DeclModifiers.hpp>

namespace gulc {
    class VariableDecl : public Decl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::Variable; }

        Type* type;
        Expr* initialValue;

        bool hasInitialValue() const { return initialValue != nullptr; }
        DeclModifiers declModifiers() const { return _declModifiers; }

        VariableDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                     bool isConstExpr, Identifier identifier, Type* type, Expr* initialValue,
                     TextPosition startPosition, TextPosition endPosition, DeclModifiers declModifiers)
                : Decl(Decl::Kind::Variable, sourceFileID, std::move(attributes), visibility, isConstExpr,
                       std::move(identifier)),
                  type(type), initialValue(initialValue), _startPosition(startPosition), _endPosition(endPosition),
                  _declModifiers(declModifiers) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        bool isStatic() const { return (_declModifiers & DeclModifiers::Static) == DeclModifiers::Static; }
        bool isMutable() const { return (_declModifiers & DeclModifiers::Mut) == DeclModifiers::Mut; }
        bool isVolatile() const { return (_declModifiers & DeclModifiers::Volatile) == DeclModifiers::Volatile; }
        bool isAbstract() const { return (_declModifiers & DeclModifiers::Abstract) == DeclModifiers::Abstract; }
        bool isVirtual() const { return (_declModifiers & DeclModifiers::Virtual) == DeclModifiers::Virtual; }
        bool isOverride() const { return (_declModifiers & DeclModifiers::Override) == DeclModifiers::Override; }

        ~VariableDecl() override {
            delete type;
            delete initialValue;
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;
        DeclModifiers _declModifiers;

    };
}

#endif //GULC_VARIABLEDECL_HPP
