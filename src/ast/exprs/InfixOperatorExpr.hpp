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
#ifndef GULC_INFIXOPERATOREXPR_HPP
#define GULC_INFIXOPERATOREXPR_HPP

#include <ast/Expr.hpp>

namespace gulc {
    enum class InfixOperators {
        Unknown,
        Add, // +
        Subtract, // -
        Multiply, // *
        Divide, // /
        Remainder, // %
        Power, // ^^ (exponents)

        BitwiseAnd, // &
        BitwiseOr, // |
        BitwiseXor, // ^

        BitshiftLeft, // << (logical shift left)
        BitshiftRight, // >> (logical shift right OR arithmetic shift right, depending on the type)

        LogicalAnd, // &&
        LogicalOr, // ||

        EqualTo, // ==
        NotEqualTo, // !=

        GreaterThan, // >
        LessThan, // <
        GreaterThanEqualTo, // >=
        LessThanEqualTo, // <=

        // TODO: Implement this
        Spaceship, // <=>
    };

    inline std::string getInfixOperatorStringValue(InfixOperators infixOperator) {
        switch (infixOperator) {
            default:
                return "[UNKNOWN]";
            case InfixOperators::Add:
                return "+";
            case InfixOperators::Subtract:
                return "-";
            case InfixOperators::Multiply:
                return "*";
            case InfixOperators::Divide:
                return "/";
            case InfixOperators::Remainder:
                return "%";
            case InfixOperators::Power:
                return "^^";
            case InfixOperators::BitwiseAnd:
                return "&";
            case InfixOperators::BitwiseOr:
                return "|";
            case InfixOperators::BitwiseXor:
                return "^";
            case InfixOperators::BitshiftLeft:
                return "<<";
            case InfixOperators::BitshiftRight:
                return ">>";
            case InfixOperators::LogicalAnd:
                return "&&";
            case InfixOperators::LogicalOr:
                return "||";
            case InfixOperators::EqualTo:
                return "==";
            case InfixOperators::NotEqualTo:
                return "!=";
            case InfixOperators::GreaterThan:
                return ">";
            case InfixOperators::LessThan:
                return "<";
            case InfixOperators::GreaterThanEqualTo:
                return ">=";
            case InfixOperators::LessThanEqualTo:
                return "<=";
        }
    }

    class InfixOperatorExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::InfixOperator; }

        Expr* leftValue;
        Expr* rightValue;

        InfixOperators infixOperator() const { return _infixOperator; }

        InfixOperatorExpr(InfixOperators infixOperator, Expr* leftValue, Expr* rightValue)
                : InfixOperatorExpr(Expr::Kind::InfixOperator, infixOperator, leftValue, rightValue) {}

        TextPosition startPosition() const override { return leftValue->startPosition(); }
        TextPosition endPosition() const override { return rightValue->endPosition(); }

        Expr* deepCopy() const override {
            auto result = new InfixOperatorExpr(_infixOperator, leftValue->deepCopy(), rightValue->deepCopy());
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return leftValue->toString() + " " + getInfixOperatorStringValue(_infixOperator) + " " + rightValue->toString();
        }

        ~InfixOperatorExpr() override {
            delete leftValue;
            delete rightValue;
        }

    protected:
        InfixOperators _infixOperator;

        InfixOperatorExpr(Expr::Kind exprKind, InfixOperators infixOperator, Expr* leftValue, Expr* rightValue)
                : Expr(exprKind),
                  leftValue(leftValue), rightValue(rightValue), _infixOperator(infixOperator) {}

    };
}

#endif //GULC_INFIXOPERATOREXPR_HPP
