#ifndef GULC_EXPR_HPP
#define GULC_EXPR_HPP

#include "Stmt.hpp"
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
            CheckExtendsType,
            FunctionCall,
            Has,
            Identifier,
            IndexerCall,
            InfixOperator,
            Is,
            LabeledArgument,
            MemberAccessCall,
            Paren,
            PostfixOperator,
            PrefixOperator,
            TemplateConstRefExpr,
            Ternary,
            Type,
            ValueLiteral,
            VariableDecl,
        };

        Expr::Kind getExprKind() const { return _exprKind; }
        virtual Expr* deepCopy() const = 0;
        virtual std::string toString() const = 0;

    protected:
        Expr::Kind _exprKind;

        explicit Expr(Expr::Kind exprKind)
                : __ExprStmtFix(Node::Kind::Expr, Stmt::Kind::Expr),
                  _exprKind(exprKind) {}

        Stmt* deepCopyStmt() const override {
            return deepCopy();
        }

    };
}

#endif //GULC_EXPR_HPP
