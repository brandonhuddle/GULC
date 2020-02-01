#ifndef GULC_FORSTMT_HPP
#define GULC_FORSTMT_HPP

#include <ast/Stmt.hpp>
#include <ast/Expr.hpp>
#include "CompoundStmt.hpp"

namespace gulc {
    class ForStmt : public Stmt {
    public:
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::For; }

        // The initialization e.g. i = 0
        Expr* init;
        // The condition e.g. i < 10
        Expr* condition;
        // The iteration e.g. ++i
        Expr* iteration;

        ForStmt(Expr* init, Expr* condition, Expr* iteration, CompoundStmt* body,
                TextPosition startPosition, TextPosition endPosition)
                : Stmt(Stmt::Kind::For),
                  init(init), condition(condition), iteration(iteration), _body(body),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        CompoundStmt* body() const { return _body; }

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        ~ForStmt() override {
            delete init;
            delete condition;
            delete iteration;
            delete _body;
        }

    protected:
        CompoundStmt* _body;
        // These are start end ends for the `while` keyword
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_FORSTMT_HPP
