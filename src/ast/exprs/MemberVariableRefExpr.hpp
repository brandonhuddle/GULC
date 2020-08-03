#ifndef GULC_MEMBERVARIABLEREFEXPR_HPP
#define GULC_MEMBERVARIABLEREFEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/decls/VariableDecl.hpp>

namespace gulc {
    class MemberVariableRefExpr : public Expr {
    public:
        static bool classof(const Expr *expr) { return expr->getExprKind() == Kind::MemberVariableRef; }

        // `self`, `obj`, `*somePtr`
        Expr* object;

        MemberVariableRefExpr(TextPosition startPosition, TextPosition endPosition,
                              Expr* object, gulc::VariableDecl* variableDecl)
                : Expr(Expr::Kind::MemberVariableRef),
                  _startPosition(startPosition), _endPosition(endPosition),
                  object(object), _variableDecl(variableDecl) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        gulc::VariableDecl* variableDecl() const { return _variableDecl; }

        Expr* deepCopy() const override {
            auto result = new MemberVariableRefExpr(_startPosition, _endPosition,
                                                    object->deepCopy(), _variableDecl);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return object->toString() + "." + _variableDecl->identifier().name();
        }

        ~MemberVariableRefExpr() override {
            delete object;
        }

    private:
        TextPosition _startPosition;
        TextPosition _endPosition;
        gulc::VariableDecl* _variableDecl;

    };
}

#endif //GULC_MEMBERVARIABLEREFEXPR_HPP
