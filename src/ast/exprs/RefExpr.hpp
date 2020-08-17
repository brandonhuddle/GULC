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
#ifndef GULC_REFEXPR_HPP
#define GULC_REFEXPR_HPP

#include <ast/Expr.hpp>

namespace gulc {
    class RefExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::Ref; }

        bool isMutable;
        Expr* nestedExpr;

        RefExpr(bool isMutable, Expr* nestedExpr, TextPosition refStartPosition, TextPosition refEndPosition)
                : Expr(Expr::Kind::Ref),
                  isMutable(isMutable), nestedExpr(nestedExpr),
                  _refStartPosition(refStartPosition), _refEndPosition(refEndPosition) {}

        TextPosition startPosition() const override { return _refStartPosition; }
        TextPosition endPosition() const override { return nestedExpr->endPosition(); }

        Expr* deepCopy() const override {
            auto result = new RefExpr(isMutable, nestedExpr->deepCopy(),
                                      _refStartPosition, _refEndPosition);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            std::string mutString;

            if (isMutable) {
                mutString = "mut ";
            }

            return "ref " + mutString + nestedExpr->toString();
        }

        ~RefExpr() override {
            delete nestedExpr;
        }

    protected:
        TextPosition _refStartPosition;
        TextPosition _refEndPosition;

    };
}

#endif //GULC_REFEXPR_HPP
