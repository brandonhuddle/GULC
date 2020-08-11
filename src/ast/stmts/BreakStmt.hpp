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
#ifndef GULC_BREAKSTMT_HPP
#define GULC_BREAKSTMT_HPP

#include <ast/Stmt.hpp>
#include <ast/Identifier.hpp>
#include <optional>

namespace gulc {
    class BreakStmt : public Stmt {
    public:
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::Break; }

        bool hasBreakLabel() const { return _breakLabel.has_value(); }
        Identifier const& breakLabel() const { return _breakLabel.value(); }

        BreakStmt(TextPosition startPosition, TextPosition endPosition)
                : Stmt(Stmt::Kind::Break),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        BreakStmt(TextPosition startPosition, TextPosition endPosition, Identifier breakLabel)
                : Stmt(Stmt::Kind::Break),
                  _startPosition(startPosition), _endPosition(endPosition), _breakLabel(breakLabel) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Stmt* deepCopy() const override {
            if (_breakLabel.has_value()) {
                return new BreakStmt(_startPosition, _endPosition, _breakLabel.value());
            } else {
                return new BreakStmt(_startPosition, _endPosition);
            }
        }

    protected:
        std::optional<Identifier> _breakLabel;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_BREAKSTMT_HPP
