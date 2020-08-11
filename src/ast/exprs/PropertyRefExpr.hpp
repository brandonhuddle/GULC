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
#ifndef GULC_PROPERTYREFEXPR_HPP
#define GULC_PROPERTYREFEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/decls/PropertyDecl.hpp>

namespace gulc {
    /// Same as a function reference in essence. It is NOT the final `PropertyGetCall` or `PropertySetCall` but the
    /// expression that points `PropertyGetCall` and `PropertySetCall` to the underlying `Property`
    class PropertyRefExpr : public Expr {
    public:
        static bool classof(const Expr *expr) { return expr->getExprKind() == Kind::PropertyRef; }

        PropertyDecl* propertyDecl;

        PropertyRefExpr(TextPosition startPosition, TextPosition endPosition,
                        PropertyDecl* propertyDecl)
                : Expr(Expr::Kind::PropertyRef),
                  _startPosition(startPosition), _endPosition(endPosition),
                  propertyDecl(propertyDecl) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Expr* deepCopy() const override {
            auto result = new PropertyRefExpr(_startPosition, _endPosition,
                                              propertyDecl);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return propertyDecl->identifier().name();
        }

    private:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_PROPERTYREFEXPR_HPP
