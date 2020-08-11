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
#ifndef GULC_EXTENSIONDECL_HPP
#define GULC_EXTENSIONDECL_HPP

#include <ast/Decl.hpp>
#include <ast/Type.hpp>
#include <ast/Cont.hpp>
#include <llvm/Support/Casting.h>
#include "TemplateParameterDecl.hpp"
#include "ConstructorDecl.hpp"

namespace gulc {
    class ExtensionDecl : public Decl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::Extension; }

        Type* typeToExtend;

        ExtensionDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                      bool isConstExpr, DeclModifiers declModifiers,
                      Type* typeToExtend,
                      TextPosition startPosition, TextPosition endPosition,
                      std::vector<Type*> inheritedTypes, std::vector<Cont*> contracts, std::vector<Decl*> ownedMembers,
                      std::vector<ConstructorDecl*> constructors)
                : Decl(Decl::Kind::Extension, sourceFileID, std::move(attributes), visibility, isConstExpr,
                       Identifier({}, {}, "_"), declModifiers),
                  typeToExtend(typeToExtend),
                  _startPosition(startPosition), _endPosition(endPosition), _inheritedTypes(std::move(inheritedTypes)),
                  _contracts(std::move(contracts)), _ownedMembers(std::move(ownedMembers)),
                  _constructors(std::move(constructors)) {
            for (Decl* ownedMember : _ownedMembers) {
                ownedMember->container = this;
            }

            for (ConstructorDecl* constructor : _constructors) {
                constructor->container = this;
            }
        }

        std::vector<Type*>& inheritedTypes() { return _inheritedTypes; }
        std::vector<Type*> const& inheritedTypes() const { return _inheritedTypes; }
        std::vector<Cont*>& contracts() { return _contracts; }
        std::vector<Cont*> const& contracts() const { return _contracts; }
        std::vector<Decl*>& ownedMembers() { return _ownedMembers; }
        std::vector<Decl*> const& ownedMembers() const { return _ownedMembers; }
        std::vector<ConstructorDecl*>& constructors() { return _constructors; }
        std::vector<ConstructorDecl*> const& constructors() const { return _constructors; }

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Decl* deepCopy() const override {
            std::vector<Attr*> copiedAttributes;
            copiedAttributes.reserve(_attributes.size());
            std::vector<Type*> copiedInheritedTypes;
            copiedInheritedTypes.reserve(_inheritedTypes.size());
            std::vector<Cont*> copiedContracts;
            copiedContracts.reserve(_contracts.size());
            std::vector<Decl*> copiedOwnedMembers;
            copiedOwnedMembers.reserve(_ownedMembers.size());
            std::vector<ConstructorDecl*> copiedConstructors;
            copiedConstructors.reserve(_constructors.size());

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

            for (ConstructorDecl* constructor : _constructors) {
                copiedConstructors.push_back(llvm::dyn_cast<ConstructorDecl>(constructor->deepCopy()));
            }

            auto result = new ExtensionDecl(_sourceFileID, copiedAttributes, _declVisibility, _isConstExpr, _declModifiers,
                                            typeToExtend->deepCopy(),
                                            _startPosition, _endPosition,
                                            copiedInheritedTypes, copiedContracts, copiedOwnedMembers, copiedConstructors);
            result->container = container;
            result->containedInTemplate = containedInTemplate;
            result->originalDecl = (originalDecl == nullptr ? this : originalDecl);
            return result;
        }

        ~ExtensionDecl() override {
            delete typeToExtend;

            for (Type* inheritedType : _inheritedTypes) {
                delete inheritedType;
            }

            for (Cont* contract : _contracts) {
                delete contract;
            }

            for (Decl* ownedMember : _ownedMembers) {
                delete ownedMember;
            }

            for (ConstructorDecl* constructor : _constructors) {
                delete constructor;
            }
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;
        // This a list of inherited traits, this CANNOT contain anything but traits
        std::vector<Type*> _inheritedTypes;
        std::vector<Cont*> _contracts;
        // This is a list of ALL members; including static, const, AND instance members
        std::vector<Decl*> _ownedMembers;
        std::vector<ConstructorDecl*> _constructors;

    };
}

#endif //GULC_EXTENSIONDECL_HPP
