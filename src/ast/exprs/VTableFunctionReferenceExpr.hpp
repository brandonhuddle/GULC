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
#ifndef GULC_VTABLEFUNCTIONREFERENCEEXPR_HPP
#define GULC_VTABLEFUNCTIONREFERENCEEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/decls/StructDecl.hpp>

namespace gulc {
    /**
     * `VTableFunctionReferenceExpr` is used for storing only vtable function references
     *
     * Examples:
     *     structVar.virtualFunc(...)
     *     virtualFunc(...) // Implicit self
     *
     * NOTE: `Struct::virtualFunc` would NOT be a vtable function reference, it would be a static
     *       `FunctionReferenceExpr`
     */
    class VTableFunctionReferenceExpr : public Expr {
    public:
        static bool classof(const Expr *expr) { return expr->getExprKind() == Kind::VTableFunctionReference; }

        VTableFunctionReferenceExpr(TextPosition startPosition, TextPosition endPosition,
                                    StructDecl* structDecl, std::size_t vtableIndex)
                : Expr(Expr::Kind::VTableFunctionReference),
                  _startPosition(startPosition), _endPosition(endPosition),
                  _structDecl(structDecl), _vtableIndex(vtableIndex) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Expr* deepCopy() const override {
            auto result = new VTableFunctionReferenceExpr(_startPosition, _endPosition,
                                                          _structDecl, _vtableIndex);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return _structDecl->vtable[_vtableIndex]->identifier().name();
        }

    private:
        TextPosition _startPosition;
        TextPosition _endPosition;
        /// The struct for the vtable (NOTE: this is only the struct for the KNOWN type, the actual type may be
        /// different, this should only be used for basic sanity checks and output, NOT inlining)
        StructDecl* _structDecl;
        /// The actual index within the vtable this function was found at.
        std::size_t _vtableIndex;

    };
}

#endif //GULC_VTABLEFUNCTIONREFERENCEEXPR_HPP
