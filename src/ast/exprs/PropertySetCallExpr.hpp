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
#ifndef GULC_PROPERTYSETCALLEXPR_HPP
#define GULC_PROPERTYSETCALLEXPR_HPP

#include <ast/Expr.hpp>
#include "PropertyRefExpr.hpp"

namespace gulc {
    // TODO: How are we going to handle `propVar += value`? It would have to be converted into
    //       `set(propVar, get(propVar) + value)`
    class PropertySetCallExpr : public Expr {
    public:
        static bool classof(const Expr *expr) { return expr->getExprKind() == Kind::PropertySetCall; }

        PropertyRefExpr* propertyReference;
        PropertySetDecl* propertySetter;
        Expr* value;

        PropertySetCallExpr(PropertyRefExpr* propertyReference, PropertySetDecl* propertySetter, Expr* value)
                : Expr(Expr::Kind::PropertySetCall),
                  propertyReference(propertyReference), propertySetter(propertySetter), value(value) {}

        TextPosition startPosition() const override { return propertyReference->startPosition(); }
        TextPosition endPosition() const override { return value->endPosition(); }

        Expr* deepCopy() const override {
            auto result = new PropertySetCallExpr(
                    llvm::dyn_cast<PropertyRefExpr>(propertyReference->deepCopy()),
                    propertySetter, value->deepCopy()
            );
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return propertyReference->toString() + " = " + value->toString();
        }

        ~PropertySetCallExpr() override {
            delete value;
            delete propertyReference;
        }

    };
}

#endif //GULC_PROPERTYSETCALLEXPR_HPP
