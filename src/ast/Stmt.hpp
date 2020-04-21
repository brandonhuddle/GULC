#ifndef GULC_STMT_HPP
#define GULC_STMT_HPP

#include "Node.hpp"

namespace gulc {
    class Stmt : public Node {
    public:
        static bool classof(const Node* node) {
            Node::Kind nodeKind = node->getNodeKind();
            return nodeKind == Node::Kind::Stmt || nodeKind == Node::Kind::Expr;
        }

        enum class Kind {
            Expr,

            Break,
            Case,
            Catch,
            Compound,
            Continue,
            Do,
            For,
            Goto,
            If,
            Labeled,
            Return,
            Switch,
            Try,
            While,
        };

        Kind getStmtKind() const { return _stmtKind; }
        virtual Stmt* deepCopy() const = 0;

    protected:
        Kind _stmtKind;

        explicit Stmt(Stmt::Kind stmtKind)
                : Stmt(Node::Kind::Stmt, stmtKind) {}
        Stmt(Node::Kind nodeKind, Stmt::Kind stmtKind)
                : Node(nodeKind), _stmtKind(stmtKind) {}

    };
}

#endif //GULC_STMT_HPP
