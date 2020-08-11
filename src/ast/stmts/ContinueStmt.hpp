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
#ifndef GULC_CONTINUESTMT_HPP
#define GULC_CONTINUESTMT_HPP

#include <ast/Stmt.hpp>
#include <ast/Identifier.hpp>
#include <optional>

namespace gulc {
    class ContinueStmt : public Stmt {
    public:
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::Continue; }

        bool hasContinueLabel() const { return _continueLabel.has_value(); }
        Identifier const& continueLabel() const { return _continueLabel.value(); }

        ContinueStmt(TextPosition startPosition, TextPosition endPosition)
                : Stmt(Stmt::Kind::Continue),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        ContinueStmt(TextPosition startPosition, TextPosition endPosition, Identifier continueLabel)
                : Stmt(Stmt::Kind::Continue),
                  _startPosition(startPosition), _endPosition(endPosition), _continueLabel(continueLabel) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Stmt* deepCopy() const override {
            if (_continueLabel.has_value()) {
                return new ContinueStmt(_startPosition, _endPosition, _continueLabel.value());
            } else {
                return new ContinueStmt(_startPosition, _endPosition);
            }
        }

    protected:
        std::optional<Identifier> _continueLabel;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_CONTINUESTMT_HPP
