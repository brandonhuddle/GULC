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
#ifndef GULC_TRAITDECL_HPP
#define GULC_TRAITDECL_HPP

#include <ast/Decl.hpp>
#include <ast/Type.hpp>
#include <ast/Cont.hpp>
#include <set>

namespace gulc {
    // TODO: For traits we'll have to decide how we want to handle their actual implementation. With a trait you're
    //       allowed to provide default implementations like so:
    //           trait ToString {
    //               func toString() { return "trait `ToString` not properly implemented!"; }
    //           }
    //       For this, we will have to allow `toString` to have a `this` reference like any member function. We need to
    //       decide if we want to automatically create a specialized version of `ToString::toString()` for anything
    //       that implements it of if we want to only create the generic one and implicitly cast to trait `ToString`
    //       when the function is called.
    //       I think we should go the route of automatic specialization. There's no need to cast to `ToString` unless
    //       it is explicitly required, casting in the background is an extra cost (since we have to construct a vtable)
    //       For us to fully decide on how we do this we need to know the layout for a trait type. I think the below
    //       could be a good starting point:
    //           struct BackendTraitLayout<T> {
    //               var ptr: *mut T
    //               var vtable: *func(_: &T)
    //           }
    //       The `vtable` is constructed for the trait using the implementations from `T`
    //       We pass `ptr` as `self` to any of the vtable functions unchanged. `(*vtable)(ptr, ...)`
    //       We will need to know the lifetime of `ptr` for this to be considered `safe`, if we don't it would be
    //       possible to store references to stack pointers that will be overwritten.
    class TraitDecl : public Decl {
    public:
        static bool classof(const Decl* decl) {
            Decl::Kind checkKind = decl->getDeclKind();

            return checkKind == Decl::Kind::Trait || checkKind == Decl::Kind::TemplateTraitInst;
        }

        TraitDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                  bool isConstExpr, Identifier identifier, DeclModifiers declModifiers,
                  TextPosition startPosition, TextPosition endPosition, std::vector<Type*> inheritedTypes,
                  std::vector<Cont*> contracts, std::vector<Decl*> ownedMembers)
                : TraitDecl(Decl::Kind::Trait, sourceFileID, std::move(attributes), visibility, isConstExpr,
                            std::move(identifier), declModifiers, startPosition, endPosition,
                            std::move(inheritedTypes), std::move(contracts), std::move(ownedMembers)) {}

        std::vector<Type*>& inheritedTypes() { return _inheritedTypes; }
        std::vector<Type*> const& inheritedTypes() const { return _inheritedTypes; }
        std::vector<Cont*>& contracts() { return _contracts; }
        std::vector<Cont*> const& contracts() const { return _contracts; }
        std::vector<Decl*>& ownedMembers() { return _ownedMembers; }
        std::vector<Decl*> const& ownedMembers() const { return _ownedMembers; }

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

            auto result = new TraitDecl(_sourceFileID, copiedAttributes, _declVisibility, _isConstExpr,
                                        _identifier, _declModifiers,
                                        _startPosition, _endPosition,
                                        copiedInheritedTypes, copiedContracts, copiedOwnedMembers);
            result->container = container;
            result->containedInTemplate = containedInTemplate;
            result->containerTemplateType = (containerTemplateType == nullptr ? nullptr : containerTemplateType->deepCopy());
            result->originalDecl = (originalDecl == nullptr ? this : originalDecl);
            return result;
        }

        std::string getPrototypeString() const override {
            std::string result = getDeclModifiersString(_declModifiers);

            if (!result.empty()) result += " ";

            result += "trait " + _identifier.name();

            return result;
        }

        ~TraitDecl() override {
            for (Type* inheritedType : _inheritedTypes) {
                delete inheritedType;
            }

            for (Cont* contract : _contracts) {
                delete contract;
            }

            for (Decl* member : _ownedMembers) {
                delete member;
            }
        }

        // If the type is contained within a template this is a `Type` that will resolve to the container
        // The reason we need this is to make it easier properly resolve types to `Decl`s when they are contained
        // within templates. Without this we would need to manually search the `container` backwards looking for any
        // templates for every single type that resolves to a Decl
        Type* containerTemplateType;
        // List of all known members including our inherited members
        // NOTE: None of these pointers are owned by us so we don't free them
        // TODO: How will we handle shadows? Should we just consider them to be the same function?
        std::vector<Decl*> allMembers;

        // This is used to know if this trait has passed through `DeclInstantiator`
        bool isInstantiated = false;
        // Inherited types MIGHT need to be initialized before the entire struct, to account for that we have a
        // separate field to account for its initialization
        bool inheritedTypesIsInitialized = false;

    protected:
        TraitDecl(Decl::Kind declKind, unsigned int sourceFileID, std::vector<Attr*> attributes,
                  Decl::Visibility visibility, bool isConstExpr, Identifier identifier, DeclModifiers declModifiers,
                  TextPosition startPosition, TextPosition endPosition, std::vector<Type*> inheritedTypes,
                  std::vector<Cont*> contracts, std::vector<Decl*> ownedMembers)
                : Decl(declKind, sourceFileID, std::move(attributes), visibility, isConstExpr, std::move(identifier),
                       declModifiers),
                  containerTemplateType(), _startPosition(startPosition), _endPosition(endPosition),
                  _inheritedTypes(std::move(inheritedTypes)), _contracts(std::move(contracts)),
                  _ownedMembers(std::move(ownedMembers)) {
            for (Decl* ownedMember : _ownedMembers) {
                ownedMember->container = this;
            }
        }

        TextPosition _startPosition;
        TextPosition _endPosition;
        // This a list of inherited traits, this CANNOT contain anything but traits
        std::vector<Type*> _inheritedTypes;
        std::vector<Cont*> _contracts;
        // This is a list of ALL members; including static, const, AND instance members
        std::vector<Decl*> _ownedMembers;

    };
}

#endif //GULC_TRAITDECL_HPP
