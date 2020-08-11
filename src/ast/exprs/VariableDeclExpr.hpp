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
#ifndef GULC_VARIABLEDECLEXPR_HPP
#define GULC_VARIABLEDECLEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/Identifier.hpp>
#include <ast/Type.hpp>

namespace gulc {
    class VariableDeclExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::VariableDecl; }

        Type* type;
        Expr* initialValue;

        VariableDeclExpr(Identifier identifier, Type* type, Expr* initialValue, bool isAssignable,
                         TextPosition startPosition, TextPosition endPosition)
                : Expr(Expr::Kind::VariableDecl),
                  type(type), initialValue(initialValue), _identifier(std::move(identifier)),
                  _isAssignable(isAssignable), _startPosition(startPosition), _endPosition(endPosition) {}

        Identifier const& identifier() const { return _identifier; }
        bool isAssignable() const { return _isAssignable; }

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Expr* deepCopy() const override {
            Expr* copiedInitialValue = nullptr;

            if (initialValue != nullptr) {
                copiedInitialValue = initialValue->deepCopy();
            }

            auto result = new VariableDeclExpr(_identifier, type->deepCopy(), copiedInitialValue,
                                               _isAssignable, _startPosition, _endPosition);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            std::string letString = "let ";
            std::string typeString;
            std::string initialValueString;

            if (_isAssignable) {
                letString += "mut ";
            }

            if (type != nullptr) {
                typeString = " : " + type->toString();
            }

            if (initialValue != nullptr) {
                initialValueString = " = " + initialValue->toString();
            }

            return letString + _identifier.name() + typeString + initialValueString;
        }

        ~VariableDeclExpr() override {
            delete type;
            delete initialValue;
        }

    protected:
        Identifier _identifier;

        // `let mut`
        bool _isAssignable;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_VARIABLEDECLEXPR_HPP
