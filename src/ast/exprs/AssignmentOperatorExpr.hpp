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
                : Expr(Expr::Kind::AssignmentOperator),
                  leftValue(leftValue), rightValue(rightValue), _startPosition(startPosition),
                  _endPosition(endPosition), _hasNestedOperator(true), _nestedOperator(nestedOperator) {}

        AssignmentOperatorExpr(Expr* leftValue, Expr* rightValue,
                               TextPosition startPosition, TextPosition endPosition)
                : Expr(Expr::Kind::AssignmentOperator),
                  leftValue(leftValue), rightValue(rightValue), _startPosition(startPosition),
                  _endPosition(endPosition), _hasNestedOperator(false), _nestedOperator(InfixOperators::Unknown) {}

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

    };
}

#endif //GULC_ASSIGNMENTOPERATOREXPR_HPP
