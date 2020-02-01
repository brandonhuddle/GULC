#ifndef GULC_TYPEEXPR_HPP
#define GULC_TYPEEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/Type.hpp>

namespace gulc {
    /**
     * `Expr` container for a `Type`, this allows `Type`s to be found anywhere an expression is expected.
     */
    class TypeExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::Type; }

        Type* type;

        explicit TypeExpr(Type* type)
                : Expr(Expr::Kind::Type), type(type) {}

        TextPosition startPosition() const override { return type->startPosition(); }
        TextPosition endPosition() const override { return type->endPosition(); }

        ~TypeExpr() override {
            delete type;
        }

    };
}

#endif //GULC_TYPEEXPR_HPP
