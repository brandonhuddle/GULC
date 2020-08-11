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
#ifndef GULC_POSTFIXOPERATOREXPR_HPP
#define GULC_POSTFIXOPERATOREXPR_HPP

#include <ast/Expr.hpp>

namespace gulc {
    enum class PostfixOperators {
        Increment, // ++
        Decrement, // --
    };

    inline std::string getPostfixOperatorString(PostfixOperators postfixOperator) {
        switch (postfixOperator) {
            case PostfixOperators::Increment:
                return "++";
            case PostfixOperators::Decrement:
                return "--";
            default:
                return "[UNKNOWN]";
        }
    }

    class PostfixOperatorExpr : public Expr {
    public:
        static bool classof(const Expr* expr) {
            auto checkKind = expr->getExprKind();
            return checkKind == Expr::Kind::PostfixOperator || checkKind == Expr::Kind::MemberPostfixOperatorCall;
        }

        Expr* nestedExpr;

        PostfixOperators postfixOperator() const { return _postfixOperator; }

        PostfixOperatorExpr(PostfixOperators postfixOperator, Expr* nestedExpr,
                            TextPosition operatorStartPosition, TextPosition operatorEndPosition)
                : PostfixOperatorExpr(Expr::Kind::PostfixOperator, postfixOperator, nestedExpr,
                                      operatorStartPosition, operatorEndPosition) {}

        TextPosition startPosition() const override { return nestedExpr->startPosition(); }
        TextPosition endPosition() const override { return _operatorEndPosition; }

        Expr* deepCopy() const override {
            auto result = new PostfixOperatorExpr(_postfixOperator, nestedExpr->deepCopy(),
                                                  _operatorStartPosition, _operatorEndPosition);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return nestedExpr->toString() + getPostfixOperatorString(_postfixOperator);
        }

        ~PostfixOperatorExpr() override {
            delete nestedExpr;
        }

    protected:
        TextPosition _operatorStartPosition;
        TextPosition _operatorEndPosition;
        PostfixOperators _postfixOperator;

        PostfixOperatorExpr(Expr::Kind exprKind, PostfixOperators postfixOperator, Expr* nestedExpr,
                            TextPosition operatorStartPosition, TextPosition operatorEndPosition)
                : Expr(exprKind),
                  nestedExpr(nestedExpr), _operatorStartPosition(operatorStartPosition),
                  _operatorEndPosition(operatorEndPosition), _postfixOperator(postfixOperator) {}

    };
}

#endif //GULC_POSTFIXOPERATOREXPR_HPP
