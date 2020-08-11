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
#ifndef GULC_CONSTRUCTORCALLEXPR_HPP
#define GULC_CONSTRUCTORCALLEXPR_HPP

#include <vector>
#include <ast/exprs/FunctionCallExpr.hpp>
#include <llvm/Support/Casting.h>
#include <ast/decls/ConstructorDecl.hpp>
#include "LabeledArgumentExpr.hpp"
#include "ConstructorReferenceExpr.hpp"

namespace gulc {
    class ConstructorCallExpr : public FunctionCallExpr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::ConstructorCall; }

        ConstructorCallExpr(ConstructorReferenceExpr* constructorReference, std::vector<LabeledArgumentExpr*> arguments,
                            TextPosition startPosition, TextPosition endPosition)
                : FunctionCallExpr(Expr::Kind::ConstructorCall, constructorReference, std::move(arguments),
                                   startPosition, endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Expr* deepCopy() const override {
            std::vector<LabeledArgumentExpr*> copiedArguments;
            copiedArguments.reserve(arguments.size());
            ConstructorReferenceExpr* copiedConstructorReference = nullptr;

            for (Expr* argument : arguments) {
                copiedArguments.push_back(llvm::dyn_cast<LabeledArgumentExpr>(argument->deepCopy()));
            }

            if (functionReference != nullptr && llvm::isa<ConstructorReferenceExpr>(functionReference)) {
                copiedConstructorReference = llvm::dyn_cast<ConstructorReferenceExpr>(functionReference->deepCopy());
            }

            auto result = new ConstructorCallExpr(copiedConstructorReference, copiedArguments,
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

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_CONSTRUCTORCALLEXPR_HPP
