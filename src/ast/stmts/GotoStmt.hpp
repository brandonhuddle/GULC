#ifndef GULC_GOTOSTMT_HPP
#define GULC_GOTOSTMT_HPP

#include <ast/Stmt.hpp>
#include <ast/Identifier.hpp>

namespace gulc {
    class GotoStmt : public Stmt {
    public:
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::Goto; }

        Identifier const& label() const { return _label; }

        GotoStmt(TextPosition startPosition, TextPosition endPosition, Identifier label)
                : Stmt(Stmt::Kind::Goto),
                  _label(std::move(label)) {}

        TextPosition gotoStartPosition() const { return _startPosition; }
        TextPosition gotoEndPosition() const { return _endPosition; }

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _label.endPosition(); }

    protected:
        Identifier _label;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_GOTOSTMT_HPP
