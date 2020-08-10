#ifndef GULC_ALIASTYPE_HPP
#define GULC_ALIASTYPE_HPP

#include <ast/Type.hpp>
#include <ast/decls/TypeAliasDecl.hpp>

namespace gulc {
    class AliasType : public Type {
    public:
        static bool classof(const Type* type) { return type->getTypeKind() == Type::Kind::Alias; }

        AliasType(Qualifier qualifier, TypeAliasDecl* decl, TextPosition startPosition, TextPosition endPosition)
                : Type(Type::Kind::Alias, qualifier, false),
                  _decl(decl), _startPosition(startPosition), _endPosition(endPosition) {}

        TypeAliasDecl* decl() { return _decl; }
        TypeAliasDecl const* decl() const { return _decl; }
        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        std::string toString() const override {
            return _decl->identifier().name();
        }

        Type* deepCopy() const override {
            auto result = new AliasType(_qualifier, _decl, _startPosition, _endPosition);
            result->setIsLValue(_isLValue);
            return result;
        }

    protected:
        TypeAliasDecl* _decl;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_ALIASTYPE_HPP
