#ifndef GULC_PROPERTYREFEXPR_HPP
#define GULC_PROPERTYREFEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/decls/PropertyDecl.hpp>

namespace gulc {
    /// Same as a function reference in essence. It is NOT the final `PropertyGetCall` or `PropertySetCall` but the
    /// expression that points `PropertyGetCall` and `PropertySetCall` to the underlying `Property`
    class PropertyRefExpr : public Expr {
    public:
        static bool classof(const Expr *expr) { return expr->getExprKind() == Kind::PropertyRef; }

        PropertyDecl* propertyDecl;

        PropertyRefExpr(TextPosition startPosition, TextPosition endPosition,
                        PropertyDecl* propertyDecl)
                : Expr(Expr::Kind::PropertyRef),
                  _startPosition(startPosition), _endPosition(endPosition),
                  propertyDecl(propertyDecl) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Expr* deepCopy() const override {
            auto result = new PropertyRefExpr(_startPosition, _endPosition,
                                              propertyDecl);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return propertyDecl->identifier().name();
        }

    private:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_PROPERTYREFEXPR_HPP
