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
#ifndef GULC_CONSTRUCTORREFERENCEEXPR_HPP
#define GULC_CONSTRUCTORREFERENCEEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/decls/ConstructorDecl.hpp>

namespace gulc {
    class ConstructorReferenceExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::ConstructorReference; }

        ConstructorDecl* constructor;

        ConstructorReferenceExpr(TextPosition startPosition, TextPosition endPosition, ConstructorDecl* constructor)
                : Expr(Expr::Kind::ConstructorReference),
                  constructor(constructor), _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Expr* deepCopy() const override {
            auto result = new ConstructorReferenceExpr(_startPosition, _endPosition, constructor);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return constructor->container->identifier().name();
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_CONSTRUCTORREFERENCEEXPR_HPP
