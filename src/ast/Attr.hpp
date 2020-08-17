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
#ifndef GULC_ATTR_HPP
#define GULC_ATTR_HPP

#include "Node.hpp"

namespace gulc {
    class Attr : public Node {
    public:
        static bool classof(const Node* node) { return node->getNodeKind() == Node::Kind::Attr; }

        enum class Kind {
            Copy,
            // References an `AttributeDecl`...
            Custom,
            // An unresolved attribute. Could be `pod` or something custom
            Unresolved,

            Pod
        };

        Attr::Kind getAttrKind() const { return _attrKind; }

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }
        virtual Attr* deepCopy() const = 0;

    protected:
        Attr::Kind _attrKind;
        TextPosition _startPosition;
        TextPosition _endPosition;

        Attr(Attr::Kind attrKind, TextPosition startPosition, TextPosition endPosition)
                : Node(Node::Kind::Attr), _attrKind(attrKind),
                  _startPosition(startPosition), _endPosition(endPosition) {}

    };
}

#endif //GULC_ATTR_HPP
