#ifndef GULC_INDEXERCALLEXPR_HPP
#define GULC_INDEXERCALLEXPR_HPP

#include <ast/Expr.hpp>
#include <vector>

namespace gulc {
    class IndexerCallExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::IndexerCall; }

        Expr* indexerReference;
        std::vector<Expr*> arguments;
        bool hasArguments() const { return !arguments.empty(); }

        IndexerCallExpr(Expr* indexerReference, std::vector<Expr*> arguments,
                        TextPosition startPosition, TextPosition endPosition)
                : Expr(Expr::Kind::IndexerCall),
                  indexerReference(indexerReference), arguments(std::move(arguments)),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

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
