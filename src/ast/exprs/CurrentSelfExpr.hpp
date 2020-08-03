#ifndef GULC_CURRENTSELFEXPR_HPP
#define GULC_CURRENTSELFEXPR_HPP

#include <ast/Expr.hpp>

namespace gulc {
    /// Expression to store the use of `self`, this makes working with the AST just slightly nicer over an explicit
    /// parameter 0 index
    class CurrentSelfExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::CurrentSelf; }

        CurrentSelfExpr(TextPosition startPosition, TextPosition endPosition)
                : Expr(Expr::Kind::CurrentSelf),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Expr* deepCopy() const override {
            auto result = new CurrentSelfExpr(_startPosition, _endPosition);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return "self";
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_CURRENTSELFEXPR_HPP
