/*
 * Copyright (C) 2020 Brandon Huddle
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
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
            DoCatch,
            DoWhile,
            Fallthrough,
            For,
            Goto,
            If,
            Labeled,
            Return,
            Switch,
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
