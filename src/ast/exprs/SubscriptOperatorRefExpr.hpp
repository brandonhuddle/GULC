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
#ifndef GULC_SUBSCRIPTOPERATORREFEXPR_HPP
#define GULC_SUBSCRIPTOPERATORREFEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/decls/SubscriptOperatorDecl.hpp>
#include "LabeledArgumentExpr.hpp"

namespace gulc {
    /// Similar to `PropertyRefExpr` and function references. The big difference is we have to have valid arguments
    /// to have a valid subscript operator reference, the final `SubscriptOperatorGetCallExpr` or
    /// `SubscriptOperatorSetCalLExpr` won't contain the arguments. The arguments will always be contained within the
    /// reference to the actual operator declaration. This is a special case due to how we treat subscript operators
    ///
    /// NOTE: A `subscript` cannot appear outside of a `struct`, `trait`, etc. but it can be `static`. That is why we
    ///       don't have only `MemberSubscriptOperatorRefExpr`, we also have this `SubscriptOperatorRefExpr` for
    ///       static subscripts.
    class SubscriptOperatorRefExpr : public Expr {
    public:
        static bool classof(const Expr *expr) {
            auto exprKind = expr->getExprKind();

            return exprKind == Kind::SubscriptOperatorRef || exprKind == Kind::MemberSubscriptOperatorRef;
        }

        SubscriptOperatorDecl* subscriptOperatorDecl;
        std::vector<LabeledArgumentExpr*> arguments;
        bool hasArguments() const { return !arguments.empty(); }

        SubscriptOperatorRefExpr(TextPosition startPosition, TextPosition endPosition,
                                 SubscriptOperatorDecl* subscriptOperatorDecl,
                                 std::vector<LabeledArgumentExpr*> arguments)
                : SubscriptOperatorRefExpr(Expr::Kind::SubscriptOperatorRef, startPosition, endPosition,
                                           subscriptOperatorDecl, std::move(arguments)) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Expr* deepCopy() const override {
            std::vector<LabeledArgumentExpr*> copiedArguments;
            copiedArguments.resize(arguments.size());

            for (LabeledArgumentExpr* argument : arguments) {
                copiedArguments.push_back(llvm::dyn_cast<LabeledArgumentExpr>(argument->deepCopy()));
            }

            auto result = new SubscriptOperatorRefExpr(_startPosition, _endPosition,
                                                       subscriptOperatorDecl, copiedArguments);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            std::string argumentString = "[";

            for (std::size_t i = 0; i < arguments.size(); ++i) {
                if (i != 0) argumentString += ", ";

                argumentString += arguments[i]->toString();
            }

            argumentString += "]";

            return subscriptOperatorDecl->container->identifier().name() + argumentString;
        }

        ~SubscriptOperatorRefExpr() override {
            for (LabeledArgumentExpr* argument : arguments) {
                delete argument;
            }
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

        SubscriptOperatorRefExpr(Expr::Kind exprKind, TextPosition startPosition, TextPosition endPosition,
                                 SubscriptOperatorDecl* subscriptOperatorDecl,
                                 std::vector<LabeledArgumentExpr*> arguments)
                : Expr(exprKind),
                  _startPosition(startPosition), _endPosition(endPosition),
                  subscriptOperatorDecl(subscriptOperatorDecl), arguments(std::move(arguments)) {}

    };
}

#endif //GULC_SUBSCRIPTOPERATORREFEXPR_HPP
