#ifndef GULC_EXPR_HPP
#define GULC_EXPR_HPP

#include "Stmt.hpp"

namespace gulc {
    class Expr : public Stmt {
    public:
        static bool classof(const Node* node) { return node->getNodeKind() == Node::Kind::Expr; }
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::Expr; }

        enum class Kind {
            ArrayLiteral,
            As,
            AssignmentOperator,
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
            Ternary,
            Type,
            ValueLiteral,
            VariableDecl,
        };

        Expr::Kind getExprKind() const { return _exprKind; }

    protected:
        Expr::Kind _exprKind;

        explicit Expr(Expr::Kind exprKind)
                : Stmt(Node::Kind::Expr, Stmt::Kind::Expr),
                  _exprKind(exprKind) {}

    };
}

#endif //GULC_EXPR_HPP
