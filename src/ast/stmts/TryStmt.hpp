#ifndef GULC_TRYSTMT_HPP
#define GULC_TRYSTMT_HPP

#include <ast/Stmt.hpp>
#include "CompoundStmt.hpp"
#include "CatchStmt.hpp"

namespace gulc {
    class TryStmt : public Stmt {
    public:
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::Try; }

        TryStmt(TextPosition startPosition, TextPosition endPosition,
                CompoundStmt* body, std::vector<CatchStmt*> catchStatements, CompoundStmt* finallyStatement)
                : Stmt(Stmt::Kind::Try),
                  _body(body), _catchStatements(std::move(catchStatements)), _finallyStatement(finallyStatement) {}

        CompoundStmt* body() const { return _body; }
        std::vector<CatchStmt*> const& catchStatements() { return _catchStatements; }
        bool hasFinallyStatement() const { return _finallyStatement != nullptr; }
        CompoundStmt* finallyStatement() const { return _finallyStatement; }

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        ~TryStmt() override {
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

#endif //GULC_TRYSTMT_HPP
