#ifndef GULC_FUNCTIONCALLEXPR_HPP
#define GULC_FUNCTIONCALLEXPR_HPP

#include <ast/Expr.hpp>
#include <vector>

namespace gulc {
    class FunctionCallExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::FunctionCall; }

        Expr* functionReference;
        std::vector<Expr*> arguments;
        bool hasArguments() const { return !arguments.empty(); }

        FunctionCallExpr(Expr* functionReference, std::vector<Expr*> arguments,
                         TextPosition startPosition, TextPosition endPosition)
                : Expr(Expr::Kind::FunctionCall),
                  functionReference(functionReference), arguments(std::move(arguments)),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        ~FunctionCallExpr() override {
            for (Expr* argument : arguments) {
                delete argument;
            }

            delete functionReference;
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_FUNCTIONCALLEXPR_HPP
