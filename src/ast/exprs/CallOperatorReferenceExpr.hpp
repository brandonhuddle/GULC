#ifndef GULC_CALLOPERATORREFERENCEEXPR_HPP
#define GULC_CALLOPERATORREFERENCEEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/decls/CallOperatorDecl.hpp>

namespace gulc {
    class CallOperatorReferenceExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::CallOperatorReference; }

        CallOperatorDecl* callOperator;

        CallOperatorReferenceExpr(TextPosition startPosition, TextPosition endPosition, CallOperatorDecl* callOperator)
                : Expr(Expr::Kind::CallOperatorReference),
                  callOperator(callOperator), _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Expr* deepCopy() const override {
            auto result = new CallOperatorReferenceExpr(_startPosition, _endPosition, callOperator);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return callOperator->container->identifier().name();
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_CALLOPERATORREFERENCEEXPR_HPP
