#ifndef GULC_POINTERTYPE_HPP
#define GULC_POINTERTYPE_HPP

#include <ast/Type.hpp>

namespace gulc {
    class PointerType : public Type {
    public:
        static bool classof(const Type* type) { return type->getTypeKind() == Type::Kind::Pointer; }

        Type* nestedType;

        PointerType(Qualifier qualifier, Type* nestedType)
                : Type(Type::Kind::Pointer, qualifier, false), nestedType(nestedType) {}

        TextPosition startPosition() const override { return nestedType->startPosition(); }
        TextPosition endPosition() const override { return nestedType->endPosition(); }

        std::string toString() const override {
            return "*" + nestedType->toString();
        }

        Type* deepCopy() const override {
            auto result = new PointerType(_qualifier, nestedType->deepCopy());
            result->setIsLValue(_isLValue);
            return result;
        }

        ~PointerType() override {
            delete nestedType;
        }

    };
}

#endif //GULC_POINTERTYPE_HPP
