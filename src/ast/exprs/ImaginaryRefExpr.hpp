#ifndef GULC_IMAGINARYREFEXPR_HPP
#define GULC_IMAGINARYREFEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/decls/TemplateParameterDecl.hpp>

namespace gulc {
    // References an imaginary variable (i.e. a template const parameter `struct example<const i: usize>`)
    class ImaginaryRefExpr : public Expr {
    public:
        static bool classof(const Expr *expr) { return expr->getExprKind() == Kind::ImaginaryRef; }

        TemplateParameterDecl* templateParameter;

        explicit ImaginaryRefExpr(TemplateParameterDecl* templateParameter)
                : Expr(Expr::Kind::ImaginaryRef), templateParameter(templateParameter) {}

        TextPosition startPosition() const override { return templateParameter->startPosition(); }
        TextPosition endPosition() const override { return templateParameter->endPosition(); }

        Expr* deepCopy() const override {
            auto result = new ImaginaryRefExpr(templateParameter);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return templateParameter->identifier().name();
        }

    };
}

#endif //GULC_IMAGINARYREFEXPR_HPP
