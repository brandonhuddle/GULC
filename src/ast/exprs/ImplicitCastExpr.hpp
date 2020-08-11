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
#ifndef GULC_IMPLICITCASTEXPR_HPP
#define GULC_IMPLICITCASTEXPR_HPP

#include <ast/Expr.hpp>

namespace gulc {
    /// Expression to represent an implicit cast. Implicit casts should be kept to a minimum within the language to
    /// prevent accidental loss of precision/data and prevent
    class ImplicitCastExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::ImplicitCast; }

        Expr* expr;
        Type* castToType;

        ImplicitCastExpr(Expr* expr, Type* castToType)
                : Expr(Expr::Kind::ImplicitCast),
                  expr(expr), castToType(castToType) {}

        TextPosition startPosition() const override { return expr->startPosition(); }
        TextPosition endPosition() const override { return castToType->endPosition(); }

        Expr* deepCopy() const override {
            auto result = new ImplicitCastExpr(expr->deepCopy(), castToType->deepCopy());
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return expr->toString();
        }

        ~ImplicitCastExpr() override {
            delete expr;
            delete castToType;
        }

    };
}

#endif //GULC_IMPLICITCASTEXPR_HPP
