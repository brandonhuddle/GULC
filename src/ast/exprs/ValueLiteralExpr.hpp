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
#ifndef GULC_VALUELITERALEXPR_HPP
#define GULC_VALUELITERALEXPR_HPP

#include <ast/Expr.hpp>
#include <string>
#include <ast/Type.hpp>

namespace gulc {
    class ValueLiteralExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::ValueLiteral; }

        enum class LiteralType {
            Char,
            Float,
            Integer,
            String
        };

        /// What kind of literal this value is
        LiteralType literalType() const { return _literalType; }
        /// The string representation for the provided integer literal, this might be larger than can fit into the
        /// literal type
        std::string const& value() const { return _value; }
        /// The provided suffix for the integer literal. I.e. `12i32`, `"example"str`, `2000usize`, `1.0f32`
        std::string const& suffix() const { return _suffix; }
        /// Whether a suffix was provided or not
        bool hasSuffix() const { return !_suffix.empty(); }

        ValueLiteralExpr(LiteralType literalType, std::string value, std::string suffix,
                         TextPosition startPosition, TextPosition endPosition)
                : Expr(Expr::Kind::ValueLiteral),
                  _literalType(literalType), _value(std::move(value)), _suffix(std::move(suffix)),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Expr* deepCopy() const override {
            ValueLiteralExpr* result = new ValueLiteralExpr(_literalType, _value, _suffix,
                                                           _startPosition, _endPosition);

            if (valueType != nullptr) {
                result->valueType = valueType->deepCopy();
            }

            return result;
        }

        std::string toString() const override {
            return _value + _suffix;
        }

    protected:
        LiteralType _literalType;
        std::string _value;
        std::string _suffix;
        // NOTE: There CANNOT be a space between the number and the suffix so the suffix start and end can be
        //       calculated using the length of the value
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_VALUELITERALEXPR_HPP
