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
#ifndef GULC_DOSTMT_HPP
#define GULC_DOSTMT_HPP

#include <ast/Stmt.hpp>
#include <llvm/Support/Casting.h>
#include "CompoundStmt.hpp"

namespace gulc {
    class DoStmt : public Stmt {
    public:
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::Do; }

        DoStmt(TextPosition startPosition, TextPosition endPosition, CompoundStmt* body)
                : Stmt(Stmt::Kind::Do),
                  _body(body), _startPosition(startPosition), _endPosition(endPosition) {}

        CompoundStmt* body() const { return _body; }

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Stmt* deepCopy() const override {
            return new DoStmt(_startPosition, _endPosition, llvm::dyn_cast<CompoundStmt>(_body->deepCopy()));
        }

        ~DoStmt() override {
            delete _body;
        }

    protected:
        CompoundStmt* _body;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_DOSTMT_HPP
