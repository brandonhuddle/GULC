#ifndef GULC_IFSTMT_HPP
#define GULC_IFSTMT_HPP

#include <ast/Stmt.hpp>
#include <ast/Expr.hpp>
#include "CompoundStmt.hpp"

namespace gulc {
    class IfStmt : public Stmt {
    public:
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::If; }

        Expr* condition;
        CompoundStmt* trueBody() const { return _trueBody; }
        bool hasFalseBody() const { return _falseBody != nullptr; }
        Stmt* falseBody() const { return _falseBody; }

        IfStmt(TextPosition startPosition, TextPosition endPosition, Expr* condition, CompoundStmt* trueBody,
               Stmt* falseBody)
                : Stmt(Stmt::Kind::If),
                  condition(condition), _trueBody(trueBody), _falseBody(falseBody),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        ~IfStmt() override {
            delete condition;
            delete _trueBody;
            delete _falseBody;
        }

    protected:
        CompoundStmt* _trueBody;
        // This can ONLY be `if` or a compound statement
        Stmt* _falseBody;
        // The start and end just for the `if` statement
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_IFSTMT_HPP
