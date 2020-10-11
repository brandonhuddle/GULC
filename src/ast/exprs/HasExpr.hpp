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
#include <ast/Decl.hpp>
#include "IdentifierExpr.hpp"

namespace gulc {
    // TODO: Support:
    //           T has init(x: i32, y: i32)
    //           T has func sum(_: []item) -> 12
    //           T has deinit
    //           T has const var identity: T
    //           T has trait Addable
    //           T has var example: i32
    //           T has prop test: f32 { get; get ref; set }

    /**
     * Used to check if a type `has` a decl or implements trait
     */
    class HasExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::Has; }

        Expr* expr;
        Decl* decl;

        HasExpr(Expr* expr, Decl* decl, TextPosition hasStartPosition, TextPosition hasEndPosition)
                : Expr(Expr::Kind::Has),
                  expr(expr), decl(decl),
                  _hasStartPosition(hasStartPosition), _hasEndPosition(hasEndPosition) {}

        TextPosition startPosition() const override { return expr->startPosition(); }
        TextPosition endPosition() const override { return decl->endPosition(); }
        TextPosition hasStartPosition() const { return _hasStartPosition; }
        TextPosition hasEndPosition() const { return _hasEndPosition; }

        Expr* deepCopy() const override {
            auto result = new HasExpr(expr->deepCopy(), decl->deepCopy(),
                                      _hasStartPosition, _hasEndPosition);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return expr->toString() + " has " + decl->getPrototypeString();
        }

        ~HasExpr() override {
            delete expr;
            delete decl;
        }

    protected:
        TextPosition _hasStartPosition;
        TextPosition _hasEndPosition;

    };
}

#endif //GULC_HASEXPR_HPP
