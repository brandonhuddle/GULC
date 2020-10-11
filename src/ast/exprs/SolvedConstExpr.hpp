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
#ifndef GULC_SOLVEDCONSTEXPR_HPP
#define GULC_SOLVEDCONSTEXPR_HPP

#include <ast/Expr.hpp>

namespace gulc {
    // This does exactly what its name suggests: stores a solved `const` expression's value.
    // I'm creating this so we have a way to still know WHAT was solved after solving. There
    // are some areas where we will want to keep information for debug while still having the
    // problem solved.
    class SolvedConstExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::SolvedConst; }

        // The expression that was solved
        Expr* original;
        // The expression for the solution
        Expr* solution;

        SolvedConstExpr(Expr* original, Expr* solution)
                : Expr(Expr::Kind::SolvedConst),
                  original(original), solution(solution) {}

        TextPosition startPosition() const override { return original->startPosition(); }
        TextPosition endPosition() const override { return original->endPosition(); }

        Expr* deepCopy() const override {
            auto result = new SolvedConstExpr(original->deepCopy(), solution->deepCopy());
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return original->toString();
        }

        ~SolvedConstExpr() override {
            delete original;
            delete solution;
        }

    };
}

#endif //GULC_SOLVEDCONSTEXPR_HPP
