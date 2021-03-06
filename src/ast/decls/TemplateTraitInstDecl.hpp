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
#ifndef GULC_TEMPLATETRAITINSTDECL_HPP
#define GULC_TEMPLATETRAITINSTDECL_HPP

#include <ast/Expr.hpp>
#include "TraitDecl.hpp"

namespace gulc {
    class TemplateTraitDecl;

    class TemplateTraitInstDecl : public TraitDecl {
        friend TemplateTraitDecl;

    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::TemplateTraitInst; }

        std::vector<Expr*>& templateArguments() { return _templateArguments; }
        std::vector<Expr*> const& templateArguments() const { return _templateArguments; }
        TemplateTraitDecl* parentTemplateTrait() const { return _parentTemplateTrait; }

        Decl* deepCopy() const override {
            std::vector<Attr*> copiedAttributes;
            copiedAttributes.reserve(_attributes.size());
            std::vector<Type*> copiedInheritedTypes;
            copiedInheritedTypes.reserve(_inheritedTypes.size());
            std::vector<Cont*> copiedContracts;
            copiedContracts.reserve(_contracts.size());
            std::vector<Decl*> copiedOwnedMembers;
            copiedOwnedMembers.reserve(_ownedMembers.size());
            std::vector<Expr*> copiedTemplateArguments;
            copiedTemplateArguments.reserve(_templateArguments.size());

            for (Attr* attribute : _attributes) {
                copiedAttributes.push_back(attribute->deepCopy());
            }

            for (Type* inheritedType : _inheritedTypes) {
                copiedInheritedTypes.push_back(inheritedType->deepCopy());
            }

            for (Cont* contract : _contracts) {
                copiedContracts.push_back(contract->deepCopy());
            }

            for (Decl* ownedMember : _ownedMembers) {
                copiedOwnedMembers.push_back(ownedMember->deepCopy());
            }

            for (Expr* templateArgument : _templateArguments) {
                copiedTemplateArguments.push_back(templateArgument->deepCopy());
            }

            auto result = new TemplateTraitInstDecl(_sourceFileID, copiedAttributes, _declVisibility, _isConstExpr,
                                                    _identifier, _declModifiers,
                                                    _startPosition, _endPosition,
                                                    copiedInheritedTypes, copiedContracts, copiedOwnedMembers,
                                                    _parentTemplateTrait, copiedTemplateArguments);
            result->container = container;
            result->containedInTemplate = containedInTemplate;
            result->containerTemplateType = (containerTemplateType == nullptr ? nullptr : containerTemplateType->deepCopy());
            result->originalDecl = (originalDecl == nullptr ? this : originalDecl);
            return result;
        }

        ~TemplateTraitInstDecl() override {
            for (Expr* templateArgument : _templateArguments) {
                delete templateArgument;
            }
        }

    protected:
        TemplateTraitDecl* _parentTemplateTrait;
        std::vector<Expr*> _templateArguments;

        TemplateTraitInstDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                               bool isConstExpr, Identifier identifier, DeclModifiers declModifiers,
                               TextPosition startPosition, TextPosition endPosition,
                               std::vector<Type*> inheritedTypes, std::vector<Cont*> contracts,
                               std::vector<Decl*> ownedMembers, TemplateTraitDecl* parentTemplateTrait,
                               std::vector<Expr*> templateArguments)
                : TraitDecl(Decl::Kind::TemplateTraitInst, sourceFileID, std::move(attributes), visibility,
                             isConstExpr, std::move(identifier), declModifiers, startPosition, endPosition,
                             std::move(inheritedTypes), std::move(contracts), std::move(ownedMembers)),
                  _parentTemplateTrait(parentTemplateTrait), _templateArguments(std::move(templateArguments)) {}

    };
}

#endif //GULC_TEMPLATETRAITINSTDECL_HPP
