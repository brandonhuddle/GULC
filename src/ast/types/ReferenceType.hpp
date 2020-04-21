#ifndef GULC_REFERENCETYPE_HPP
#define GULC_REFERENCETYPE_HPP

#include <ast/Type.hpp>

namespace gulc {
    class ReferenceType : public Type {
    public:
        static bool classof(const Type* type) { return type->getTypeKind() == Type::Kind::Reference; }

        Type* nestedType;

        ReferenceType(Qualifier qualifier, Type* nestedType)
                : Type(Type::Kind::Reference, qualifier, false), nestedType(nestedType) {}

        TextPosition startPosition() const override { return nestedType->startPosition(); }
        TextPosition endPosition() const override { return nestedType->endPosition(); }

        std::string toString() const override {
            return "ref " + nestedType->toString();
        }

        Type* deepCopy() const override {
            return new ReferenceType(_qualifier, nestedType->deepCopy());
        }

        ~ReferenceType() override {
            delete nestedType;
        }

    };
}

#endif //GULC_REFERENCETYPE_HPP
