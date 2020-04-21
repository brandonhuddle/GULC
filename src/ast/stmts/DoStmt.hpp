#ifndef GULC_DOSTMT_HPP
#define GULC_DOSTMT_HPP

#include <ast/Stmt.hpp>
#include <ast/Expr.hpp>
#include <llvm/Support/Casting.h>
#include "CompoundStmt.hpp"

namespace gulc {
    class DoStmt : public Stmt {
    public:
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::Do; }

        Expr* condition;

        DoStmt(CompoundStmt* body, Expr* condition,
               TextPosition doStartPosition, TextPosition doEndPosition,
               TextPosition whileStartPosition, TextPosition whileEndPosition)
                : Stmt(Stmt::Kind::Do),
                  condition(condition), _body(body), _doStartPosition(doStartPosition), _doEndPosition(doEndPosition),
                  _whileStartPosition(whileStartPosition), _whileEndPosition(whileEndPosition) {}

        CompoundStmt* body() const { return _body; }

        TextPosition whileStartPosition() const { return _whileStartPosition; }
        TextPosition whileEndPosition() const { return _whileEndPosition; }

        TextPosition startPosition() const override { return _doStartPosition; }
        TextPosition endPosition() const override { return _doEndPosition; }

        Stmt* deepCopy() const override {
            return new DoStmt(llvm::dyn_cast<CompoundStmt>(_body->deepCopy()), condition->deepCopy(),
                              _doStartPosition, _doEndPosition,
                              _whileStartPosition, _whileEndPosition);
        }

        ~DoStmt() override {
            delete condition;
            delete _body;
        }

    protected:
        CompoundStmt* _body;
        TextPosition _doStartPosition;
        TextPosition _doEndPosition;
        TextPosition _whileStartPosition;
        TextPosition _whileEndPosition;

    };
}

#endif //GULC_DOSTMT_HPP
