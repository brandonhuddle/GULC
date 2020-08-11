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
#ifndef GULC_FUNCTIONREFERENCEEXPR_HPP
#define GULC_FUNCTIONREFERENCEEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/decls/FunctionDecl.hpp>

namespace gulc {
    /**
     * `FunctionReferenceExpr` is used for storing any reference to a non-vtable function.
     *
     * NOTE: This does NOT store `vtable` function references, for those view `VTableFunctionReferenceExpr`
     *
     * Examples:
     *     structVar.member(...)
     *     Struct::member // Grab the actual function pointer from the type...
     *     Struct::virtualMember // Even if it is virtual, usage of `::member` grabs the underlying function
     *     member(...) // For implicitly using `self`
     */
    class FunctionReferenceExpr : public Expr {
    public:
        static bool classof(const Expr *expr) { return expr->getExprKind() == Kind::FunctionReference; }

        FunctionReferenceExpr(TextPosition startPosition, TextPosition endPosition, FunctionDecl* functionDecl)
                : Expr(Expr::Kind::FunctionReference),
                  _startPosition(startPosition), _endPosition(endPosition), _functionDecl(functionDecl) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        FunctionDecl* functionDecl() const { return _functionDecl; }

        Expr* deepCopy() const override {
            auto result = new FunctionReferenceExpr(_startPosition, _endPosition,
                                                    _functionDecl);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return _functionDecl->identifier().name();
        }

    private:
        TextPosition _startPosition;
        TextPosition _endPosition;
        // NOTE: We don't own this so we don't free it.
        FunctionDecl* _functionDecl;

    };
}

#endif //GULC_FUNCTIONREFERENCEEXPR_HPP
