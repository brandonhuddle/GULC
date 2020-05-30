#ifndef GULC_ISEXPR_HPP
#define GULC_ISEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/Type.hpp>

namespace gulc {
    class IsExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::Is; }

        Expr* expr;
        Type* isType;

        IsExpr(Expr* expr, Type* isType, TextPosition isStartPosition, TextPosition isEndPosition)
                : Expr(Expr::Kind::Is),
                  expr(expr), isType(isType),
                  _isStartPosition(isStartPosition), _isEndPosition(isEndPosition) {}

        TextPosition startPosition() const override { return expr->startPosition(); }
        TextPosition endPosition() const override { return isType->endPosition(); }
        TextPosition isStartPosition() const { return _isStartPosition; }
        TextPosition isEndPosition() const { return _isEndPosition; }

        Expr* deepCopy() const override {
            auto result = new IsExpr(expr->deepCopy(), isType->deepCopy(),
                                     _isStartPosition, _isEndPosition);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return expr->toString() + " is " + isType->toString();
        }

        ~IsExpr() override {
            delete expr;
            delete isType;
        }

    protected:
        TextPosition _isStartPosition;
        TextPosition _isEndPosition;

    };
}

#endif //GULC_ISEXPR_HPP
