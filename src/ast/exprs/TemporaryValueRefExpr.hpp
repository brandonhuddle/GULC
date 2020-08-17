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
#ifndef GULC_TEMPORARYVALUEREFEXPR_HPP
#define GULC_TEMPORARYVALUEREFEXPR_HPP

#include <ast/Expr.hpp>

namespace gulc {
    // NOTE: I've mainly made this class to make it easier to destruct temporary values without accidentally
    //       destructing local variables. I think when porting the compiler into Ghoul we will need our own IR that
    //       will allow us to remove a lot of the context that the AST requires
    class TemporaryValueRefExpr : public Expr {
    public:
        static bool classof(const Expr *expr) { return expr->getExprKind() == Kind::TemporaryValueRef; }

        TemporaryValueRefExpr(TextPosition startPosition, TextPosition endPosition, std::string temporaryName)
                : Expr(Expr::Kind::TemporaryValueRef),
                  _startPosition(startPosition), _endPosition(endPosition), _temporaryName(std::move(temporaryName)) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        std::string const& temporaryName() const { return _temporaryName; }

        Expr* deepCopy() const override {
            auto result = new TemporaryValueRefExpr(_startPosition, _endPosition,
                                                    _temporaryName);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return _temporaryName;
        }

    private:
        TextPosition _startPosition;
        TextPosition _endPosition;
        std::string _temporaryName;

    };
}

#endif //GULC_TEMPORARYVALUEREFEXPR_HPP
