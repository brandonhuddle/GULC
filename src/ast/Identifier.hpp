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
#ifndef GULC_IDENTIFIER_HPP
#define GULC_IDENTIFIER_HPP

#include <string>
#include "Node.hpp"

namespace gulc {
    class Identifier final : public Node {
    public:
        static bool classof(const Node* node) { return node->getNodeKind() == Node::Kind::Identifier; }

        Identifier()
                : Identifier({}, {}, "") {}

        Identifier(TextPosition startPositon, TextPosition endPosition, std::string name)
                : Node(Node::Kind::Identifier),
                  _startPosition(startPositon), _endPosition(endPosition), _name(std::move(name)) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }
        std::string const& name() const { return _name; }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;
        std::string _name;

    };
}

#endif //GULC_IDENTIFIER_HPP
