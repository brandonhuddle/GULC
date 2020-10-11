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
#ifndef GULC_IMAGINARYTYPEDECL_HPP
#define GULC_IMAGINARYTYPEDECL_HPP

#include <ast/Decl.hpp>
#include <ast/types/TraitType.hpp>
#include "ConstructorDecl.hpp"
#include "DestructorDecl.hpp"

namespace gulc {
    // `ImaginaryTypeDecl` is used to store all information needed for `ImaginaryType`. This is only a band-aid for the fact
    // that `Type` is copied for all references to `Type`.
    // The `ImaginaryTypeDecl` is used to describe what can be done with a template type through the `where` clause.
    // I.e. if you do `where T has trait Addable` then `ImaginaryTypeDecl` will state that it implements `Addable` and all
    //      members in `Addable` will be accessible through this.
    class ImaginaryTypeDecl : public Decl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::ImaginaryType; }

        ImaginaryTypeDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                          bool isConstExpr, Identifier identifier, TextPosition startPosition, TextPosition endPosition,
                          Type* baseType, std::vector<TraitType*> inheritedTraits, std::vector<Decl*> ownedMembers,
                          std::vector<ConstructorDecl*> constructors, DestructorDecl* destructor)
                : Decl(Decl::Kind::ImaginaryType, sourceFileID, std::move(attributes), visibility, isConstExpr,
                       std::move(identifier), DeclModifiers::None),
                  _startPosition(startPosition), _endPosition(endPosition),
                  _inheritedTraits(std::move(inheritedTraits)), _baseType(baseType),
                  _ownedMembers(std::move(ownedMembers)), _constructors(std::move(constructors)),
                  _destructor(destructor) {}

        std::vector<TraitType*>& inheritedTraits() { return _inheritedTraits; }
        std::vector<TraitType*> const& inheritedTraits() const { return _inheritedTraits; }
        Type* baseType() const { return _baseType; }
        std::vector<Decl*>& ownedMembers() { return _ownedMembers; }
        std::vector<Decl*> const& ownedMembers() const { return _ownedMembers; }
        std::vector<ConstructorDecl*>& constructors() { return _constructors; }
        std::vector<ConstructorDecl*> const& constructors() const { return _constructors; }
        DestructorDecl* destructor() const { return _destructor; }

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Decl* deepCopy() const override {
            std::vector<TraitType*> copiedInheritedTraits;
            copiedInheritedTraits.reserve(_inheritedTraits.size());
            std::vector<Decl*> copiedOwnedMembers;
            copiedOwnedMembers.reserve(_ownedMembers.size());
            std::vector<ConstructorDecl*> copiedConstructors;
            copiedConstructors.reserve(_constructors.size());
            Type* copiedBaseType = nullptr;
            DestructorDecl* copiedDestructor = nullptr;

            for (TraitType* inheritedTrait : _inheritedTraits) {
                copiedInheritedTraits.push_back(llvm::dyn_cast<TraitType>(inheritedTrait->deepCopy()));
            }

            if (_baseType != nullptr) {
                copiedBaseType = _baseType->deepCopy();
            }

            for (Decl* ownedMember : _ownedMembers) {
                copiedOwnedMembers.push_back(ownedMember->deepCopy());
            }

            for (ConstructorDecl* constructor : _constructors) {
                copiedConstructors.push_back(llvm::dyn_cast<ConstructorDecl>(constructor->deepCopy()));
            }

            if (_destructor != nullptr) {
                copiedDestructor = llvm::dyn_cast<DestructorDecl>(_destructor->deepCopy());
            }

            auto result = new ImaginaryTypeDecl(_sourceFileID, _attributes, _declVisibility, _isConstExpr,
                                                _identifier, _startPosition, _endPosition,
                                                copiedBaseType, copiedInheritedTraits, copiedOwnedMembers,
                                                copiedConstructors, copiedDestructor);
            result->container = container;
            result->containedInTemplate = containedInTemplate;
//            result->containerTemplateType = (containerTemplateType == nullptr ? nullptr : containerTemplateType->deepCopy());
            result->originalDecl = (originalDecl == nullptr ? this : originalDecl);
            return result;
        }

        std::string getPrototypeString() const override {
            // For the prototype I think it is just the template type name. Or should we also add the base type when
            // specified?
            return _identifier.name();
        }

        ~ImaginaryTypeDecl() override {
            for (TraitType* traitType : _inheritedTraits) {
                delete traitType;
            }

            delete _baseType;

            for (Decl* ownedMember : _ownedMembers) {
                delete ownedMember;
            }

            for (ConstructorDecl* constructor : _constructors) {
                delete constructor;
            }

            delete _destructor;
        }

        // This is a list of all declared members and all members from traits
        std::vector<Decl*> allMembers;

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;
        std::vector<TraitType*> _inheritedTraits;
        // NOTE: Unlike `StructDecl` this CAN be `i32` etc. This is set through template specialization, e.g.
        //       `struct example<T: i32> {}`
        Type* _baseType;
        // This is a list of declared members. I.e. `where T has func example()`, this does NOT contain members from
        // `where T has trait Addable`
        std::vector<Decl*> _ownedMembers;
        std::vector<ConstructorDecl*> _constructors;
        DestructorDecl* _destructor;

    };
}

#endif //GULC_IMAGINARYTYPEDECL_HPP
