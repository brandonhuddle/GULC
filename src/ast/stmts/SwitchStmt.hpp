#ifndef GULC_SWITCHSTMT_HPP
#define GULC_SWITCHSTMT_HPP

#include <ast/Stmt.hpp>
#include <ast/Expr.hpp>
#include <vector>

namespace gulc {
    class SwitchStmt : public Stmt {
    public:
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::Switch; }

        Expr* condition;
        // We keep this out as it might be modified...
        std::vector<Stmt*> statements;

        SwitchStmt(TextPosition startPosition, TextPosition endPosition, Expr* condition, std::vector<Stmt*> statements)
                : Stmt(Stmt::Kind::Switch),
                  condition(condition), statements(std::move(statements)) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Stmt* deepCopy() const override {
            std::vector<Stmt*> copiedStatements;
            copiedStatements.reserve(statements.size());

            for (Stmt* statement : statements) {
                copiedStatements.push_back(statement->deepCopy());
            }

            return new SwitchStmt(_startPosition, _endPosition, condition->deepCopy(),
                                  copiedStatements);
        }

        ~SwitchStmt() override {
            delete condition;

            for (Stmt* statement : statements) {
                delete statement;
            }
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_SWITCHSTMT_HPP
