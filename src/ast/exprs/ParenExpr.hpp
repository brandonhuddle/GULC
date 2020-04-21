#ifndef GULC_PARENEXPR_HPP
#define GULC_PARENEXPR_HPP

#include <ast/Expr.hpp>

namespace gulc {
    class ParenExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::Paren; }

        Expr* nestedExpr;

        ParenExpr(Expr* nestedExpr, TextPosition startPosition, TextPosition endPosition)
                : Expr(Expr::Kind::Paren),
                  nestedExpr(nestedExpr), _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Expr* deepCopy() const override {
            return new ParenExpr(nestedExpr->deepCopy(), _startPosition, _endPosition);
        }

        ~ParenExpr() override {
            delete nestedExpr;
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_PARENEXPR_HPP
