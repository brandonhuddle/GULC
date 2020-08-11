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
#ifndef GULC_FLATARRAYTYPE_HPP
#define GULC_FLATARRAYTYPE_HPP

#include <ast/Type.hpp>
#include <ast/Expr.hpp>
#include <llvm/Support/Casting.h>

namespace gulc {
    class FlatArrayType : public Type {
    public:
        static bool classof(const Type* type) { return type->getTypeKind() == Type::Kind::FlatArray; }

        Type* indexType;
        Expr* length;

        FlatArrayType(Qualifier qualifier, Type* nestedType, Expr* length)
                : Type(Type::Kind::FlatArray, qualifier, false), indexType(nestedType), length(length) {}

        TextPosition startPosition() const override { return indexType->startPosition(); }
        TextPosition endPosition() const override { return indexType->endPosition(); }

        std::string toString() const override {
            return "[" + indexType->toString() + "; " + length->toString() + "]";
        }

        Type* deepCopy() const override {
            auto result = new FlatArrayType(_qualifier, indexType->deepCopy(), length->deepCopy());
            result->setIsLValue(_isLValue);
            return result;
        }

        ~FlatArrayType() override {
            delete indexType;
            delete length;
        }

    };
}

#endif //GULC_FLATARRAYTYPE_HPP
