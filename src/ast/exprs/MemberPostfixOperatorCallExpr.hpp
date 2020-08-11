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
#ifndef GULC_MEMBERPOSTFIXOPERATORCALLEXPR_HPP
#define GULC_MEMBERPOSTFIXOPERATORCALLEXPR_HPP

#include <ast/decls/OperatorDecl.hpp>
#include "PostfixOperatorExpr.hpp"

namespace gulc {
    class MemberPostfixOperatorCallExpr : public PostfixOperatorExpr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::MemberPostfixOperatorCall; }

        OperatorDecl* postfixOperatorDecl;

        MemberPostfixOperatorCallExpr(OperatorDecl* postfixOperatorDecl, PostfixOperators postfixOperator,
                                      Expr* nestedExpr, TextPosition operatorStartPosition,
                                      TextPosition operatorEndPosition)
                : PostfixOperatorExpr(Expr::Kind::MemberPostfixOperatorCall, postfixOperator, nestedExpr,
                                      operatorStartPosition, operatorEndPosition),
                  postfixOperatorDecl(postfixOperatorDecl) {}

        Expr* deepCopy() const override {
            auto result = new MemberPostfixOperatorCallExpr(postfixOperatorDecl, _postfixOperator,
                                                            nestedExpr->deepCopy(),
                                                            _operatorStartPosition,
                                                            _operatorEndPosition);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

    };
}

#endif //GULC_MEMBERPOSTFIXOPERATORCALLEXPR_HPP
