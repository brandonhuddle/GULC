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
