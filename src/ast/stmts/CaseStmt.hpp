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
#ifndef GULC_CASESTMT_HPP
#define GULC_CASESTMT_HPP

#include <ast/Stmt.hpp>
#include <ast/Expr.hpp>

namespace gulc {
    class CaseStmt : public Stmt {
    public:
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::Case; }

        Expr* condition;
        Stmt* trueStmt;
        bool isDefault() const { return _isDefault; }

        CaseStmt(TextPosition startPosition, TextPosition endPosition, bool isDefault, Expr* condition, Stmt* trueStmt)
                : Stmt(Stmt::Kind::Case),
                  condition(condition), trueStmt(trueStmt), _isDefault(isDefault),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Stmt* deepCopy() const override {
            return new CaseStmt(_startPosition, _endPosition, _isDefault,
                                condition->deepCopy(), trueStmt->deepCopy());
        }

        ~CaseStmt() override {
            delete condition;
            delete trueStmt;
        }

    protected:
        bool _isDefault;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_CASESTMT_HPP
