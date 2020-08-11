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
#ifndef GULC_LOCALVARIABLEREFEXPR_HPP
#define GULC_LOCALVARIABLEREFEXPR_HPP

#include <ast/Expr.hpp>

namespace gulc {
    class LocalVariableRefExpr : public Expr {
    public:
        static bool classof(const Expr *expr) { return expr->getExprKind() == Kind::LocalVariableRef; }

        LocalVariableRefExpr(TextPosition startPosition, TextPosition endPosition, std::string variableName)
                : Expr(Expr::Kind::LocalVariableRef),
                  _startPosition(startPosition), _endPosition(endPosition), _variableName(std::move(variableName)) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        std::string const& variableName() const { return _variableName; }

        Expr* deepCopy() const override {
            auto result = new LocalVariableRefExpr(_startPosition, _endPosition,
                                                   _variableName);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return _variableName;
        }

    private:
        TextPosition _startPosition;
        TextPosition _endPosition;
        // Two alternatives I though about going with:
        //  1. Keep a reference to `VariableDeclExpr`, decided against it since a `deepCopy` on a `FunctionDecl` could
        //     invalidate all `LocalVariableRefExpr`s with that, requiring us to manually redo the bodies
        //  2. Store the index of the local variable in the current context, decided against it to allow easier
        //     variable elision without needing to redo all indexes
        // I don't like using the variable name but we can optimize it somewhat through a map to improve lookup times.
        // We might not even need a lookup, we might be able to just output the actual variables reference in IR
        // through variable names still. We will see.
        std::string _variableName;

    };
}

#endif //GULC_LOCALVARIABLEREFEXPR_HPP
