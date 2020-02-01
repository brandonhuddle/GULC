#ifndef GULC_CASESTMT_HPP
#define GULC_CASESTMT_HPP

#include <ast/Stmt.hpp>
#include <ast/Expr.hpp>

namespace gulc {
    class CaseStmt : public Stmt {
    public:
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::Case; }

        Expr* condition;
        Stmt* trueStmt;
        bool isDefault() const { return _isDefault; }

        CaseStmt(TextPosition startPosition, TextPosition endPosition, bool isDefault, Expr* condition, Stmt* trueStmt)
                : Stmt(Stmt::Kind::Case),
                  condition(condition), trueStmt(trueStmt), _isDefault(isDefault),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        ~CaseStmt() override {
            delete condition;
            delete trueStmt;
        }

    protected:
        bool _isDefault;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_CASESTMT_HPP
