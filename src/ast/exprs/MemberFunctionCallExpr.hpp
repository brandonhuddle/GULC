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
#ifndef GULC_MEMBERFUNCTIONCALLEXPR_HPP
#define GULC_MEMBERFUNCTIONCALLEXPR_HPP

#include "FunctionCallExpr.hpp"

namespace gulc {
    /// `MemberFunctionCallExpr` is the same as `FunctionCallExpr` EXCEPT it handles the `self` argument.
    class MemberFunctionCallExpr : public FunctionCallExpr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::MemberFunctionCall; }

        // This could be either `SelfRefExpr` OR an `Expr` that returns a struct type.
        Expr* selfArgument;

        MemberFunctionCallExpr(Expr* functionReference, Expr* selfArgument, std::vector<LabeledArgumentExpr*> arguments,
                               TextPosition startPosition, TextPosition endPosition)
                : FunctionCallExpr(Expr::Kind::MemberFunctionCall, functionReference, std::move(arguments),
                                   startPosition, endPosition),
                  selfArgument(selfArgument) {}

        Expr* deepCopy() const override {
            std::vector<LabeledArgumentExpr*> copiedArguments;
            copiedArguments.reserve(arguments.size());
            Expr* copiedSelfArgument = nullptr;

            for (Expr* argument : arguments) {
                copiedArguments.push_back(llvm::dyn_cast<LabeledArgumentExpr>(argument->deepCopy()));
            }

            if (selfArgument != nullptr) {
                copiedSelfArgument = selfArgument->deepCopy();
            }

            auto result = new MemberFunctionCallExpr(functionReference->deepCopy(),
                                                     copiedSelfArgument, copiedArguments,
                                                     _startPosition, _endPosition);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            std::string argumentsString;

            for (std::size_t i = 0; i < arguments.size(); ++i) {
                if (i != 0) argumentsString += ", ";

                argumentsString += arguments[i]->toString();
            }

            std::string selfString;

            if (selfArgument != nullptr) {
                selfString = selfArgument->toString() + ".";
            }

            return selfString + functionReference->toString() + "(" + argumentsString + ")";
        }

        ~MemberFunctionCallExpr() override {
            delete selfArgument;
        }

    };
}

#endif //GULC_MEMBERFUNCTIONCALLEXPR_HPP
