#ifndef GULC_IMPLICITCASTEXPR_HPP
#define GULC_IMPLICITCASTEXPR_HPP

#include <ast/Expr.hpp>

namespace gulc {
    /// Expression to represent an implicit cast. Implicit casts should be kept to a minimum within the language to
    /// prevent accidental loss of precision/data and prevent
    class ImplicitCastExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::ImplicitCast; }

        Expr* expr;
        Type* castToType;

        ImplicitCastExpr(Expr* expr, Type* castToType)
                : Expr(Expr::Kind::ImplicitCast),
                  expr(expr), castToType(castToType) {}

        TextPosition startPosition() const override { return expr->startPosition(); }
        TextPosition endPosition() const override { return castToType->endPosition(); }

        Expr* deepCopy() const override {
            auto result = new ImplicitCastExpr(expr->deepCopy(), castToType->deepCopy());
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return expr->toString();
        }

        ~ImplicitCastExpr() override {
            delete expr;
            delete castToType;
        }

    };
}

#endif //GULC_IMPLICITCASTEXPR_HPP
