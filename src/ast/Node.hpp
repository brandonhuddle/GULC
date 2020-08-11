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
#ifndef GULC_NODE_HPP
#define GULC_NODE_HPP

namespace gulc {
    struct TextPosition {
        unsigned int index;
        unsigned int line;
        unsigned int column;

        TextPosition()
                : index(0), line(0), column(0) {}

        TextPosition(unsigned int index, unsigned int line, unsigned int column)
                : index(index), line(line), column(column) {}
    };

    /**
     * Node is the base AST class for all AST objects. It contains all properties required by all other nodes
     */
    class Node {
    public:
        enum class Kind {
            Attr,
            Cont,
            Decl,
            Expr,
            Identifier,
            Stmt,
            Type
        };

        Kind getNodeKind() const { return _nodeKind; }

        virtual TextPosition startPosition() const = 0;
        virtual TextPosition endPosition() const = 0;

        virtual ~Node() = default;

    protected:
        Kind _nodeKind;

        explicit Node(Kind kind) : _nodeKind(kind) {}

    };
}

#endif //GULC_NODE_HPP
