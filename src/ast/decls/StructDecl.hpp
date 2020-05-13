#ifndef GULC_STRUCTDECL_HPP
#define GULC_STRUCTDECL_HPP

#include <ast/Decl.hpp>
#include <ast/Type.hpp>
#include <set>
#include "ConstructorDecl.hpp"
#include "DestructorDecl.hpp"
#include "VariableDecl.hpp"

namespace gulc {
    class StructDecl : public Decl {
    public:
        static bool classof(const Decl* decl) {
            Decl::Kind checkKind = decl->getDeclKind();

            return checkKind == Decl::Kind::Struct || checkKind == Decl::Kind::TemplateStructInst;
        }

        enum class Kind {
            Struct,
            Class,
            Union
        };

        StructDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                   bool isConstExpr, Identifier identifier, DeclModifiers declModifiers,
                   TextPosition startPosition, TextPosition endPosition,
                   Kind structKind, std::vector<Type*> inheritedTypes, std::vector<Cont*> contracts,
                   std::vector<Decl*> ownedMembers, std::vector<ConstructorDecl*> constructors,
                   DestructorDecl* destructor)
                : StructDecl(Decl::Kind::Struct, sourceFileID, std::move(attributes), visibility, isConstExpr,
                             std::move(identifier), declModifiers, startPosition, endPosition, structKind,
                             std::move(inheritedTypes), std::move(contracts), std::move(ownedMembers),
                             std::move(constructors), destructor) {}

//        bool isClass() const { return _isClass; }
        Kind structKind() const { return _structKind; }
        std::string structKindName() const {
            switch (_structKind) {
                case Kind::Struct:
                    return "struct";
                case Kind::Class:
                    return "class";
                case Kind::Union:
                    return "union";
            }

            return "[UNKNOWN]";
        }

        std::vector<Type*>& inheritedTypes() { return _inheritedTypes; }
        std::vector<Type*> const& inheritedTypes() const { return _inheritedTypes; }
        std::vector<Cont*>& contracts() { return _contracts; }
        std::vector<Cont*> const& contracts() const { return _contracts; }
        std::vector<Decl*>& ownedMembers() { return _ownedMembers; }
        std::vector<Decl*> const& ownedMembers() const { return _ownedMembers; }
        std::vector<ConstructorDecl*>& constructors() { return _constructors; }
        std::vector<ConstructorDecl*> const& constructors() const { return _constructors; }
        DestructorDecl* destructor;

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
            DestructorDecl* copiedDestructorDecl = nullptr;

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

            if (destructor != nullptr) {
                copiedDestructorDecl = llvm::dyn_cast<DestructorDecl>(destructor->deepCopy());
            }

            auto result = new StructDecl(_sourceFileID, copiedAttributes, _declVisibility, _isConstExpr,
                                         _identifier, _declModifiers,
                                         _startPosition, _endPosition, _structKind,
                                         copiedInheritedTypes, copiedContracts, copiedOwnedMembers, copiedConstructors,
                                         copiedDestructorDecl);
            result->container = container;
            result->containedInTemplate = containedInTemplate;
            result->containerTemplateType = (containerTemplateType == nullptr ? nullptr : containerTemplateType->deepCopy());
            return result;
        }

        ~StructDecl() override {
            for (Type* inheritedType : _inheritedTypes) {
                delete inheritedType;
            }

            for (Cont* contract : _contracts) {
                delete contract;
            }

            for (Decl* member : _ownedMembers) {
                delete member;
            }

            for (VariableDecl* ownedPaddingMember : ownedPaddingMembers) {
                delete ownedPaddingMember;
            }

            for (ConstructorDecl* constructor : _constructors) {
                delete constructor;
            }

            delete destructor;
        }

        // If the type is contained within a template this is a `Type` that will resolve to the container
        // The reason we need this is to make it easier properly resolve types to `Decl`s when they are contained
        // within templates. Without this we would need to manually search the `container` backwards looking for any
        // templates for every single type that resolves to a Decl
        Type* containerTemplateType;

        // This is the base struct found in the `inheritedTypes` list (if a base struct was found)
        // We don't own this so we don't free it
        StructDecl* baseStruct;
        // This is used to know if this struct has passed through `DeclInstantiator`
        bool isInstantiated = false;
        // Inherited types MIGHT need to be initialized before the entire struct, to account for that we have a
        // separate field to account for its initialization
        bool inheritedTypesIsInitialized = false;
        // This is a list of the actual data members with padding.
        // NOTE: This does NOT include inherited members! It also does NOT include a member to reference our base,
        //       this must be handled manually!
        std::vector<VariableDecl*> memoryLayout;
        // These are hidden padding members used to align memory in the `memoryLayout`, these are only stored here
        // so that we know to delete them
        std::vector<VariableDecl*> ownedPaddingMembers;
        std::size_t dataSizeWithoutPadding;
        std::size_t dataSizeWithPadding;

        // The virtual function table for this struct
        // NOTE: None of these need to be deleted as they are all stored in other lists that are deleted.
        std::vector<FunctionDecl*> vtable;
        // Since the Itanium spec gives us a mangled name specification for the vtable, we will use it here.
        std::string vtableName;
        // The `StructDecl` that contains the vtable
        // NOTE: Structs only contain ONE vtable in GUL since we only support single inheritance
        //       If a struct inherits a `trait` the functions in that trait are NOT virtual by default so we don't have
        //       to worry about including those functions in our vtable unless the struct explicitly marks those
        //       functions it inherits as `virtual` inside the struct. At which point they will be added to the `vtable`
        //       the same way any other struct is added to the vtable. Constructing the `vtable` for a `trait` will be
        //       handled differently
        StructDecl* vtableOwner;

    protected:
        StructDecl(Decl::Kind declKind, unsigned int sourceFileID, std::vector<Attr*> attributes,
                   Decl::Visibility visibility, bool isConstExpr, Identifier identifier, DeclModifiers declModifiers,
                   TextPosition startPosition, TextPosition endPosition, Kind structKind,
                   std::vector<Type*> inheritedTypes, std::vector<Cont*> contracts, std::vector<Decl*> ownedMembers,
                   std::vector<ConstructorDecl*> constructors, DestructorDecl* destructor)
                : Decl(declKind, sourceFileID, std::move(attributes), visibility, isConstExpr, std::move(identifier),
                       declModifiers),
                  containerTemplateType(), baseStruct(nullptr), memoryLayout(), dataSizeWithoutPadding(0),
                  dataSizeWithPadding(0), vtableOwner(nullptr),
                  _startPosition(startPosition), _endPosition(endPosition),
                  _structKind(structKind), _inheritedTypes(std::move(inheritedTypes)),
                  _contracts(std::move(contracts)), _ownedMembers(std::move(ownedMembers)),
                  _constructors(std::move(constructors)), destructor(destructor) {
            for (Decl* ownedMember : _ownedMembers) {
                ownedMember->container = this;
            }

            for (ConstructorDecl* constructor : _constructors) {
                constructor->container = this;
            }

            if (this->destructor != nullptr) {
                this->destructor->container = this;
            }
        }

        TextPosition _startPosition;
        TextPosition _endPosition;
        Kind _structKind;
        // This a list of both the possible base struct AND the inherited traits
        std::vector<Type*> _inheritedTypes;
        std::vector<Cont*> _contracts;
        // This is a list of ALL members; including static, const, AND instance members
        std::vector<Decl*> _ownedMembers;
        std::vector<ConstructorDecl*> _constructors;

    };
}

#endif //GULC_STRUCTDECL_HPP
