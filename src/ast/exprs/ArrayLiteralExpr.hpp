#ifndef GULC_ARRAYLITERALEXPR_HPP
#define GULC_ARRAYLITERALEXPR_HPP

#include <ast/Expr.hpp>
#include <vector>

namespace gulc {
    /**
     * Container for array literals
     *
     * Examples:
     *     [ 12, 33, 2 ]
     *     [ "Hello", ", world!" ]
     *     [ function(), 44 + 22, 2.0f as int ]
     */
    class ArrayLiteralExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::ArrayLiteral; }

        std::vector<Expr*> indexes;

        ArrayLiteralExpr(std::vector<Expr*> indexes,
                         TextPosition startPosition, TextPosition endPosition)
                : Expr(Expr::Kind::ArrayLiteral),
                  indexes(std::move(indexes)), _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        ~ArrayLiteralExpr() override {
            for (Expr* index : indexes) {
                delete index;
            }
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_ARRAYLITERALEXPR_HPP
