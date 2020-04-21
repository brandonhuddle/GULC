#ifndef GULC_WHILESTMT_HPP
#define GULC_WHILESTMT_HPP

#include <ast/Stmt.hpp>
#include <ast/Expr.hpp>
#include <llvm/Support/Casting.h>
#include "CompoundStmt.hpp"

namespace gulc {
    class WhileStmt : public Stmt {
    public:
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::While; }

        Expr* condition;

        WhileStmt(Expr* condition, CompoundStmt* body,
                  TextPosition startPosition, TextPosition endPosition)
                : Stmt(Stmt::Kind::While),
                  condition(condition), _body(body),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        CompoundStmt* body() const { return _body; }

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Stmt* deepCopy() const override {
            return new WhileStmt(condition->deepCopy(), llvm::dyn_cast<CompoundStmt>(_body->deepCopy()),
                                 _startPosition, _endPosition);
        }

        ~WhileStmt() override {
            delete condition;
            delete _body;
        }

    protected:
        CompoundStmt* _body;
        // These are start end ends for the `while` keyword
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_WHILESTMT_HPP
