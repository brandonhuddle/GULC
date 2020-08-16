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
#ifndef GULC_WHILESTMT_HPP
#define GULC_WHILESTMT_HPP

#include <ast/Stmt.hpp>
#include <ast/Expr.hpp>
#include <llvm/Support/Casting.h>
#include "CompoundStmt.hpp"

namespace gulc {
    class WhileStmt : public Stmt {
    public:
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::While; }

        Expr* condition;

        WhileStmt(Expr* condition, CompoundStmt* body,
                  TextPosition startPosition, TextPosition endPosition)
                : Stmt(Stmt::Kind::While),
                  condition(condition), _body(body),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        CompoundStmt* body() const { return _body; }

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Stmt* deepCopy() const override {
            return new WhileStmt(condition->deepCopy(), llvm::dyn_cast<CompoundStmt>(_body->deepCopy()),
                                 _startPosition, _endPosition);
        }

        // This is used by the passes to store the number of local variables that exist in the context of the for loop
        unsigned int currentNumLocalVariables;

        ~WhileStmt() override {
            delete condition;
            delete _body;
        }

    protected:
        CompoundStmt* _body;
        // These are start end ends for the `while` keyword
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_WHILESTMT_HPP
