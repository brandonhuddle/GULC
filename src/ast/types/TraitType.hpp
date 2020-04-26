#ifndef GULC_TRAITTYPE_HPP
#define GULC_TRAITTYPE_HPP

#include <ast/Type.hpp>
#include <ast/decls/TraitDecl.hpp>

namespace gulc {
    class TraitType : public Type {
    public:
        static bool classof(const Type* type) { return type->getTypeKind() == Type::Kind::Trait; }

        TraitType(Qualifier qualifier, TraitDecl* decl, TextPosition startPosition, TextPosition endPosition)
                : Type(Type::Kind::Trait, qualifier, false),
                  _decl(decl), _startPosition(startPosition), _endPosition(endPosition) {}

        TraitDecl* decl() { return _decl; }
        TraitDecl const* decl() const { return _decl; }
        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        std::string toString() const override {
            return _decl->identifier().name();
        }

        Type* deepCopy() const override {
            return new TraitType(_qualifier, _decl, _startPosition, _endPosition);
        }

    protected:
        TraitDecl* _decl;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_TRAITTYPE_HPP
