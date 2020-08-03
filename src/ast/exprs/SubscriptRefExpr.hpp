#ifndef GULC_SUBSCRIPTREFEXPR_HPP
#define GULC_SUBSCRIPTREFEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/decls/SubscriptOperatorDecl.hpp>

namespace gulc {
    class SubscriptRefExpr : public Expr {
    public:
        static bool classof(const Expr *expr) { return expr->getExprKind() == Kind::SubscriptRef; }

        SubscriptRefExpr(TextPosition startPosition, TextPosition endPosition,
                         SubscriptOperatorDecl* subscriptOperatorDecl)
                : Expr(Expr::Kind::SubscriptRef),
                  _startPosition(startPosition), _endPosition(endPosition),
                  _subscriptOperatorDecl(subscriptOperatorDecl) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        SubscriptOperatorDecl* subscriptOperatorDecl() const { return _subscriptOperatorDecl; }

        Expr* deepCopy() const override {
            auto result = new SubscriptRefExpr(_startPosition, _endPosition,
                                               _subscriptOperatorDecl);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return _subscriptOperatorDecl->identifier().name();
        }

    private:
        TextPosition _startPosition;
        TextPosition _endPosition;
        // NOTE: We don't own this so we don't free it.
        SubscriptOperatorDecl* _subscriptOperatorDecl;

    };
}

#endif //GULC_SUBSCRIPTREFEXPR_HPP
