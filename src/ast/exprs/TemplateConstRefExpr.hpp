#ifndef GULC_TEMPLATECONSTREFEXPR_HPP
#define GULC_TEMPLATECONSTREFEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/decls/TemplateParameterDecl.hpp>

namespace gulc {
    class TemplateConstRefExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::TemplateConstRefExpr; }

        explicit TemplateConstRefExpr(TemplateParameterDecl* templateParameter)
                : Expr(Expr::Kind::TemplateConstRefExpr), _templateParameter(templateParameter) {}

        TemplateParameterDecl* templateParameter() { return _templateParameter; }

        TextPosition startPosition() const override { return _templateParameter->startPosition(); }
        TextPosition endPosition() const override { return _templateParameter->endPosition(); }

        Expr* deepCopy() const override {
            return new TemplateConstRefExpr(_templateParameter);
        }

    protected:
        TemplateParameterDecl* _templateParameter;

    };
}

#endif //GULC_TEMPLATECONSTREFEXPR_HPP
