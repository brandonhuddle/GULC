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
#ifndef GULC_MEMBERPROPERTYREFEXPR_HPP
#define GULC_MEMBERPROPERTYREFEXPR_HPP

#include <ast/exprs/PropertyRefExpr.hpp>
#include <ast/decls/PropertyDecl.hpp>

namespace gulc {
    class MemberPropertyRefExpr : public PropertyRefExpr {
    public:
        static bool classof(const Expr *expr) { return expr->getExprKind() == Kind::MemberPropertyRef; }

        // `self`, `obj`, `*somePtr`
        Expr* object;

        MemberPropertyRefExpr(TextPosition startPosition, TextPosition endPosition,
                              Expr* object, gulc::PropertyDecl* propertyDecl)
                : PropertyRefExpr(Expr::Kind::MemberPropertyRef, startPosition, endPosition, propertyDecl),
                  object(object) {}

        Expr* deepCopy() const override {
            auto result = new MemberPropertyRefExpr(_startPosition, _endPosition,
                                                    object->deepCopy(), propertyDecl);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return object->toString() + "." + propertyDecl->identifier().name();
        }

        ~MemberPropertyRefExpr() override {
            delete object;
        }

    };
}

#endif //GULC_MEMBERPROPERTYREFEXPR_HPP
