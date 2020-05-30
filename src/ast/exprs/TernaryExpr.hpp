#ifndef GULC_TERNARYEXPR_HPP
#define GULC_TERNARYEXPR_HPP

#include <ast/Expr.hpp>

namespace gulc {
    class TernaryExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::Ternary; }

        Expr* condition;
        Expr* trueExpr;
        Expr* falseExpr;

        TernaryExpr(Expr* condition, Expr* trueExpr, Expr* falseExpr)
                : Expr(Expr::Kind::Ternary),
                  condition(condition), trueExpr(trueExpr), falseExpr(falseExpr) {}

        TextPosition startPosition() const override { return condition->startPosition(); }
        TextPosition endPosition() const override { return falseExpr->endPosition(); }

        Expr* deepCopy() const override {
            auto result = new TernaryExpr(condition->deepCopy(), trueExpr->deepCopy(), falseExpr->deepCopy());
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return condition->toString() + " ? " + trueExpr->toString() + " : " + falseExpr->toString();
        }

        ~TernaryExpr() override {
            delete condition;
            delete trueExpr;
            delete falseExpr;
        }

    };
}

#endif //GULC_TERNARYEXPR_HPP
