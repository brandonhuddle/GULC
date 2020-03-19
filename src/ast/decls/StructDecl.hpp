#ifndef GULC_STRUCTDECL_HPP
#define GULC_STRUCTDECL_HPP

#include <ast/Decl.hpp>
#include <ast/Type.hpp>
#include "ConstructorDecl.hpp"
#include "DestructorDecl.hpp"

namespace gulc {
    class StructDecl : public Decl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::Struct; }

        StructDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                   bool isConstExpr, Identifier identifier, TextPosition startPosition, TextPosition endPosition,
                   bool isClass, std::vector<Type*> inheritedTypes, std::vector<Cont*> contracts,
                   std::vector<Decl*> allMembers, std::vector<ConstructorDecl*> constructors,
                   DestructorDecl* destructor)
                : StructDecl(Decl::Kind::Struct, sourceFileID, std::move(attributes), visibility, isConstExpr,
                             std::move(identifier), startPosition, endPosition, isClass, std::move(inheritedTypes),
                             std::move(contracts), std::move(allMembers), std::move(constructors), destructor) {}

        bool isClass() const { return _isClass; }
        std::vector<Type*>& inheritedTypes() { return _inheritedTypes; }
        std::vector<Type*> const& inheritedTypes() const { return _inheritedTypes; }
        std::vector<Cont*>& contracts() { return _contracts; }
        std::vector<Cont*> const& contracts() const { return _contracts; }
        std::vector<Decl*>& allMembers() { return _allMembers; }
        std::vector<Decl*> const& allMembers() const { return _allMembers; }

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        ~StructDecl() override {
            for (Type* inheritedType : _inheritedTypes) {
                delete inheritedType;
            }

            for (Decl* member : _allMembers) {
                delete member;
            }
        }

        // This is the base struct found in the `inheritedTypes` list (if a base struct was found)
        // We don't own this so we don't free it
        StructDecl* baseStruct;
        // We use this as a hacky way to tell `BaseResolver` that this struct has already been processed
        bool baseWasResolved = false;

    protected:
        StructDecl(Decl::Kind declKind, unsigned int sourceFileID, std::vector<Attr*> attributes,
                   Decl::Visibility visibility, bool isConstExpr, Identifier identifier,
                   TextPosition startPosition, TextPosition endPosition, bool isClass,
                   std::vector<Type*> inheritedTypes, std::vector<Cont*> contracts, std::vector<Decl*> allMembers,
                   std::vector<ConstructorDecl*> constructors, DestructorDecl* destructor)
                : Decl(declKind, sourceFileID, std::move(attributes), visibility, isConstExpr, std::move(identifier)),
                  baseStruct(nullptr), _startPosition(startPosition), _endPosition(endPosition),
                  _isClass(isClass), _inheritedTypes(std::move(inheritedTypes)), _contracts(std::move(contracts)),
                  _allMembers(std::move(allMembers)), _constructors(std::move(constructors)),
                  _destructor(destructor) {}

        TextPosition _startPosition;
        TextPosition _endPosition;
        bool _isClass;
        // This a list of both the possible base struct AND the inherited traits
        std::vector<Type*> _inheritedTypes;
        std::vector<Cont*> _contracts;
        // This is a list of ALL members; including static, const, AND instance members
        std::vector<Decl*> _allMembers;
        std::vector<ConstructorDecl*> _constructors;
        DestructorDecl* _destructor;

    };
}

#endif //GULC_STRUCTDECL_HPP
