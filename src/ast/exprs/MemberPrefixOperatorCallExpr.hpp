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
#ifndef GULC_MEMBERPREFIXOPERATORCALLEXPR_HPP
#define GULC_MEMBERPREFIXOPERATORCALLEXPR_HPP

#include <ast/decls/OperatorDecl.hpp>
#include "PrefixOperatorExpr.hpp"

namespace gulc {
    class MemberPrefixOperatorCallExpr : public PrefixOperatorExpr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::MemberPrefixOperatorCall; }

        OperatorDecl* prefixOperatorDecl;

        MemberPrefixOperatorCallExpr(OperatorDecl* prefixOperatorDecl, PrefixOperators prefixOperator,
                                     Expr* nestedExpr,
                                     TextPosition operatorStartPosition, TextPosition operatorEndPosition)
                : PrefixOperatorExpr(Expr::Kind::MemberPrefixOperatorCall, prefixOperator, nestedExpr,
                                     operatorStartPosition, operatorEndPosition),
                  prefixOperatorDecl(prefixOperatorDecl) {}

        Expr* deepCopy() const override {
            auto result = new MemberPrefixOperatorCallExpr(prefixOperatorDecl, _prefixOperator,
                                                           nestedExpr->deepCopy(),
                                                           _operatorStartPosition,
                                                           _operatorEndPosition);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }
    };
}

#endif //GULC_MEMBERPREFIXOPERATORCALLEXPR_HPP
