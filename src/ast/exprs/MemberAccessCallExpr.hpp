/*
 * Copyright (C) 2020 Brandon Huddle
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef GULC_MEMBERACCESSCALLEXPR_HPP
#define GULC_MEMBERACCESSCALLEXPR_HPP

#include <ast/Expr.hpp>
#include <llvm/Support/Casting.h>
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

        Expr* deepCopy() const override {
            auto result = new MemberAccessCallExpr(_isArrowCall, objectRef->deepCopy(),
                                                   llvm::dyn_cast<IdentifierExpr>(member->deepCopy()));
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            if (_isArrowCall) {
                return objectRef->toString() + "->" + member->toString();
            } else {
                return objectRef->toString() + "." + member->toString();
            }
        }

        ~MemberAccessCallExpr() override {
            delete objectRef;
            delete member;
        }

    private:
        bool _isArrowCall;

    };
}

#endif //GULC_MEMBERACCESSCALLEXPR_HPP
