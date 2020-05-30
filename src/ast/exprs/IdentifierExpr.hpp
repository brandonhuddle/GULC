#ifndef GULC_IDENTIFIEREXPR_HPP
#define GULC_IDENTIFIEREXPR_HPP

#include <ast/Expr.hpp>
#include <ast/Identifier.hpp>
#include <vector>
#include <ast/Decl.hpp>
#include "VariableDeclExpr.hpp"

namespace gulc {
    class IdentifierExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::Identifier; }

        IdentifierExpr(Identifier identifier, std::vector<Expr*> templateArguments)
                : Expr(Expr::Kind::Identifier),
                  _identifier(std::move(identifier)), _templateArguments(std::move(templateArguments)) {}

        Identifier const& identifier() const { return _identifier; }
        std::vector<Expr*>& templateArguments() { return _templateArguments; }
        std::vector<Expr*> const& templateArguments() const { return _templateArguments; }
        bool hasTemplateArguments() { return !_templateArguments.empty(); }

        TextPosition startPosition() const override { return _identifier.startPosition(); }
        TextPosition endPosition() const override { return _identifier.endPosition(); }

        Expr* deepCopy() const override {
            std::vector<Expr*> copiedTemplateArguments;
            copiedTemplateArguments.reserve(_templateArguments.size());

            for (Expr* templateArgument : _templateArguments) {
                copiedTemplateArguments.push_back(templateArgument->deepCopy());
            }

            auto result = new IdentifierExpr(_identifier, copiedTemplateArguments);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            std::string templateArgumentsString;

            if (!_templateArguments.empty()) {
                templateArgumentsString += "<";

                for (auto templateArgument : _templateArguments) {
                    templateArgumentsString += templateArgument->toString();
                }

                templateArgumentsString += ">";
            }

            return _identifier.name() + templateArgumentsString;
        }

        ~IdentifierExpr() override {
            for (Expr* templateArgument : _templateArguments) {
                delete templateArgument;
            }
        }

    protected:
        Identifier _identifier;
        std::vector<Expr*> _templateArguments;

    };
}

#endif //GULC_IDENTIFIEREXPR_HPP
