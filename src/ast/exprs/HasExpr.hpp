#ifndef GULC_HASEXPR_HPP
#define GULC_HASEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/Type.hpp>
#include "IdentifierExpr.hpp"

namespace gulc {
    /**
     * Used to check if a type `has` a trait.
     *
     * Example:
     *     Ex has TAddable<Ex, Ex, Ex> // Ex::static operator infix +(left: in Ex, right: in Ex) -> Ex
     *     Ex has TDivisible<Ex, Ex, Ex> // Ex::static operator infix /(left: in Ex, right: in Ex) -> Ex
     *     Ex has TToString<Ex> // Ex::static func toString(ex: in Ex) -> Ex
     *     Ex has TAssignable<Ex> // Ex::static operator infix =(lvalue: inout Ex, right: in Ex)
     */
    class HasExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::Has; }

        Expr* expr;
        Type* trait;

        HasExpr(Expr* expr, Type* trait, TextPosition hasStartPosition, TextPosition hasEndPosition)
                : Expr(Expr::Kind::Has),
                  expr(expr), trait(trait),
                  _hasStartPosition(hasStartPosition), _hasEndPosition(hasEndPosition) {}

        TextPosition startPosition() const override { return expr->startPosition(); }
        TextPosition endPosition() const override { return trait->endPosition(); }
        TextPosition hasStartPosition() const { return _hasStartPosition; }
        TextPosition hasEndPosition() const { return _hasEndPosition; }

        ~HasExpr() override {
            delete expr;
            delete trait;
        }

    protected:
        TextPosition _hasStartPosition;
        TextPosition _hasEndPosition;

    };
}

#endif //GULC_HASEXPR_HPP
