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
#ifndef GULC_HASEXPR_HPP
#define GULC_HASEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/Type.hpp>
#include "IdentifierExpr.hpp"

namespace gulc {
    /**
     * Used to check if a type `has` a trait.
     *
     * Example:
     *     Ex has TAddable<Ex, Ex, Ex> // Ex::static operator infix +(left: in Ex, right: in Ex) -> Ex
     *     Ex has TDivisible<Ex, Ex, Ex> // Ex::static operator infix /(left: in Ex, right: in Ex) -> Ex
     *     Ex has TToString<Ex> // Ex::static func toString(ex: in Ex) -> Ex
     *     Ex has TAssignable<Ex> // Ex::static operator infix =(lvalue: inout Ex, right: in Ex)
     */
    class HasExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::Has; }

        Expr* expr;
        Type* trait;

        HasExpr(Expr* expr, Type* trait, TextPosition hasStartPosition, TextPosition hasEndPosition)
                : Expr(Expr::Kind::Has),
                  expr(expr), trait(trait),
                  _hasStartPosition(hasStartPosition), _hasEndPosition(hasEndPosition) {}

        TextPosition startPosition() const override { return expr->startPosition(); }
        TextPosition endPosition() const override { return trait->endPosition(); }
        TextPosition hasStartPosition() const { return _hasStartPosition; }
        TextPosition hasEndPosition() const { return _hasEndPosition; }

        Expr* deepCopy() const override {
            auto result = new HasExpr(expr->deepCopy(), trait->deepCopy(),
                                      _hasStartPosition, _hasEndPosition);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return expr->toString() + " has " + trait->toString();
        }

        ~HasExpr() override {
            delete expr;
            delete trait;
        }

    protected:
        TextPosition _hasStartPosition;
        TextPosition _hasEndPosition;

    };
}

#endif //GULC_HASEXPR_HPP
