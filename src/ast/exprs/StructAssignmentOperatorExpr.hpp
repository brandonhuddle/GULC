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
#ifndef GULC_STRUCTASSIGNMENTOPERATOREXPR_HPP
#define GULC_STRUCTASSIGNMENTOPERATOREXPR_HPP

#include <ast/exprs/AssignmentOperatorExpr.hpp>

namespace gulc {
    enum class StructAssignmentType {
        Move,
        Copy,
    };

    // Struct assignments are special and require their own operator to handle them correctly.
    // Struct assignments use `copy` or `move` constructors for assignment
    class StructAssignmentOperatorExpr : public AssignmentOperatorExpr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::StructAssignmentOperator; }

        StructAssignmentType structAssignmentType;

        StructAssignmentOperatorExpr(StructAssignmentType structAssignmentType, Expr* leftValue, Expr* rightValue,
                                     InfixOperators nestedOperator,
                                     TextPosition startPosition, TextPosition endPosition)
                : AssignmentOperatorExpr(Expr::Kind::StructAssignmentOperator, leftValue, rightValue,
                                         startPosition, endPosition,
                                         true, nestedOperator),
                  structAssignmentType(structAssignmentType) {}

        StructAssignmentOperatorExpr(StructAssignmentType structAssignmentType, Expr* leftValue, Expr* rightValue,
                                     TextPosition startPosition, TextPosition endPosition)
                : AssignmentOperatorExpr(Expr::Kind::StructAssignmentOperator, leftValue, rightValue,
                                         startPosition, endPosition,
                                         false, InfixOperators::Unknown),
                  structAssignmentType(structAssignmentType) {}

        Expr* deepCopy() const override {
            Expr* result;

            if (_hasNestedOperator) {
                result = new StructAssignmentOperatorExpr(structAssignmentType,
                                                          leftValue->deepCopy(), rightValue->deepCopy(),
                                                          _nestedOperator,
                                                          _startPosition, _endPosition);
            } else {
                result = new StructAssignmentOperatorExpr(structAssignmentType,
                                                          leftValue->deepCopy(), rightValue->deepCopy(),
                                                          _startPosition, _endPosition);
            }

            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

    };
}

#endif //GULC_STRUCTASSIGNMENTOPERATOREXPR_HPP
