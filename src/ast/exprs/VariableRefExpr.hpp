#ifndef GULC_VARIABLEREFEXPR_HPP
#define GULC_VARIABLEREFEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/decls/VariableDecl.hpp>

namespace gulc {
    class VariableRefExpr : public Expr {
    public:
        static bool classof(const Expr *expr) { return expr->getExprKind() == Kind::VariableRef; }

        VariableDecl* variableDecl;

        VariableRefExpr(TextPosition startPosition, TextPosition endPosition,
                        VariableDecl* variableDecl)
                : Expr(Expr::Kind::VariableRef),
                  _startPosition(startPosition), _endPosition(endPosition),
                  variableDecl(variableDecl) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Expr* deepCopy() const override {
            auto result = new VariableRefExpr(_startPosition, _endPosition,
                                              variableDecl);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return variableDecl->identifier().name();
        }

    private:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_VARIABLEREFEXPR_HPP
