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
#ifndef GULC_SWITCHSTMT_HPP
#define GULC_SWITCHSTMT_HPP

#include <ast/Stmt.hpp>
#include <ast/Expr.hpp>
#include <vector>

namespace gulc {
    class SwitchStmt : public Stmt {
    public:
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::Switch; }

        Expr* condition;
        // We keep this out as it might be modified...
        std::vector<Stmt*> statements;

        SwitchStmt(TextPosition startPosition, TextPosition endPosition, Expr* condition, std::vector<Stmt*> statements)
                : Stmt(Stmt::Kind::Switch),
                  condition(condition), statements(std::move(statements)) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Stmt* deepCopy() const override {
            std::vector<Stmt*> copiedStatements;
            copiedStatements.reserve(statements.size());

            for (Stmt* statement : statements) {
                copiedStatements.push_back(statement->deepCopy());
            }

            return new SwitchStmt(_startPosition, _endPosition, condition->deepCopy(),
                                  copiedStatements);
        }

        ~SwitchStmt() override {
            delete condition;

            for (Stmt* statement : statements) {
                delete statement;
            }
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_SWITCHSTMT_HPP
