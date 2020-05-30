#ifndef GULC_INDEXERCALLEXPR_HPP
#define GULC_INDEXERCALLEXPR_HPP

#include <ast/Expr.hpp>
#include <vector>
#include <llvm/Support/Casting.h>
#include "LabeledArgumentExpr.hpp"

namespace gulc {
    class IndexerCallExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::IndexerCall; }

        Expr* indexerReference;
        std::vector<LabeledArgumentExpr*> arguments;
        bool hasArguments() const { return !arguments.empty(); }

        IndexerCallExpr(Expr* indexerReference, std::vector<LabeledArgumentExpr*> arguments,
                        TextPosition startPosition, TextPosition endPosition)
                : Expr(Expr::Kind::IndexerCall),
                  indexerReference(indexerReference), arguments(std::move(arguments)),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Expr* deepCopy() const override {
            std::vector<LabeledArgumentExpr*> copiedArguments;
            copiedArguments.reserve(arguments.size());

            for (Expr* argument : arguments) {
                copiedArguments.push_back(llvm::dyn_cast<LabeledArgumentExpr>(argument->deepCopy()));
            }

            auto result = new IndexerCallExpr(indexerReference->deepCopy(), copiedArguments,
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

            return indexerReference->toString() + "[" + argumentsString + "]";
        }

        ~IndexerCallExpr() override {
            for (Expr* argument : arguments) {
                delete argument;
            }

            delete indexerReference;
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_INDEXERCALLEXPR_HPP
