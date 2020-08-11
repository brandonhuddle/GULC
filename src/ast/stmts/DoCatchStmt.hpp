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
#ifndef GULC_DOCATCHSTMT_HPP
#define GULC_DOCATCHSTMT_HPP

#include <ast/Stmt.hpp>
#include "CompoundStmt.hpp"
#include "CatchStmt.hpp"

namespace gulc {
    class DoCatchStmt : public Stmt {
    public:
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::DoCatch; }

        DoCatchStmt(TextPosition startPosition, TextPosition endPosition,
                    CompoundStmt* body, std::vector<CatchStmt*> catchStatements, CompoundStmt* finallyStatement)
                : Stmt(Stmt::Kind::DoCatch),
                  _body(body), _catchStatements(std::move(catchStatements)), _finallyStatement(finallyStatement),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        CompoundStmt* body() const { return _body; }
        std::vector<CatchStmt*> const& catchStatements() { return _catchStatements; }
        bool hasFinallyStatement() const { return _finallyStatement != nullptr; }
        CompoundStmt* finallyStatement() const { return _finallyStatement; }

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Stmt* deepCopy() const override {
            std::vector<CatchStmt*> copiedCatchStatements;
            copiedCatchStatements.reserve(_catchStatements.size());
            CompoundStmt* copiedFinallyStatement = nullptr;

            for (CatchStmt* catchStatement : _catchStatements) {
                copiedCatchStatements.push_back(llvm::dyn_cast<CatchStmt>(catchStatement->deepCopy()));
            }

            if (_finallyStatement != nullptr) {
                copiedFinallyStatement = llvm::dyn_cast<CompoundStmt>(_finallyStatement->deepCopy());
            }

            return new DoCatchStmt(_startPosition, _endPosition,
                                   llvm::dyn_cast<CompoundStmt>(_body->deepCopy()),
                                   copiedCatchStatements, copiedFinallyStatement);
        }

        ~DoCatchStmt() override {
            delete _body;

            for (CatchStmt* catchStmt : _catchStatements) {
                delete catchStmt;
            }

            delete _finallyStatement;
        }

    protected:
        CompoundStmt* _body;
        std::vector<CatchStmt*> _catchStatements;
        CompoundStmt* _finallyStatement;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_DOCATCHSTMT_HPP
