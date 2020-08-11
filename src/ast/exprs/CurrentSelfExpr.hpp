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
#ifndef GULC_CURRENTSELFEXPR_HPP
#define GULC_CURRENTSELFEXPR_HPP

#include <ast/Expr.hpp>

namespace gulc {
    /// Expression to store the use of `self`, this makes working with the AST just slightly nicer over an explicit
    /// parameter 0 index
    class CurrentSelfExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::CurrentSelf; }

        CurrentSelfExpr(TextPosition startPosition, TextPosition endPosition)
                : Expr(Expr::Kind::CurrentSelf),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Expr* deepCopy() const override {
            auto result = new CurrentSelfExpr(_startPosition, _endPosition);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return "self";
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_CURRENTSELFEXPR_HPP
