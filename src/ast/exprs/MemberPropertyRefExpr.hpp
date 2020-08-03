#ifndef GULC_MEMBERPROPERTYREFEXPR_HPP
#define GULC_MEMBERPROPERTYREFEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/decls/PropertyDecl.hpp>

namespace gulc {
    class MemberPropertyRefExpr : public Expr {
    public:
        static bool classof(const Expr *expr) { return expr->getExprKind() == Kind::MemberPropertyRef; }

        // `self`, `obj`, `*somePtr`
        Expr* object;

        MemberPropertyRefExpr(TextPosition startPosition, TextPosition endPosition,
                              Expr* object, gulc::PropertyDecl* propertyDecl)
                : Expr(Expr::Kind::MemberPropertyRef),
                  _startPosition(startPosition), _endPosition(endPosition),
                  object(object), _propertyDecl(propertyDecl) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        gulc::PropertyDecl* propertyDecl() const { return _propertyDecl; }

        Expr* deepCopy() const override {
            auto result = new MemberPropertyRefExpr(_startPosition, _endPosition,
                                                    object->deepCopy(), _propertyDecl);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return object->toString() + "." + _propertyDecl->identifier().name();
        }

        ~MemberPropertyRefExpr() override {
            delete object;
        }

    private:
        TextPosition _startPosition;
        TextPosition _endPosition;
        gulc::PropertyDecl* _propertyDecl;

    };
}

#endif //GULC_MEMBERPROPERTYREFEXPR_HPP
