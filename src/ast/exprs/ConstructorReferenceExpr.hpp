#ifndef GULC_CONSTRUCTORREFERENCEEXPR_HPP
#define GULC_CONSTRUCTORREFERENCEEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/decls/ConstructorDecl.hpp>

namespace gulc {
    class ConstructorReferenceExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::ConstructorReference; }

        ConstructorDecl* constructor;

        ConstructorReferenceExpr(TextPosition startPosition, TextPosition endPosition, ConstructorDecl* constructor)
                : Expr(Expr::Kind::ConstructorReference),
                  constructor(constructor), _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Expr* deepCopy() const override {
            auto result = new ConstructorReferenceExpr(_startPosition, _endPosition, constructor);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return constructor->container->identifier().name();
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_CONSTRUCTORREFERENCEEXPR_HPP
