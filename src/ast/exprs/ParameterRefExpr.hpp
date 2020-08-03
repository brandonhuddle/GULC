#ifndef GULC_PARAMETERREFEXPR_HPP
#define GULC_PARAMETERREFEXPR_HPP

#include <ast/Expr.hpp>

namespace gulc {
    class ParameterRefExpr : public Expr {
    public:
        static bool classof(const Expr *expr) { return expr->getExprKind() == Kind::ParameterRef; }

        ParameterRefExpr(TextPosition startPosition, TextPosition endPosition, std::string parameterRef)
                : Expr(Expr::Kind::ParameterRef),
                  _startPosition(startPosition), _endPosition(endPosition), _parameterRef(std::move(parameterRef)) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        std::string const& parameterRef() const { return _parameterRef; }

        Expr* deepCopy() const override {
            auto result = new ParameterRefExpr(_startPosition, _endPosition,
                                               _parameterRef);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return _parameterRef;
        }

    private:
        TextPosition _startPosition;
        TextPosition _endPosition;
        std::string _parameterRef;

    };
}

#endif //GULC_PARAMETERREFEXPR_HPP
