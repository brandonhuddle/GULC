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
#ifndef GULC_LVALUETORVALUEEXPR_HPP
#define GULC_LVALUETORVALUEEXPR_HPP

#include <ast/Expr.hpp>

namespace gulc {
    /// Dereferences the implicit lvalue reference
    class LValueToRValueExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::LValueToRValue; }

        Expr* lvalue;

        explicit LValueToRValueExpr(Expr* lvalue)
                : Expr(Expr::Kind::LValueToRValue),
                  lvalue(lvalue) {}

        TextPosition startPosition() const override { return lvalue->startPosition(); }
        TextPosition endPosition() const override { return lvalue->endPosition(); }

        Expr* deepCopy() const override {
            auto result = new LValueToRValueExpr(lvalue->deepCopy());
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return lvalue->toString();
        }

        ~LValueToRValueExpr() override {
            delete lvalue;
        }

    };
}

#endif //GULC_LVALUETORVALUEEXPR_HPP
