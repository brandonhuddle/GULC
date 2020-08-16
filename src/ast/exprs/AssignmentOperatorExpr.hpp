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
#ifndef GULC_ASSIGNMENTOPERATOREXPR_HPP
#define GULC_ASSIGNMENTOPERATOREXPR_HPP

#include <ast/Expr.hpp>
#include <string>
#include "InfixOperatorExpr.hpp"

namespace gulc {
    class AssignmentOperatorExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::AssignmentOperator; }

        Expr* leftValue;
        Expr* rightValue;

        bool hasNestedOperator() const { return _hasNestedOperator; }
        InfixOperators nestedOperator() const { return _nestedOperator; }

        AssignmentOperatorExpr(Expr* leftValue, Expr* rightValue, InfixOperators nestedOperator,
                               TextPosition startPosition, TextPosition endPosition)
                : AssignmentOperatorExpr(Expr::Kind::AssignmentOperator, leftValue, rightValue,
                                         startPosition, endPosition,
                                         true, nestedOperator) {}

        AssignmentOperatorExpr(Expr* leftValue, Expr* rightValue,
                               TextPosition startPosition, TextPosition endPosition)
                : AssignmentOperatorExpr(Expr::Kind::AssignmentOperator, leftValue, rightValue,
                                         startPosition, endPosition,
                                         false, InfixOperators::Unknown) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Expr* deepCopy() const override {
            Expr* result;

            if (_hasNestedOperator) {
                result = new AssignmentOperatorExpr(leftValue->deepCopy(), rightValue->deepCopy(),
                                                    _nestedOperator, _startPosition, _endPosition);
            } else {
                result = new AssignmentOperatorExpr(leftValue->deepCopy(), rightValue->deepCopy(),
                                                    _startPosition, _endPosition);
            }

            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            if (hasNestedOperator()) {
                return leftValue->toString() + " " + getInfixOperatorStringValue(_nestedOperator) + "= " +
                       rightValue->toString();
            } else {
                return leftValue->toString() + " " + getInfixOperatorStringValue(_nestedOperator) + " " +
                       rightValue->toString();
            }
        }

        ~AssignmentOperatorExpr() override {
            delete leftValue;
            delete rightValue;
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;
        bool _hasNestedOperator;
        InfixOperators _nestedOperator;

        AssignmentOperatorExpr(Expr::Kind exprKind, Expr* leftValue, Expr* rightValue,
                               TextPosition startPosition, TextPosition endPosition,
                               bool hasNestedOperator, InfixOperators nestedOperator)
                : Expr(exprKind),
                  leftValue(leftValue), rightValue(rightValue), _startPosition(startPosition),
                  _endPosition(endPosition), _hasNestedOperator(hasNestedOperator), _nestedOperator(nestedOperator) {}

    };
}

#endif //GULC_ASSIGNMENTOPERATOREXPR_HPP
