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
#ifndef GULC_TYPEEXPR_HPP
#define GULC_TYPEEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/Type.hpp>

namespace gulc {
    /**
     * `Expr` container for a `Type`, this allows `Type`s to be found anywhere an expression is expected.
     */
    class TypeExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::Type; }

        Type* type;

        explicit TypeExpr(Type* type)
                : Expr(Expr::Kind::Type), type(type) {}

        TextPosition startPosition() const override { return type->startPosition(); }
        TextPosition endPosition() const override { return type->endPosition(); }

        Expr* deepCopy() const override {
            auto result = new TypeExpr(type->deepCopy());
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return type->toString();
        }

        ~TypeExpr() override {
            delete type;
        }

    };
}

#endif //GULC_TYPEEXPR_HPP
