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
#ifndef GULC_PROPERTYGETCALLEXPR_HPP
#define GULC_PROPERTYGETCALLEXPR_HPP

#include <ast/Expr.hpp>
#include "PropertyRefExpr.hpp"

namespace gulc {
    class PropertyGetCallExpr : public Expr {
    public:
        static bool classof(const Expr *expr) { return expr->getExprKind() == Kind::PropertyGetCall; }

        PropertyRefExpr* propertyReference;
        PropertyGetDecl* propertyGetter;

        PropertyGetCallExpr(PropertyRefExpr* propertyReference, PropertyGetDecl* propertyGetter)
                : Expr(Expr::Kind::PropertyGetCall),
                  propertyReference(propertyReference), propertyGetter(propertyGetter) {}

        TextPosition startPosition() const override { return propertyReference->startPosition(); }
        TextPosition endPosition() const override { return propertyReference->endPosition(); }

        Expr* deepCopy() const override {
            auto result = new PropertyGetCallExpr(
                    llvm::dyn_cast<PropertyRefExpr>(propertyReference->deepCopy()),
                    propertyGetter
            );
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return propertyReference->toString();
        }

        ~PropertyGetCallExpr() override {
            delete propertyReference;
        }

    };
}

#endif //GULC_PROPERTYGETCALLEXPR_HPP
