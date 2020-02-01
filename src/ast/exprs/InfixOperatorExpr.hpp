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
    };

    class InfixOperatorExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::InfixOperator; }

        Expr* leftValue;
        Expr* rightValue;

        InfixOperators infixOperator() const { return _infixOperator; }

        InfixOperatorExpr(InfixOperators infixOperator, Expr* leftValue, Expr* rightValue)
                : Expr(Expr::Kind::InfixOperator),
                  leftValue(leftValue), rightValue(rightValue), _infixOperator(infixOperator) {}

        TextPosition startPosition() const override { return leftValue->startPosition(); }
        TextPosition endPosition() const override { return rightValue->endPosition(); }

        ~InfixOperatorExpr() override {
            delete leftValue;
            delete rightValue;
        }

    protected:
        InfixOperators _infixOperator;

    };
}

#endif //GULC_INFIXOPERATOREXPR_HPP
