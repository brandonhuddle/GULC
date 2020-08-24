#ifndef GULC_RVALUETOINREFEXPR_HPP
#define GULC_RVALUETOINREFEXPR_HPP

#include <ast/Expr.hpp>

namespace gulc {
    /// Converts an rvalue into a `in` reference. This is used only when the expression is not a reference and not an
    /// lvalue. In that situation we add an `RValueToInRefExpr` to tell the code generator to store the rvalue in a way
    /// that we can get an `in` reference to it.
    class RValueToInRefExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::RValueToInRef; }

        Expr* rvalue;

        explicit RValueToInRefExpr(Expr* rvalue)
                : Expr(Expr::Kind::RValueToInRef),
                  rvalue(rvalue) {}

        TextPosition startPosition() const override { return rvalue->startPosition(); }
        TextPosition endPosition() const override { return rvalue->endPosition(); }

        Expr* deepCopy() const override {
            auto result = new RValueToInRefExpr(rvalue->deepCopy());
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return rvalue->toString();
        }

        ~RValueToInRefExpr() override {
            delete rvalue;
        }

    };
}

#endif //GULC_RVALUETOINREFEXPR_HPP
