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
#ifndef GULC_SUBSCRIPTOPERATORGETCALLEXPR_HPP
#define GULC_SUBSCRIPTOPERATORGETCALLEXPR_HPP

#include <ast/Expr.hpp>
#include "SubscriptOperatorRefExpr.hpp"

namespace gulc {
    class SubscriptOperatorGetCallExpr : public Expr {
    public:
        static bool classof(const Expr *expr) { return expr->getExprKind() == Kind::SubscriptOperatorGetCall; }

        SubscriptOperatorRefExpr* subscriptOperatorReference;
        SubscriptOperatorGetDecl* subscriptOperatorGetter;

        SubscriptOperatorGetCallExpr(SubscriptOperatorRefExpr* subscriptOperatorReference,
                                     SubscriptOperatorGetDecl* subscriptOperatorGetter)
                : Expr(Expr::Kind::SubscriptOperatorGetCall),
                  subscriptOperatorReference(subscriptOperatorReference),
                  subscriptOperatorGetter(subscriptOperatorGetter) {}

        TextPosition startPosition() const override { return subscriptOperatorReference->startPosition(); }
        TextPosition endPosition() const override { return subscriptOperatorReference->endPosition(); }

        Expr* deepCopy() const override {
            auto result = new SubscriptOperatorGetCallExpr(
                    llvm::dyn_cast<SubscriptOperatorRefExpr>(subscriptOperatorReference->deepCopy()),
                    subscriptOperatorGetter
            );
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return subscriptOperatorReference->toString();
        }

        ~SubscriptOperatorGetCallExpr() override {
            delete subscriptOperatorReference;
        }

    };
}

#endif //GULC_SUBSCRIPTOPERATORGETCALLEXPR_HPP
