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
#ifndef GULC_DESTRUCTORCALLEXPR_HPP
#define GULC_DESTRUCTORCALLEXPR_HPP

#include "FunctionCallExpr.hpp"
#include "DestructorReferenceExpr.hpp"

namespace gulc {
    // NOTE: I'm making `DestructorCallExpr` inherit from `FunctionCallExpr` solely for continuity and simplicity sake.
    class DestructorCallExpr : public FunctionCallExpr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::DestructorCall; }

        // This is the object that will be passed as `self`
        Expr* objectRef;

        DestructorCallExpr(DestructorReferenceExpr* destructorReferenceExpr, Expr* objectRef,
                           TextPosition startPosition, TextPosition endPosition)
                : FunctionCallExpr(Expr::Kind::DestructorCall, destructorReferenceExpr, {},
                                   startPosition, endPosition),
                  objectRef(objectRef) {}

        Expr* deepCopy() const override {
            DestructorReferenceExpr* copiedDestructorReferenceExpr = nullptr;

            if (functionReference != nullptr && llvm::isa<DestructorReferenceExpr>(functionReference)) {
                copiedDestructorReferenceExpr = llvm::dyn_cast<DestructorReferenceExpr>(functionReference->deepCopy());
            }

            auto result = new DestructorCallExpr(copiedDestructorReferenceExpr, objectRef->deepCopy(),
                                                  _startPosition, _endPosition);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            std::string argumentsString = objectRef->toString();

            return functionReference->toString() + "(" + argumentsString + ")";
        }

        ~DestructorCallExpr() override {
            delete objectRef;
        }

    };
}

#endif //GULC_DESTRUCTORCALLEXPR_HPP
