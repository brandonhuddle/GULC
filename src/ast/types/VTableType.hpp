#ifndef GULC_VTABLETYPE_HPP
#define GULC_VTABLETYPE_HPP

#include <ast/Type.hpp>

namespace gulc {
    class VTableType : public Type {
    public:
        static bool classof(const Type* type) { return type->getTypeKind() == Type::Kind::VTable; }

        // NOTE: vtable is always const. They are NOT modifiable in GUL
        VTableType()
                : Type(Kind::VTable, Qualifier::Unassigned, false) {}

        TextPosition startPosition() const override {
            return {};
        }

        TextPosition endPosition() const override {
            return {};
        }

        std::string toString() const override { return "#vtable#"; }

        Type* deepCopy() const override {
            auto result = new VTableType();
            result->setIsLValue(_isLValue);
            return result;
        }

    };
}

#endif //GULC_VTABLETYPE_HPP
