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
#ifndef GULC_BOOLLITERALEXPR_HPP
#define GULC_BOOLLITERALEXPR_HPP

#include <ast/Expr.hpp>

namespace gulc {
    class BoolLiteralExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::BoolLiteral; }

        BoolLiteralExpr(TextPosition startPosition, TextPosition endPosition, bool value)
                : Expr(Expr::Kind::BoolLiteral),
                  _startPosition(startPosition), _endPosition(endPosition), _value(value) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }
        bool value() const { return _value; }

        Expr* deepCopy() const override {
            auto result = new BoolLiteralExpr(_startPosition, _endPosition, _value);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            if (_value) {
                return "true";
            } else {
                return "false";
            }
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;
        bool _value;

    };
}

#endif //GULC_BOOLLITERALEXPR_HPP
