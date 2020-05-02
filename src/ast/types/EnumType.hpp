#ifndef GULC_ENUMTYPE_HPP
#define GULC_ENUMTYPE_HPP

#include <ast/Type.hpp>
#include <ast/decls/EnumDecl.hpp>

namespace gulc {
    class EnumType : public Type {
    public:
        static bool classof(const Type* type) { return type->getTypeKind() == Type::Kind::Enum; }

        EnumType(Qualifier qualifier, EnumDecl* decl, TextPosition startPosition, TextPosition endPosition)
                : Type(Type::Kind::Enum, qualifier, false),
                  _decl(decl), _startPosition(startPosition), _endPosition(endPosition) {}

        EnumDecl* decl() { return _decl; }
        EnumDecl const* decl() const { return _decl; }
        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        std::string toString() const override {
            return _decl->identifier().name();
        }

        Type* deepCopy() const override {
            return new EnumType(_qualifier, _decl, _startPosition, _endPosition);
        }

    protected:
        EnumDecl* _decl;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_ENUMTYPE_HPP
