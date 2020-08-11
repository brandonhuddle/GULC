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
#ifndef GULC_PREFIXOPERATOREXPR_HPP
#define GULC_PREFIXOPERATOREXPR_HPP

#include <ast/Expr.hpp>

namespace gulc {
    enum class PrefixOperators {
        Increment, // ++
        Decrement, // --

        Positive, // +
        Negative, // -

        LogicalNot, // !
        BitwiseNot, // ~

        Dereference, // *
        Reference, // &

        // Get the size of any type
        SizeOf, // sizeof
        // Get the alignment of any type OR member
        AlignOf, // alignof
        // Get the offset of an member in a struct/class
        OffsetOf, // offsetof
        // Get the name of any decl (e.g. `nameof(std.io.Stream) == "Stream"`, `nameof(ExampleClass::Member) == "Member")
        // This is useful for making refactoring easier. Sometimes you need to know the name of something but if you
        // use the exact string value of the name it might not get recognized in refactoring. Making getting the name a
        // part of the actual language will trigger an error if the decl within the `nameof` wasn't found.
        NameOf, // nameof
        // Get known traits of a type
        TraitsOf, // traitsof
    };

    inline std::string getPrefixOperatorString(PrefixOperators prefixOperator) {
        switch (prefixOperator) {
            case PrefixOperators::Increment:
                return "++";
            case PrefixOperators::Decrement:
                return "--";
            case PrefixOperators::Positive:
                return "+";
            case PrefixOperators::Negative:
                return "-";
            case PrefixOperators::LogicalNot:
                return "!";
            case PrefixOperators::BitwiseNot:
                return "~";
            case PrefixOperators::Dereference:
                return "*";
            case PrefixOperators::Reference:
                return "&";
            case PrefixOperators::SizeOf:
                return "sizeof";
            case PrefixOperators::AlignOf:
                return "alignof";
            case PrefixOperators::OffsetOf:
                return "offsetof";
            case PrefixOperators::NameOf:
                return "nameof";
            case PrefixOperators::TraitsOf:
                return "traitsof";
            default:
                return "[UNKNOWN]";
        }
    }

    class PrefixOperatorExpr : public Expr {
    public:
        static bool classof(const Expr* expr) {
            auto exprKind = expr->getExprKind();
            return exprKind == Expr::Kind::PrefixOperator || exprKind == Expr::Kind::MemberPrefixOperatorCall;
        }

        Expr* nestedExpr;

        PrefixOperators prefixOperator() const { return _prefixOperator; }

        PrefixOperatorExpr(PrefixOperators prefixOperator, Expr* nestedExpr,
                           TextPosition operatorStartPosition, TextPosition operatorEndPosition)
                : PrefixOperatorExpr(Expr::Kind::PrefixOperator, prefixOperator, nestedExpr,
                                     operatorStartPosition, operatorEndPosition) {}

        TextPosition startPosition() const override { return _operatorStartPosition; }
        TextPosition endPosition() const override { return nestedExpr->endPosition(); }

        Expr* deepCopy() const override {
            auto result = new PrefixOperatorExpr(_prefixOperator, nestedExpr->deepCopy(),
                                                 _operatorStartPosition, _operatorEndPosition);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return getPrefixOperatorString(_prefixOperator) + "(" + nestedExpr->toString() + ")";
        }

        ~PrefixOperatorExpr() override {
            delete nestedExpr;
        }

    protected:
        TextPosition _operatorStartPosition;
        TextPosition _operatorEndPosition;
        PrefixOperators _prefixOperator;

        PrefixOperatorExpr(Expr::Kind exprKind, PrefixOperators prefixOperator, Expr* nestedExpr,
                           TextPosition operatorStartPosition, TextPosition operatorEndPosition)
                : Expr(exprKind),
                  nestedExpr(nestedExpr), _operatorStartPosition(operatorStartPosition),
                  _operatorEndPosition(operatorEndPosition), _prefixOperator(prefixOperator) {}

    };
}

#endif //GULC_PREFIXOPERATOREXPR_HPP
