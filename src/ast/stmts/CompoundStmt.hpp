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
#ifndef GULC_COMPOUNDSTMT_HPP
#define GULC_COMPOUNDSTMT_HPP

#include <ast/Stmt.hpp>
#include <vector>

namespace gulc {
    class CompoundStmt : public Stmt {
    public:
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::Compound; }

        std::vector<Stmt*> statements;

        CompoundStmt(std::vector<Stmt*> statements, TextPosition startPosition, TextPosition endPosition)
                : Stmt(Stmt::Kind::Compound), statements(std::move(statements)),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Stmt* deepCopy() const override {
            std::vector<Stmt*> copiedStatements;
            copiedStatements.reserve(statements.size());

            for (Stmt* statement : statements) {
                copiedStatements.push_back(statement->deepCopy());
            }

            return new CompoundStmt(copiedStatements, _startPosition, _endPosition);
        }

        ~CompoundStmt() override {
            for (Stmt* statement : statements) {
                delete statement;
            }
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_COMPOUNDSTMT_HPP
