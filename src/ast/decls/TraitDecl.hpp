#ifndef GULC_TRAITDECL_HPP
#define GULC_TRAITDECL_HPP

#include <ast/Decl.hpp>
#include <ast/Type.hpp>
#include <ast/Cont.hpp>

namespace gulc {
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

        // This is used to know if this trait has passed through `DeclInstantiator`
        bool isInstantiated = false;

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
