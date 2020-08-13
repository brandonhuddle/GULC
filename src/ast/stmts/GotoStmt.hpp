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
#ifndef GULC_GOTOSTMT_HPP
#define GULC_GOTOSTMT_HPP

#include <ast/Stmt.hpp>
#include <ast/Identifier.hpp>
#include <ast/Expr.hpp>
#include <vector>

namespace gulc {
    class GotoStmt : public Stmt {
    public:
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::Goto; }

        Identifier const& label() const { return _label; }

        GotoStmt(TextPosition startPosition, TextPosition endPosition, Identifier label)
                : Stmt(Stmt::Kind::Goto),
                  _label(std::move(label)), _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition gotoStartPosition() const { return _startPosition; }
        TextPosition gotoEndPosition() const { return _endPosition; }

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _label.endPosition(); }

        Stmt* deepCopy() const override {
            return new GotoStmt(_startPosition, _endPosition, _label);
        }

        // The most common case for this will be destructor calls.
        std::vector<Expr*> preGotoDeferred;

        ~GotoStmt() override {
            for (Expr* preGotoDeferredExpr : preGotoDeferred) {
                delete preGotoDeferredExpr;
            }
        }

    protected:
        Identifier _label;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_GOTOSTMT_HPP
