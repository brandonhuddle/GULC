#ifndef GULC_LVALUETORVALUEEXPR_HPP
#define GULC_LVALUETORVALUEEXPR_HPP

#include <ast/Expr.hpp>

namespace gulc {
    /// Dereferences the implicit lvalue reference
    class LValueToRValueExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::LValueToRValue; }

        Expr* lvalue;

        explicit LValueToRValueExpr(Expr* lvalue)
                : Expr(Expr::Kind::LValueToRValue),
                  lvalue(lvalue) {}

        TextPosition startPosition() const override { return lvalue->startPosition(); }
        TextPosition endPosition() const override { return lvalue->endPosition(); }

        Expr* deepCopy() const override {
            auto result = new LValueToRValueExpr(lvalue->deepCopy());
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return lvalue->toString();
        }

        ~LValueToRValueExpr() override {
            delete lvalue;
        }

    };
}

#endif //GULC_LVALUETORVALUEEXPR_HPP
