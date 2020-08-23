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
#ifndef GULC_MEMBERSUBSCRIPTOPERATORREFEXPR_HPP
#define GULC_MEMBERSUBSCRIPTOPERATORREFEXPR_HPP

#include "SubscriptOperatorRefExpr.hpp"

namespace gulc {
    class MemberSubscriptOperatorRefExpr : public SubscriptOperatorRefExpr {
    public:
        static bool classof(const Expr *expr) { return expr->getExprKind() == Kind::MemberSubscriptOperatorRef; }

        // `self`, `obj`, `*somePtr`
        Expr* object;

        MemberSubscriptOperatorRefExpr(TextPosition startPosition, TextPosition endPosition,
                                       SubscriptOperatorDecl* subscriptOperatorDecl,
                                       Expr* object, std::vector<LabeledArgumentExpr*> arguments)
                : SubscriptOperatorRefExpr(Expr::Kind::MemberSubscriptOperatorRef, startPosition, endPosition,
                                           subscriptOperatorDecl, std::move(arguments)),
                  object(object) {}

        Expr* deepCopy() const override {
            std::vector<LabeledArgumentExpr*> copiedArguments;
            copiedArguments.resize(arguments.size());

            for (LabeledArgumentExpr* argument : arguments) {
                copiedArguments.push_back(llvm::dyn_cast<LabeledArgumentExpr>(argument->deepCopy()));
            }

            auto result = new MemberSubscriptOperatorRefExpr(_startPosition, _endPosition,
                                                             subscriptOperatorDecl, object->deepCopy(),
                                                             copiedArguments);
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

            return object->toString() + argumentString;
        }

        ~MemberSubscriptOperatorRefExpr() override {
            delete object;
        }

    };
}

#endif //GULC_MEMBERSUBSCRIPTOPERATORREFEXPR_HPP
