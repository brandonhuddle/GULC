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
#ifndef GULC_CONT_HPP
#define GULC_CONT_HPP

#include "Node.hpp"

namespace gulc {
    // Contract
    class Cont : public Node {
    public:
        static bool classof(const Node* node) { return node->getNodeKind() == Node::Kind::Cont; }

        enum class Kind {
            Requires,
            Ensures,
            Throws,
            Where
        };

        Cont::Kind getContKind() const { return _contKind; }
        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }
        virtual Cont* deepCopy() const = 0;

    protected:
        Cont::Kind _contKind;
        TextPosition _startPosition;
        TextPosition _endPosition;

        Cont(Cont::Kind contKind, TextPosition startPosition, TextPosition endPosition)
                : Node(Node::Kind::Cont), _contKind(contKind),
                  _startPosition(startPosition), _endPosition(endPosition) {}

    };
}

#endif //GULC_CONT_HPP
