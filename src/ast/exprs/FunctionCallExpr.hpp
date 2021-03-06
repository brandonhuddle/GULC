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
#ifndef GULC_FUNCTIONCALLEXPR_HPP
#define GULC_FUNCTIONCALLEXPR_HPP

#include <ast/Expr.hpp>
#include <vector>
#include <llvm/Support/Casting.h>
#include "LabeledArgumentExpr.hpp"

namespace gulc {
    class FunctionCallExpr : public Expr {
    public:
        static bool classof(const Expr* expr) {
            auto kind = expr->getExprKind();

            return kind == Expr::Kind::FunctionCall || kind == Expr::Kind::MemberFunctionCall;
        }

        Expr* functionReference;
        std::vector<LabeledArgumentExpr*> arguments;
        bool hasArguments() const { return !arguments.empty(); }

        FunctionCallExpr(Expr* functionReference, std::vector<LabeledArgumentExpr*> arguments,
                         TextPosition startPosition, TextPosition endPosition)
                : FunctionCallExpr(Expr::Kind::FunctionCall, functionReference, std::move(arguments),
                                   startPosition, endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Expr* deepCopy() const override {
            std::vector<LabeledArgumentExpr*> copiedArguments;
            copiedArguments.reserve(arguments.size());

            for (Expr* argument : arguments) {
                copiedArguments.push_back(llvm::dyn_cast<LabeledArgumentExpr>(argument->deepCopy()));
            }

            auto result = new FunctionCallExpr(functionReference->deepCopy(), copiedArguments,
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

            return functionReference->toString() + "(" + argumentsString + ")";
        }

        ~FunctionCallExpr() override {
            for (LabeledArgumentExpr* argument : arguments) {
                delete argument;
            }

            delete functionReference;
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

        FunctionCallExpr(Expr::Kind kind, Expr* functionReference, std::vector<LabeledArgumentExpr*> arguments,
                         TextPosition startPosition, TextPosition endPosition)
                : Expr(kind),
                  functionReference(functionReference), arguments(std::move(arguments)),
                  _startPosition(startPosition), _endPosition(endPosition) {}

    };
}

#endif //GULC_FUNCTIONCALLEXPR_HPP
