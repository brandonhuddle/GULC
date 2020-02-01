#ifndef GULC_POSTFIXOPERATOREXPR_HPP
#define GULC_POSTFIXOPERATOREXPR_HPP

#include <ast/Expr.hpp>

namespace gulc {
    enum class PostfixOperators {
        Increment, // ++
        Decrement, // --
    };

    class PostfixOperatorExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::PostfixOperator; }

        Expr* nestedExpr;

        PostfixOperators postfixOperator() const { return _postfixOperator; }

        PostfixOperatorExpr(PostfixOperators postfixOperator, Expr* nestedExpr,
                            TextPosition operatorStartPosition, TextPosition operatorEndPosition)
                : Expr(Expr::Kind::PostfixOperator),
                  nestedExpr(nestedExpr), _operatorStartPosition(operatorStartPosition),
                  _operatorEndPosition(operatorEndPosition), _postfixOperator(postfixOperator) {}

        TextPosition startPosition() const override { return nestedExpr->startPosition(); }
        TextPosition endPosition() const override { return _operatorEndPosition; }

        ~PostfixOperatorExpr() override {
            delete nestedExpr;
        }

    protected:
        TextPosition _operatorStartPosition;
        TextPosition _operatorEndPosition;
        PostfixOperators _postfixOperator;

    };
}

#endif //GULC_POSTFIXOPERATOREXPR_HPP
