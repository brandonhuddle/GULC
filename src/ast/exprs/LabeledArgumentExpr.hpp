#ifndef GULC_LABELEDARGUMENTEXPR_HPP
#define GULC_LABELEDARGUMENTEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/Identifier.hpp>

namespace gulc {
    class LabeledArgumentExpr : public Expr {
    public:
        static bool classof(const Expr *expr) { return expr->getExprKind() == Kind::LabeledArgument; }

        Expr* argument;

        LabeledArgumentExpr(Identifier label, Expr* argument)
                : Expr(Expr::Kind::LabeledArgument),
                  _label(std::move(label)), argument(argument) {}

        Identifier const& label() const { return _label; }
        TextPosition startPosition() const override { return _label.startPosition(); }
        TextPosition endPosition() const override { return argument->endPosition(); }

        Expr* deepCopy() const override {
            return new LabeledArgumentExpr(_label, argument->deepCopy());
        }

        std::string toString() const override {
            return _label.name() + ": " + argument->toString();
        }

        ~LabeledArgumentExpr() override {
            delete argument;
        }

    protected:
        Identifier _label;

    };
}

#endif //GULC_LABELEDARGUMENTEXPR_HPP
