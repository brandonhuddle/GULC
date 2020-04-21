#ifndef GULC_BREAKSTMT_HPP
#define GULC_BREAKSTMT_HPP

#include <ast/Stmt.hpp>
#include <ast/Identifier.hpp>
#include <optional>

namespace gulc {
    class BreakStmt : public Stmt {
    public:
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::Break; }

        bool hasBreakLabel() const { return _breakLabel.has_value(); }
        Identifier const& breakLabel() const { return _breakLabel.value(); }

        BreakStmt(TextPosition startPosition, TextPosition endPosition)
                : Stmt(Stmt::Kind::Break),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        BreakStmt(TextPosition startPosition, TextPosition endPosition, Identifier breakLabel)
                : Stmt(Stmt::Kind::Break),
                  _startPosition(startPosition), _endPosition(endPosition), _breakLabel(breakLabel) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Stmt* deepCopy() const override {
            if (_breakLabel.has_value()) {
                return new BreakStmt(_startPosition, _endPosition, _breakLabel.value());
            } else {
                return new BreakStmt(_startPosition, _endPosition);
            }
        }

    protected:
        std::optional<Identifier> _breakLabel;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_BREAKSTMT_HPP
