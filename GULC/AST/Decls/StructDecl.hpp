// Copyright (C) 2019 Michael Brandon Huddle
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef GULC_STRUCTDECL_HPP
#define GULC_STRUCTDECL_HPP

#include <AST/Decl.hpp>
#include <vector>
#include "GlobalVariableDecl.hpp"
#include "ConstructorDecl.hpp"
#include "DestructorDecl.hpp"

namespace gulc {
    class StructDecl : public Decl {
    public:
        static bool classof(const Decl *decl) { return decl->getDeclKind() == Kind::Struct; }

        StructDecl(std::string name, std::string sourceFile, TextPosition startPosition, TextPosition endPosition,
                   Visibility visibility, std::vector<Type*> baseTypes, std::vector<ConstructorDecl*> constructors,
                   std::vector<Decl*> members, DestructorDecl* destructor)
                : Decl(Kind::Struct, std::move(name), std::move(sourceFile), startPosition, endPosition,
                       visibility),
                  baseTypes(std::move(baseTypes)), constructors(std::move(constructors)), members(std::move(members)),
                  destructor(destructor) {}

        // These are just sorted references to already existing members within the `members` list, we do NOT free these as that would cause a double free issue.
        std::vector<GlobalVariableDecl*> dataMembers;

        std::vector<Type*> baseTypes;
        // This stores our direct parent struct if there are any parent structs in the `baseTypes` list
        // We don't own this so we don't free it
        StructDecl* baseStruct;
        std::vector<ConstructorDecl*> constructors;
        std::vector<Decl*> members;
        // There can only be one destructor
        DestructorDecl* destructor;

        // This is a list of members inherited from the base struct and all bases of base structs
        // This will also remove any overridden functions, shadowed members, and unaccessible members
        // We do not own this so we don't free it
        std::vector<Decl*> inheritedMembers;

        Decl* deepCopy() const override {
            std::vector<Type*> copiedBaseTypes{};
            std::vector<ConstructorDecl*> copiedConstructors{};
            std::vector<Decl*> copiedMembers{};
            DestructorDecl* copiedDestructor = nullptr;

            for (Type* baseType : baseTypes) {
                copiedBaseTypes.push_back(baseType->deepCopy());
            }

            for (ConstructorDecl* constructor : constructors) {
                copiedConstructors.push_back(llvm::dyn_cast<ConstructorDecl>(constructor->deepCopy()));
            }

            for (Decl* decl : members) {
                copiedMembers.push_back(decl->deepCopy());
            }

            if (destructor) {
                copiedDestructor = llvm::dyn_cast<DestructorDecl>(destructor->deepCopy());
            }

            auto result = new StructDecl(name(), sourceFile(),
                                         startPosition(), endPosition(),
                                         visibility(), std::move(copiedBaseTypes),
                                         std::move(copiedConstructors), std::move(copiedMembers), copiedDestructor);
            result->parentNamespace = parentNamespace;
            result->parentStruct = parentStruct;
            result->baseStruct = baseStruct;
            result->inheritedMembers = std::vector<Decl*>(inheritedMembers);

            return result;
        }

        ~StructDecl() override {
            delete destructor;

            for (Type* baseType : baseTypes) {
                delete baseType;
            }

            for (ConstructorDecl* constructor : constructors) {
                delete constructor;
            }

            for (Decl* decl : members) {
                delete decl;
            }
        }

    };
}

#endif //GULC_STRUCTDECL_HPP
