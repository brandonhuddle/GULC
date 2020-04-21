#ifndef GULC_STRUCTTYPE_HPP
#define GULC_STRUCTTYPE_HPP

#include <ast/Type.hpp>
#include <ast/decls/StructDecl.hpp>

namespace gulc {
    class StructType : public Type {
    public:
        static bool classof(const Type* type) { return type->getTypeKind() == Type::Kind::Struct; }

        StructType(Qualifier qualifier, StructDecl* decl, TextPosition startPosition, TextPosition endPosition)
                : Type(Type::Kind::Struct, qualifier, false),
                  _decl(decl), _startPosition(startPosition), _endPosition(endPosition) {}

        StructDecl* decl() { return _decl; }
        StructDecl const* decl() const { return _decl; }
        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        std::string toString() const override {
            return _decl->identifier().name();
        }

        Type* deepCopy() const override {
            return new StructType(_qualifier, _decl, _startPosition, _endPosition);
        }

    protected:
        StructDecl* _decl;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_STRUCTTYPE_HPP
