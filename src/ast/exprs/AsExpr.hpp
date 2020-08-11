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
#ifndef GULC_ASEXPR_HPP
#define GULC_ASEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/Type.hpp>

namespace gulc {
    /**
     * Expression to hold the `as` expressions. This might be converted to an explicit cast OR it might be overloaded
     * in some way.
     */
    class AsExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::As; }

        Expr* expr;
        Type* asType;

        AsExpr(Expr* expr, Type* asType, TextPosition asStartPosition, TextPosition asEndPosition)
                : Expr(Expr::Kind::As),
                  expr(expr), asType(asType),
                  _asStartPosition(asStartPosition), _asEndPosition(asEndPosition) {}

        TextPosition startPosition() const override { return expr->startPosition(); }
        TextPosition endPosition() const override { return asType->endPosition(); }
        TextPosition asStartPosition() const { return _asStartPosition; }
        TextPosition asEndPosition() const { return _asEndPosition; }

        Expr* deepCopy() const override {
            auto result = new AsExpr(expr->deepCopy(), asType->deepCopy(),
                                     _asStartPosition, _asEndPosition);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return expr->toString() + " as " + asType->toString();
        }

        ~AsExpr() override {
            delete expr;
            delete asType;
        }

    protected:
        TextPosition _asStartPosition;
        TextPosition _asEndPosition;

    };
}

#endif //GULC_ASEXPR_HPP
