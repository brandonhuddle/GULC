#ifndef GULC_COMPOUNDSTMT_HPP
#define GULC_COMPOUNDSTMT_HPP

#include <ast/Stmt.hpp>
#include <vector>

namespace gulc {
    class CompoundStmt : public Stmt {
    public:
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::Compound; }

        std::vector<Stmt*> statements;

        CompoundStmt(std::vector<Stmt*> statements, TextPosition startPosition, TextPosition endPosition)
                : Stmt(Stmt::Kind::Compound), statements(std::move(statements)),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Stmt* deepCopy() const override {
            std::vector<Stmt*> copiedStatements;
            copiedStatements.reserve(statements.size());

            for (Stmt* statement : statements) {
                copiedStatements.push_back(statement->deepCopy());
            }

            return new CompoundStmt(copiedStatements, _startPosition, _endPosition);
        }

        ~CompoundStmt() override {
            for (Stmt* statement : statements) {
                delete statement;
            }
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_COMPOUNDSTMT_HPP
