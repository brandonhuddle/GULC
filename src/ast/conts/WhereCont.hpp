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
#ifndef GULC_WHERECONT_HPP
#define GULC_WHERECONT_HPP

#include <ast/Cont.hpp>
#include <ast/Expr.hpp>

namespace gulc {
    /**
     * WhereCont is used to change how templates are used. It is basically `Requires` but for template types and
     * template constants
     */
    class WhereCont : public Cont {
    public:
        static bool classof(const Cont* cont) { return cont->getContKind() == Cont::Kind::Where; }

        Expr* condition;

        WhereCont(Expr* condition, TextPosition startPosition, TextPosition endPosition)
                : Cont(Cont::Kind::Where, startPosition, endPosition), condition(condition) {}

        Cont* deepCopy() const override {
            return new WhereCont(condition->deepCopy(), _startPosition, _endPosition);
        }

        ~WhereCont() override {
            delete condition;
        }

    };
}

#endif //GULC_WHERECONT_HPP
