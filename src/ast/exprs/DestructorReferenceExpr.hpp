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
#ifndef GULC_DESTRUCTORREFERENCEEXPR_HPP
#define GULC_DESTRUCTORREFERENCEEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/decls/DestructorDecl.hpp>

namespace gulc {
    class DestructorReferenceExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::DestructorReference; }

        DestructorDecl* destructor;

        DestructorReferenceExpr(TextPosition startPosition, TextPosition endPosition, DestructorDecl* destructor)
                : Expr(Expr::Kind::DestructorReference),
                  destructor(destructor), _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Expr* deepCopy() const override {
            auto result = new DestructorReferenceExpr(_startPosition, _endPosition, destructor);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return destructor->container->identifier().name() + "::deinit";
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_DESTRUCTORREFERENCEEXPR_HPP
