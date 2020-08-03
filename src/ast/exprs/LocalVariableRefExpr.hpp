#ifndef GULC_LOCALVARIABLEREFEXPR_HPP
#define GULC_LOCALVARIABLEREFEXPR_HPP

#include <ast/Expr.hpp>

namespace gulc {
    class LocalVariableRefExpr : public Expr {
    public:
        static bool classof(const Expr *expr) { return expr->getExprKind() == Kind::LocalVariableRef; }

        LocalVariableRefExpr(TextPosition startPosition, TextPosition endPosition, std::string variableName)
                : Expr(Expr::Kind::LocalVariableRef),
                  _startPosition(startPosition), _endPosition(endPosition), _variableName(std::move(variableName)) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        std::string const& variableName() const { return _variableName; }

        Expr* deepCopy() const override {
            auto result = new LocalVariableRefExpr(_startPosition, _endPosition,
                                                   _variableName);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return _variableName;
        }

    private:
        TextPosition _startPosition;
        TextPosition _endPosition;
        // Two alternatives I though about going with:
        //  1. Keep a reference to `VariableDeclExpr`, decided against it since a `deepCopy` on a `FunctionDecl` could
        //     invalidate all `LocalVariableRefExpr`s with that, requiring us to manually redo the bodies
        //  2. Store the index of the local variable in the current context, decided against it to allow easier
        //     variable elision without needing to redo all indexes
        // I don't like using the variable name but we can optimize it somewhat through a map to improve lookup times.
        // We might not even need a lookup, we might be able to just output the actual variables reference in IR
        // through variable names still. We will see.
        std::string _variableName;

    };
}

#endif //GULC_LOCALVARIABLEREFEXPR_HPP
