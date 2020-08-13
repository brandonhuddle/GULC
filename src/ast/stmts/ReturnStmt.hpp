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
#ifndef GULC_RETURNSTMT_HPP
#define GULC_RETURNSTMT_HPP

#include <ast/Stmt.hpp>
#include <ast/Expr.hpp>
#include <vector>

namespace gulc {
    class ReturnStmt : public Stmt {
    public:
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::Return; }

        Expr* returnValue;

        ReturnStmt(TextPosition startPosition, TextPosition endPosition)
                : Stmt(Stmt::Kind::Return), returnValue(nullptr) {}
        ReturnStmt(TextPosition startPosition, TextPosition endPosition, Expr* returnValue)
                : Stmt(Stmt::Kind::Return), returnValue(returnValue) {}

        TextPosition returnStartPosition() const { return _startPosition; }
        TextPosition returnEndPosition() const { return _endPosition; }

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override {
            if (returnValue != nullptr) {
                return returnValue->endPosition();
            } else {
                return _endPosition;
            }
        }

        Stmt* deepCopy() const override {
            if (returnValue == nullptr) {
                return new ReturnStmt(_startPosition, _endPosition);
            } else {
                return new ReturnStmt(_startPosition, _endPosition, returnValue->deepCopy());
            }
        }

        // The most common case for this will be destructor calls.
        std::vector<Expr*> preReturnDeferred;

        ~ReturnStmt() override {
            for (Expr* preReturnDeferredExpr : preReturnDeferred) {
                delete preReturnDeferredExpr;
            }

            delete returnValue;
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_RETURNSTMT_HPP
