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
#ifndef GULC_PARAMETERREFEXPR_HPP
#define GULC_PARAMETERREFEXPR_HPP

#include <ast/Expr.hpp>

namespace gulc {
    class ParameterRefExpr : public Expr {
    public:
        static bool classof(const Expr *expr) { return expr->getExprKind() == Kind::ParameterRef; }

        ParameterRefExpr(TextPosition startPosition, TextPosition endPosition,
                         std::size_t parameterIndex, std::string parameterRef)
                : Expr(Expr::Kind::ParameterRef),
                  _startPosition(startPosition), _endPosition(endPosition),
                  _parameterIndex(parameterIndex), _parameterRef(std::move(parameterRef)) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        std::size_t parameterIndex() const { return _parameterIndex; }
        std::string const& parameterRef() const { return _parameterRef; }

        Expr* deepCopy() const override {
            auto result = new ParameterRefExpr(_startPosition, _endPosition,
                                               _parameterIndex, _parameterRef);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return _parameterRef;
        }

    private:
        TextPosition _startPosition;
        TextPosition _endPosition;
        std::size_t _parameterIndex;
        std::string _parameterRef;

    };
}

#endif //GULC_PARAMETERREFEXPR_HPP
