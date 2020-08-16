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
#ifndef GULC_DOWHILESTMT_HPP
#define GULC_DOWHILESTMT_HPP

#include <ast/Stmt.hpp>
#include <ast/Expr.hpp>
#include <llvm/Support/Casting.h>
#include "CompoundStmt.hpp"

namespace gulc {
    class DoWhileStmt : public Stmt {
    public:
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::DoWhile; }

        Expr* condition;

        DoWhileStmt(CompoundStmt* body, Expr* condition,
                    TextPosition doStartPosition, TextPosition doEndPosition,
                    TextPosition whileStartPosition, TextPosition whileEndPosition)
                : Stmt(Stmt::Kind::DoWhile),
                  condition(condition), _body(body), _doStartPosition(doStartPosition), _doEndPosition(doEndPosition),
                  _whileStartPosition(whileStartPosition), _whileEndPosition(whileEndPosition) {}

        CompoundStmt* body() const { return _body; }

        TextPosition whileStartPosition() const { return _whileStartPosition; }
        TextPosition whileEndPosition() const { return _whileEndPosition; }

        TextPosition startPosition() const override { return _doStartPosition; }
        TextPosition endPosition() const override { return _doEndPosition; }

        Stmt* deepCopy() const override {
            return new DoWhileStmt(llvm::dyn_cast<CompoundStmt>(_body->deepCopy()), condition->deepCopy(),
                                   _doStartPosition, _doEndPosition,
                                   _whileStartPosition, _whileEndPosition);
        }

        // This is used by the passes to store the number of local variables that exist in the context of the for loop
        unsigned int currentNumLocalVariables;

        ~DoWhileStmt() override {
            delete condition;
            delete _body;
        }

    protected:
        CompoundStmt* _body;
        TextPosition _doStartPosition;
        TextPosition _doEndPosition;
        TextPosition _whileStartPosition;
        TextPosition _whileEndPosition;

    };
}

#endif //GULC_DOWHILESTMT_HPP
