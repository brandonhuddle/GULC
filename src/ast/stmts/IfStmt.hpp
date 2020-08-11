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
#ifndef GULC_IFSTMT_HPP
#define GULC_IFSTMT_HPP

#include <ast/Stmt.hpp>
#include <ast/Expr.hpp>
#include <llvm/Support/Casting.h>
#include "CompoundStmt.hpp"

namespace gulc {
    class IfStmt : public Stmt {
    public:
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::If; }

        Expr* condition;
        CompoundStmt* trueBody() const { return _trueBody; }
        bool hasFalseBody() const { return _falseBody != nullptr; }
        Stmt* falseBody() const { return _falseBody; }

        IfStmt(TextPosition startPosition, TextPosition endPosition, Expr* condition, CompoundStmt* trueBody,
               Stmt* falseBody)
                : Stmt(Stmt::Kind::If),
                  condition(condition), _trueBody(trueBody), _falseBody(falseBody),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Stmt* deepCopy() const override {
            Stmt* copiedFalseBody = nullptr;

            if (_falseBody != nullptr) {
                copiedFalseBody = _falseBody->deepCopy();
            }

            return new IfStmt(_startPosition, _endPosition, condition->deepCopy(),
                              llvm::dyn_cast<CompoundStmt>(_trueBody->deepCopy()),
                              copiedFalseBody);
        }

        ~IfStmt() override {
            delete condition;
            delete _trueBody;
            delete _falseBody;
        }

    protected:
        CompoundStmt* _trueBody;
        // This can ONLY be `if` or a compound statement
        Stmt* _falseBody;
        // The start and end just for the `if` statement
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_IFSTMT_HPP
