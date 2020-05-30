#ifndef GULC_FALLTHROUGHSTMT_HPP
#define GULC_FALLTHROUGHSTMT_HPP

#include <ast/Stmt.hpp>

namespace gulc {
    class FallthroughStmt : public Stmt {
    public:
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::Fallthrough; }

        FallthroughStmt(TextPosition startPosition, TextPosition endPosition)
                : Stmt(Stmt::Kind::Fallthrough),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Stmt* deepCopy() const override {
            return new FallthroughStmt(_startPosition, _endPosition);
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_FALLTHROUGHSTMT_HPP
