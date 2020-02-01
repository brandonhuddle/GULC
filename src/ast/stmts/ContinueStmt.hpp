#ifndef GULC_CONTINUESTMT_HPP
#define GULC_CONTINUESTMT_HPP

#include <ast/Stmt.hpp>
#include <ast/Identifier.hpp>
#include <optional>

namespace gulc {
    class ContinueStmt : public Stmt {
    public:
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::Continue; }

        bool hasContinueLabel() const { return _continueLabel.has_value(); }
        Identifier const& continueLabel() const { return _continueLabel.value(); }

        ContinueStmt(TextPosition startPosition, TextPosition endPosition)
                : Stmt(Stmt::Kind::Continue),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        ContinueStmt(TextPosition startPosition, TextPosition endPosition, Identifier continueLabel)
                : Stmt(Stmt::Kind::Continue),
                  _startPosition(startPosition), _endPosition(endPosition), _continueLabel(continueLabel) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

    protected:
        std::optional<Identifier> _continueLabel;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_CONTINUESTMT_HPP
