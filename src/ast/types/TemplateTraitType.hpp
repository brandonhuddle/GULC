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
#ifndef GULC_TEMPLATETRAITTYPE_HPP
#define GULC_TEMPLATETRAITTYPE_HPP

#include <ast/Type.hpp>
#include <ast/decls/TemplateTraitDecl.hpp>

namespace gulc {
    /**
     * `TemplateTraitType` is made to be used within uninstantiated template contexts.
     *
     * We use this type to allow us to resolve `TemplatedType` to its exact `Template*Decl` without having to
     * completely instantiate it. This makes it easier to validate templates while still making it easy to instantiate
     * templates (i.e. we will replace all `TemplateTraitType` with `TraitType` after instantiating the template it is
     * stored in).
     */
    class TemplateTraitType : public Type {
    public:
        static bool classof(const Type* type) { return type->getTypeKind() == Type::Kind::TemplateTrait; }

        TemplateTraitType(Qualifier qualifier, std::vector<Expr*> templateArguments, TemplateTraitDecl* decl,
                          TextPosition startPosition, TextPosition endPosition)
                : Type(Type::Kind::TemplateTrait, qualifier, false),
                  _templateArguments(std::move(templateArguments)), _decl(decl),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        std::vector<Expr*>& templateArguments() { return _templateArguments; }
        std::vector<Expr*> const& templateArguments() const { return _templateArguments; }
        TemplateTraitDecl* decl() { return _decl; }
        TemplateTraitDecl const* decl() const { return _decl; }
        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        std::string toString() const override {
            return _decl->identifier().name();
        }

        Type* deepCopy() const override {
            std::vector<Expr*> copiedTemplateArguments;

            for (Expr* templateArgument : _templateArguments) {
                copiedTemplateArguments.push_back(templateArgument->deepCopy());
            }

            auto result = new TemplateTraitType(_qualifier, copiedTemplateArguments, _decl,
                                                _startPosition, _endPosition);
            result->setIsLValue(_isLValue);
            return result;
        }

        ~TemplateTraitType() override {
            for (Expr* templateArgument : _templateArguments) {
                delete templateArgument;
            }
        }

    protected:
        std::vector<Expr*> _templateArguments;
        TemplateTraitDecl* _decl;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_TEMPLATETRAITTYPE_HPP
