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
#ifndef GULC_POINTERTYPE_HPP
#define GULC_POINTERTYPE_HPP

#include <ast/Type.hpp>

namespace gulc {
    class PointerType : public Type {
    public:
        static bool classof(const Type* type) { return type->getTypeKind() == Type::Kind::Pointer; }

        Type* nestedType;

        PointerType(Qualifier qualifier, Type* nestedType)
                : Type(Type::Kind::Pointer, qualifier, false), nestedType(nestedType) {}

        TextPosition startPosition() const override { return nestedType->startPosition(); }
        TextPosition endPosition() const override { return nestedType->endPosition(); }

        std::string toString() const override {
            return "*" + nestedType->toString();
        }

        Type* deepCopy() const override {
            auto result = new PointerType(_qualifier, nestedType->deepCopy());
            result->setIsLValue(_isLValue);
            return result;
        }

        ~PointerType() override {
            delete nestedType;
        }

    };
}

#endif //GULC_POINTERTYPE_HPP
