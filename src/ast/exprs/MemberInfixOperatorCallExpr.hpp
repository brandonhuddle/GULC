#ifndef GULC_MEMBERINFIXOPERATORCALLEXPR_HPP
#define GULC_MEMBERINFIXOPERATORCALLEXPR_HPP

#include <ast/decls/OperatorDecl.hpp>
#include "InfixOperatorExpr.hpp"

namespace gulc {
    class MemberInfixOperatorCallExpr : public InfixOperatorExpr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::MemberInfixOperatorCall; }

        OperatorDecl* infixOperatorDecl;

        MemberInfixOperatorCallExpr(OperatorDecl* infixOperatorDecl, InfixOperators infixOperator,
                                    Expr* leftValue, Expr* rightValue)
                : InfixOperatorExpr(Expr::Kind::MemberInfixOperatorCall, infixOperator, leftValue, rightValue),
                  infixOperatorDecl(infixOperatorDecl) {}

        Expr* deepCopy() const override {
            auto result = new MemberInfixOperatorCallExpr(infixOperatorDecl, _infixOperator,
                                                          leftValue->deepCopy(),
                                                          rightValue->deepCopy());
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

    };
}

#endif //GULC_MEMBERINFIXOPERATORCALLEXPR_HPP
