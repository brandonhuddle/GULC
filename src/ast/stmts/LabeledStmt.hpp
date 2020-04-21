#ifndef GULC_LABELEDSTMT_HPP
#define GULC_LABELEDSTMT_HPP

#include <ast/Stmt.hpp>
#include <ast/Identifier.hpp>

namespace gulc {
    class LabeledStmt : public Stmt {
    public:
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::Labeled; }

        Identifier const& label() const { return _label; }
        Stmt* labeledStmt;

        LabeledStmt(Identifier label, Stmt* labeledStmt)
                : Stmt(Stmt::Kind::Labeled),
                  _label(std::move(label)), labeledStmt(labeledStmt) {}

        TextPosition startPosition() const override { return _label.startPosition(); }
        TextPosition endPosition() const override { return _label.endPosition(); }

        Stmt* deepCopy() const override {
            return new LabeledStmt(_label, labeledStmt->deepCopy());
        }

        ~LabeledStmt() override {
            delete labeledStmt;
        }

    protected:
        Identifier _label;

    };
}

#endif //GULC_LABELEDSTMT_HPP
