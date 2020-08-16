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
#ifndef GULC_LABELEDSTMT_HPP
#define GULC_LABELEDSTMT_HPP

#include <ast/Stmt.hpp>
#include <ast/Identifier.hpp>

namespace gulc {
    class LabeledStmt : public Stmt {
    public:
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::Labeled; }

        Identifier const& label() const { return _label; }
        Stmt* labeledStmt;

        LabeledStmt(Identifier label, Stmt* labeledStmt)
                : Stmt(Stmt::Kind::Labeled),
                  _label(std::move(label)), labeledStmt(labeledStmt), currentNumLocalVariables(0) {}

        TextPosition startPosition() const override { return _label.startPosition(); }
        TextPosition endPosition() const override { return _label.endPosition(); }

        Stmt* deepCopy() const override {
            auto result = new LabeledStmt(_label, labeledStmt->deepCopy());
            result->currentNumLocalVariables = currentNumLocalVariables;
            return result;
        }

        // This is used by the passes to store the number of local variables that were declared before the label
        unsigned int currentNumLocalVariables;

        ~LabeledStmt() override {
            delete labeledStmt;
        }

    protected:
        Identifier _label;

    };
}

#endif //GULC_LABELEDSTMT_HPP
