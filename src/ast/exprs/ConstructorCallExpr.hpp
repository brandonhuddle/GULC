#ifndef GULC_CONSTRUCTORCALLEXPR_HPP
#define GULC_CONSTRUCTORCALLEXPR_HPP

#include <vector>
#include <ast/Expr.hpp>
#include <llvm/Support/Casting.h>
#include <ast/decls/ConstructorDecl.hpp>
#include "LabeledArgumentExpr.hpp"

namespace gulc {
    class ConstructorCallExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::ConstructorCall; }

        ConstructorDecl* constructorReference;
        std::vector<LabeledArgumentExpr*> arguments;
        bool hasArguments() const { return !arguments.empty(); }

        ConstructorCallExpr(ConstructorDecl* constructorReference, std::vector<LabeledArgumentExpr*> arguments,
                            TextPosition startPosition, TextPosition endPosition)
                : Expr(Expr::Kind::ConstructorCall),
                  constructorReference(constructorReference), arguments(std::move(arguments)),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Expr* deepCopy() const override {
            std::vector<LabeledArgumentExpr*> copiedArguments;
            copiedArguments.reserve(arguments.size());

            for (Expr* argument : arguments) {
                copiedArguments.push_back(llvm::dyn_cast<LabeledArgumentExpr>(argument->deepCopy()));
            }

            auto result = new ConstructorCallExpr(constructorReference, copiedArguments,
                                                  _startPosition, _endPosition);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            std::string argumentsString;

            for (std::size_t i = 0; i < arguments.size(); ++i) {
                if (i != 0) argumentsString += ", ";

                argumentsString += arguments[i]->toString();
            }

            return constructorReference->container->identifier().name() + "(" + argumentsString + ")";
        }

        ~ConstructorCallExpr() override {
            for (LabeledArgumentExpr* argument : arguments) {
                delete argument;
            }
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_CONSTRUCTORCALLEXPR_HPP
