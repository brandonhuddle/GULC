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
#ifndef GULC_CATCHSTMT_HPP
#define GULC_CATCHSTMT_HPP

#include <ast/Stmt.hpp>
#include <ast/Identifier.hpp>
#include <optional>
#include <ast/Type.hpp>
#include <llvm/Support/Casting.h>
#include "CompoundStmt.hpp"

namespace gulc {
    // Syntax:
    // catch e: exception {} // A typed exception where you want to access the value from the exception
    // catch exception {} // A typed exception where you don't care what the exception says
    // catch {} // A universal, catch all
    class CatchStmt : public Stmt {
    public:
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::Catch; }

        CatchStmt(TextPosition startPosition, TextPosition endPosition, CompoundStmt* body)
                : Stmt(Stmt::Kind::Catch),
                  exceptionType(nullptr), _body(body), _startPosition(startPosition), _endPosition(endPosition) {}
        CatchStmt(TextPosition startPosition, TextPosition endPosition, CompoundStmt* body, Type* exceptionType)
                : Stmt(Stmt::Kind::Catch),
                  exceptionType(exceptionType), _body(body),
                  _startPosition(startPosition), _endPosition(endPosition) {}
        CatchStmt(TextPosition startPosition, TextPosition endPosition, CompoundStmt* body, Type* exceptionType, Identifier varName)
                : Stmt(Stmt::Kind::Catch),
                  _varName(varName), exceptionType(exceptionType), _body(body),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        bool hasVarName() const { return _varName.has_value(); }
        Identifier const& varName() const { return _varName.value(); }
        bool hasExceptionType() const { return exceptionType != nullptr; }
        Type* exceptionType;

        CompoundStmt* body() const { return _body; }

        TextPosition catchStartPosition() const { return _startPosition; }
        TextPosition catchEndPosition() const { return _endPosition; }

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override {
            if (exceptionType != nullptr) {
                return exceptionType->endPosition();
            } else {
                return _endPosition;
            }
        }

        Stmt* deepCopy() const override {
            if (exceptionType == nullptr) {
                return new CatchStmt(_startPosition, _endPosition,
                                     llvm::dyn_cast<CompoundStmt>(_body->deepCopy()));
            } else if (_varName.has_value()) {
                return new CatchStmt(_startPosition, _endPosition,
                                     llvm::dyn_cast<CompoundStmt>(_body->deepCopy()),
                                     exceptionType->deepCopy(), _varName.value());
            } else {
                return new CatchStmt(_startPosition, _endPosition,
                                     llvm::dyn_cast<CompoundStmt>(_body->deepCopy()),
                                     exceptionType->deepCopy());
            }
        }

        ~CatchStmt() override {
            delete exceptionType;
        }

    protected:
        std::optional<Identifier> _varName;
        CompoundStmt* _body;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_CATCHSTMT_HPP
