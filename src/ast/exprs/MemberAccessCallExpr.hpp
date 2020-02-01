#ifndef GULC_MEMBERACCESSCALLEXPR_HPP
#define GULC_MEMBERACCESSCALLEXPR_HPP

#include <ast/Expr.hpp>
#include "IdentifierExpr.hpp"

namespace gulc {
    class MemberAccessCallExpr : public Expr {
    public:
        static bool classof(const Expr *expr) { return expr->getExprKind() == Kind::MemberAccessCall; }

        MemberAccessCallExpr(bool isArrowCall, Expr* objectRef, IdentifierExpr* member)
                : Expr(Expr::Kind::MemberAccessCall),
                  objectRef(objectRef), member(member), _isArrowCall(isArrowCall) {}

        bool isArrowCall() const { return _isArrowCall; }
        Expr* objectRef;
        IdentifierExpr* member;

        TextPosition startPosition() const override { return objectRef->startPosition(); }
        TextPosition endPosition() const override { return member->endPosition(); }

        ~MemberAccessCallExpr() override {
            delete objectRef;
            delete member;
        }

    private:
        bool _isArrowCall;

    };
}

#endif //GULC_MEMBERACCESSCALLEXPR_HPP
