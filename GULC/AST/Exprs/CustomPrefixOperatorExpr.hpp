// Copyright (C) 2019 Michael Brandon Huddle
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef GULC_CUSTOMPREFIXOPERATOREXPR_HPP
#define GULC_CUSTOMPREFIXOPERATOREXPR_HPP

#include <AST/Expr.hpp>

namespace gulc {
    class CustomPrefixOperatorExpr : public Expr {
    public:
        static bool classof(const Expr *expr) { return expr->getExprKind() == Kind::CustomPrefixOperator; }

        CustomPrefixOperatorExpr(TextPosition startPosition, TextPosition endPosition,
                                 Expr* operatorReference, Expr* expr)
                : Expr(Kind::CustomPrefixOperator, startPosition, endPosition),
                  operatorReference(operatorReference), expr(expr) {}

        Expr* operatorReference;
        Expr* expr;

        Expr* deepCopy() const override {
            return new CustomPrefixOperatorExpr(startPosition(), endPosition(),
                                                operatorReference->deepCopy(), expr->deepCopy());
        }

        ~CustomPrefixOperatorExpr() override {
            delete operatorReference;
            delete expr;
        }

    };
}

#endif //GULC_CUSTOMPREFIXOPERATOREXPR_HPP
