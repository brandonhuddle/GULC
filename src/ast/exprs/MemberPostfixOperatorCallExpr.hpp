#ifndef GULC_MEMBERPOSTFIXOPERATORCALLEXPR_HPP
#define GULC_MEMBERPOSTFIXOPERATORCALLEXPR_HPP

#include <ast/decls/OperatorDecl.hpp>
#include "PostfixOperatorExpr.hpp"

namespace gulc {
    class MemberPostfixOperatorCallExpr : public PostfixOperatorExpr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::MemberPostfixOperatorCall; }

        OperatorDecl* postfixOperatorDecl;

        MemberPostfixOperatorCallExpr(OperatorDecl* postfixOperatorDecl, PostfixOperators postfixOperator,
                                      Expr* nestedExpr, TextPosition operatorStartPosition,
                                      TextPosition operatorEndPosition)
                : PostfixOperatorExpr(Expr::Kind::MemberPostfixOperatorCall, postfixOperator, nestedExpr,
                                      operatorStartPosition, operatorEndPosition),
                  postfixOperatorDecl(postfixOperatorDecl) {}

        Expr* deepCopy() const override {
            auto result = new MemberPostfixOperatorCallExpr(postfixOperatorDecl, _postfixOperator,
                                                            nestedExpr->deepCopy(),
                                                            _operatorStartPosition,
                                                            _operatorEndPosition);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

    };
}

#endif //GULC_MEMBERPOSTFIXOPERATORCALLEXPR_HPP
