#ifndef GULC_TRYEXPR_HPP
#define GULC_TRYEXPR_HPP

#include <ast/Expr.hpp>

namespace gulc {
    class TryExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::Try; }

        Expr* nestedExpr;

        TryExpr(Expr* nestedExpr, TextPosition tryStartPosition, TextPosition tryEndPosition)
                : Expr(Expr::Kind::Try),
                  nestedExpr(nestedExpr), _tryStartPosition(tryStartPosition), _tryEndPosition(tryEndPosition) {}

        TextPosition startPosition() const override { return _tryStartPosition; }
        TextPosition endPosition() const override { return nestedExpr->endPosition(); }

        Expr* deepCopy() const override {
            auto result = new TryExpr(nestedExpr->deepCopy(),
                                      _tryStartPosition, _tryEndPosition);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return "try " + nestedExpr->toString();
        }

        ~TryExpr() override {
            delete nestedExpr;
        }

    protected:
        TextPosition _tryStartPosition;
        TextPosition _tryEndPosition;

    };
}

#endif //GULC_TRYEXPR_HPP
