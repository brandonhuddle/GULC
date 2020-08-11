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
#ifndef GULC_CHECKEXTENDSTYPEEXPR_HPP
#define GULC_CHECKEXTENDSTYPEEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/Type.hpp>

namespace gulc {
    /**
     * This is a special type only used within specific contexts (e.g. `where ...`)
     * This expression is used to check if a type extends another type in any way
     *
     * Syntax:
     *     T : StructExample
     *     T : TraitExample
     *
     * Primary Use:
     *     where T : Trait
     *     where T : Struct
     */
    class CheckExtendsTypeExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::CheckExtendsType; }

        Type* checkType;
        Type* extendsType;

        CheckExtendsTypeExpr(Type* checkType, Type* extendsType,
                             TextPosition extendsStartPosition, TextPosition extendsEndPosition)
                : Expr(Expr::Kind::CheckExtendsType),
                  checkType(checkType), extendsType(extendsType),
                  _extendsStartPosition(extendsStartPosition), _extendsEndPosition(extendsEndPosition) {}

        TextPosition startPosition() const override { return checkType->startPosition(); }
        TextPosition endPosition() const override { return extendsType->endPosition(); }
        TextPosition extendsStartPosition() const { return _extendsStartPosition; }
        TextPosition extendsEndPosition() const { return _extendsEndPosition; }

        Expr* deepCopy() const override {
            auto result = new CheckExtendsTypeExpr(checkType->deepCopy(), extendsType->deepCopy(),
                                                   _extendsStartPosition, _extendsEndPosition);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return checkType->toString() + " : " + extendsType->toString();
        }

        ~CheckExtendsTypeExpr() override {
            delete checkType;
            delete extendsType;
        }

    protected:
        TextPosition _extendsStartPosition;
        TextPosition _extendsEndPosition;

    };
}

#endif //GULC_CHECKEXTENDSTYPEEXPR_HPP
