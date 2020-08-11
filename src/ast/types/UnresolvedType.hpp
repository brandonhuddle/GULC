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
#ifndef GULC_UNRESOLVEDTYPE_HPP
#define GULC_UNRESOLVEDTYPE_HPP

#include <ast/Type.hpp>
#include <vector>
#include <ast/Identifier.hpp>
#include <ast/Expr.hpp>

namespace gulc {
    class UnresolvedType : public Type {
    public:
        static bool classof(const Type* type) { return type->getTypeKind() == Type::Kind::Unresolved; }

        UnresolvedType(Qualifier qualifier, std::vector<Identifier> namespacePath, Identifier identifier,
                       std::vector<Expr*> templateArguments)
                : Type(Type::Kind::Unresolved, qualifier, false),
                  _namespacePath(std::move(namespacePath)), _identifier(std::move(identifier)),
                  templateArguments(std::move(templateArguments)) {}

        std::vector<Identifier> const& namespacePath() const { return _namespacePath; }
        Identifier const& identifier() const { return _identifier; }
        std::vector<Expr*> templateArguments;
        bool hasTemplateArguments() const { return !templateArguments.empty(); }

        TextPosition startPosition() const override {
            if (!_namespacePath.empty()) {
                return _namespacePath.front().startPosition();
            } else {
                return _identifier.startPosition();
            }
        }

        TextPosition endPosition() const override {
            return _identifier.endPosition();
        }

        std::string toString() const override {
            std::string result;
            std::string templateArgsString;

            for (Identifier const& namespaceIdentifier : _namespacePath) {
                result += namespaceIdentifier.name() + ".";
            }

            if (!templateArguments.empty()) {
                templateArgsString = "<";

                for (std::size_t i = 0; i < templateArguments.size(); ++i) {
                    if (i != 0) templateArgsString += ", ";

                    templateArgsString += templateArguments[i]->toString();
                }

                templateArgsString += ">";
            }

            return result + _identifier.name() + templateArgsString;
        }

        Type* deepCopy() const override {
            std::vector<Expr*> copiedTemplateArguments;

            for (Expr* templateArgument : templateArguments) {
                copiedTemplateArguments.push_back(templateArgument->deepCopy());
            }

            auto result = new UnresolvedType(_qualifier, _namespacePath, _identifier,
                                             copiedTemplateArguments);
            result->setIsLValue(_isLValue);
            return result;
        }

        ~UnresolvedType() override {
            for (Expr* templateArgument : templateArguments) {
                delete templateArgument;
            }
        }

    protected:
        std::vector<Identifier> _namespacePath;
        Identifier _identifier;

    };
}

#endif //GULC_UNRESOLVEDTYPE_HPP
