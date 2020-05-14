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

        Expr* deepCopy() const override {
            std::vector<Expr*> copiedArguments;
            copiedArguments.reserve(arguments.size());

            for (Expr* argument : arguments) {
                copiedArguments.push_back(argument->deepCopy());
            }

            return new FunctionCallExpr(functionReference->deepCopy(), copiedArguments,
                                        _startPosition, _endPosition);
        }

        std::string toString() const override {
            std::string argumentsString;

            for (std::size_t i = 0; i < arguments.size(); ++i) {
                if (i != 0) argumentsString += ", ";

                argumentsString += arguments[i]->toString();
            }

            return functionReference->toString() + "(" + argumentsString + ")";
        }

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
