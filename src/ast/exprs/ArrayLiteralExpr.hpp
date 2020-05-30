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

        Expr* deepCopy() const override {
            std::vector<Expr*> copiedIndexes;
            copiedIndexes.reserve(indexes.size());

            for (Expr* index : indexes) {
                copiedIndexes.push_back(index->deepCopy());
            }

            auto result = new ArrayLiteralExpr(copiedIndexes, _startPosition, _endPosition);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            std::string result = "[ ";

            for (std::size_t i = 0; i < indexes.size(); ++i) {
                if (i != 0) result += ", ";

                result += indexes[i]->toString();
            }

            return result + " ]";
        }

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
