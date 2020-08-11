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
#ifndef GULC_LABELEDARGUMENTEXPR_HPP
#define GULC_LABELEDARGUMENTEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/Identifier.hpp>

namespace gulc {
    class LabeledArgumentExpr : public Expr {
    public:
        static bool classof(const Expr *expr) { return expr->getExprKind() == Kind::LabeledArgument; }

        Expr* argument;

        LabeledArgumentExpr(Identifier label, Expr* argument)
                : Expr(Expr::Kind::LabeledArgument),
                  _label(std::move(label)), argument(argument) {}

        Identifier const& label() const { return _label; }
        TextPosition startPosition() const override { return _label.startPosition(); }
        TextPosition endPosition() const override { return argument->endPosition(); }

        Expr* deepCopy() const override {
            auto result = new LabeledArgumentExpr(_label, argument->deepCopy());
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return _label.name() + ": " + argument->toString();
        }

        ~LabeledArgumentExpr() override {
            delete argument;
        }

    protected:
        Identifier _label;

    };
}

#endif //GULC_LABELEDARGUMENTEXPR_HPP
