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
#ifndef GULC_SELFTYPE_HPP
#define GULC_SELFTYPE_HPP

#include <ast/Type.hpp>

namespace gulc {
    /// NOTE: This is a partially resolved type, it should be replaced with a fully resolved type to what `Self`
    ///       actually is during processing.
    class SelfType : public Type {
    public:
        static bool classof(const Type* type) { return type->getTypeKind() == Type::Kind::Self; }

        SelfType(Qualifier qualifier, TextPosition startPosition, TextPosition endPosition)
                : Type(Type::Kind::Self, qualifier, false),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        std::string toString() const override {
            return "Self";
        }

        Type* deepCopy() const override {
            auto result = new SelfType(_qualifier, _startPosition, _endPosition);
            result->setIsLValue(_isLValue);
            return result;
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_SELFTYPE_HPP
