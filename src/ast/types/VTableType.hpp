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
#ifndef GULC_VTABLETYPE_HPP
#define GULC_VTABLETYPE_HPP

#include <ast/Type.hpp>

namespace gulc {
    class VTableType : public Type {
    public:
        static bool classof(const Type* type) { return type->getTypeKind() == Type::Kind::VTable; }

        // NOTE: vtable is always const. They are NOT modifiable in GUL
        VTableType()
                : Type(Kind::VTable, Qualifier::Unassigned, false) {}

        TextPosition startPosition() const override {
            return {};
        }

        TextPosition endPosition() const override {
            return {};
        }

        std::string toString() const override { return "#vtable#"; }

        Type* deepCopy() const override {
            auto result = new VTableType();
            result->setIsLValue(_isLValue);
            return result;
        }

    };
}

#endif //GULC_VTABLETYPE_HPP
