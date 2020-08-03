#ifndef GULC_EXPR_HPP
#define GULC_EXPR_HPP

#include "Stmt.hpp"
#include "Type.hpp"
#include <string>

namespace gulc {
    class __ExprStmtFix : public Stmt {
    public:
        explicit __ExprStmtFix(Stmt::Kind stmtKind)
                : __ExprStmtFix(Node::Kind::Stmt, stmtKind) {}
        __ExprStmtFix(Node::Kind nodeKind, Stmt::Kind stmtKind)
                : Stmt(nodeKind, stmtKind) {}

        Stmt* deepCopy() const override { return deepCopyStmt(); }

    protected:
        virtual Stmt* deepCopyStmt() const = 0;

    };

    class Expr : public __ExprStmtFix {
    public:
        static bool classof(const Node* node) { return node->getNodeKind() == Node::Kind::Expr; }
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::Expr; }

        enum class Kind {
            ArrayLiteral,
            As,
            AssignmentOperator,
            CallOperatorReference,
            CheckExtendsType,
            ConstructorCall,
            ConstructorReference,
            CurrentSelf,
            EnumConstRef,
            FunctionCall,
            FunctionReference,
            Has,
            Identifier,
            ImplicitCast,
            InfixOperator,
            Is,
            LabeledArgument,
            LocalVariableRef,
            MemberAccessCall,
            MemberFunctionCall,
            MemberPostfixOperatorCall,
            MemberPrefixOperatorCall,
            MemberPropertyRef,
            MemberSubscriptCall,
            MemberVariableRef,
            ParameterRef,
            Paren,
            PostfixOperator,
            PrefixOperator,
            PropertyRef,
            SubscriptCall,
            SubscriptRef,
            TemplateConstRef,
            Ternary,
            Type,
            ValueLiteral,
            VariableDecl,
            VariableRef,
            VTableFunctionReference,
        };

        Expr::Kind getExprKind() const { return _exprKind; }
        // I'm tired of seeing clang complain about this not being marked `override` in every file.
        // It isn't an override, it is SHADOWING. Annoying.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
        virtual Expr* deepCopy() const = 0;
#pragma clang diagnostic pop
        virtual std::string toString() const = 0;

        // This is the type for the current value (i.e. `12 + 12` would have the type `i32`, function calls would be
        // the type of the function's return type)
        Type* valueType;

        ~Expr() override {
            delete valueType;
        }

    protected:
        Expr::Kind _exprKind;

        explicit Expr(Expr::Kind exprKind)
                : __ExprStmtFix(Node::Kind::Expr, Stmt::Kind::Expr),
                  valueType(nullptr), _exprKind(exprKind) {}

        Stmt* deepCopyStmt() const override {
            return deepCopy();
        }

    };
}

#endif //GULC_EXPR_HPP
