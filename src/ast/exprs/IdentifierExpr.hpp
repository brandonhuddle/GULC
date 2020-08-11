/*
 * Copyright (C) 2020 Brandon Huddle
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
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
