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
#ifndef GULC_ISEXPR_HPP
#define GULC_ISEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/Type.hpp>

namespace gulc {
    class IsExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::Is; }

        Expr* expr;
        Type* isType;

        IsExpr(Expr* expr, Type* isType, TextPosition isStartPosition, TextPosition isEndPosition)
                : Expr(Expr::Kind::Is),
                  expr(expr), isType(isType),
                  _isStartPosition(isStartPosition), _isEndPosition(isEndPosition) {}

        TextPosition startPosition() const override { return expr->startPosition(); }
        TextPosition endPosition() const override { return isType->endPosition(); }
        TextPosition isStartPosition() const { return _isStartPosition; }
        TextPosition isEndPosition() const { return _isEndPosition; }

        Expr* deepCopy() const override {
            auto result = new IsExpr(expr->deepCopy(), isType->deepCopy(),
                                     _isStartPosition, _isEndPosition);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return expr->toString() + " is " + isType->toString();
        }

        ~IsExpr() override {
            delete expr;
            delete isType;
        }

    protected:
        TextPosition _isStartPosition;
        TextPosition _isEndPosition;

    };
}

#endif //GULC_ISEXPR_HPP
