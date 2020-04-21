#ifndef GULC_RETURNSTMT_HPP
#define GULC_RETURNSTMT_HPP

#include <ast/Stmt.hpp>
#include <ast/Expr.hpp>

namespace gulc {
    class ReturnStmt : public Stmt {
    public:
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::Return; }

        Expr* returnValue;

        ReturnStmt(TextPosition startPosition, TextPosition endPosition)
                : Stmt(Stmt::Kind::Return), returnValue(nullptr) {}
        ReturnStmt(TextPosition startPosition, TextPosition endPosition, Expr* returnValue)
                : Stmt(Stmt::Kind::Return), returnValue(returnValue) {}

        TextPosition returnStartPosition() const { return _startPosition; }
        TextPosition returnEndPosition() const { return _endPosition; }

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override {
            if (returnValue != nullptr) {
                return returnValue->endPosition();
            } else {
                return _endPosition;
            }
        }

        Stmt* deepCopy() const override {
            if (returnValue == nullptr) {
                return new ReturnStmt(_startPosition, _endPosition);
            } else {
                return new ReturnStmt(_startPosition, _endPosition, returnValue->deepCopy());
            }
        }

        ~ReturnStmt() override {
            delete returnValue;
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_RETURNSTMT_HPP
