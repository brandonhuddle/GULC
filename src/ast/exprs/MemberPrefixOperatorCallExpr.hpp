#ifndef GULC_MEMBERPREFIXOPERATORCALLEXPR_HPP
#define GULC_MEMBERPREFIXOPERATORCALLEXPR_HPP

#include <ast/decls/OperatorDecl.hpp>
#include "PrefixOperatorExpr.hpp"

namespace gulc {
    class MemberPrefixOperatorCallExpr : public PrefixOperatorExpr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::MemberPrefixOperatorCall; }

        OperatorDecl* prefixOperatorDecl;

        MemberPrefixOperatorCallExpr(OperatorDecl* prefixOperatorDecl, PrefixOperators prefixOperator,
                                     Expr* nestedExpr,
                                     TextPosition operatorStartPosition, TextPosition operatorEndPosition)
                : PrefixOperatorExpr(Expr::Kind::MemberPrefixOperatorCall, prefixOperator, nestedExpr,
                                     operatorStartPosition, operatorEndPosition),
                  prefixOperatorDecl(prefixOperatorDecl) {}

        Expr* deepCopy() const override {
            auto result = new MemberPrefixOperatorCallExpr(prefixOperatorDecl, _prefixOperator,
                                                           nestedExpr->deepCopy(),
                                                           _operatorStartPosition,
                                                           _operatorEndPosition);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }
    };
}

#endif //GULC_MEMBERPREFIXOPERATORCALLEXPR_HPP
