#ifndef GULC_SELFTYPE_HPP
#define GULC_SELFTYPE_HPP

#include <ast/Type.hpp>

namespace gulc {
    /// NOTE: This is a partially resolved type, it should be replaced with a fully resolved type to what `Self`
    ///       actually is during processing.
    class SelfType : public Type {
    public:
        static bool classof(const Type* type) { return type->getTypeKind() == Type::Kind::Self; }

        SelfType(Qualifier qualifier, TextPosition startPosition, TextPosition endPosition)
                : Type(Type::Kind::Self, qualifier, false),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        std::string toString() const override {
            return "Self";
        }

        Type* deepCopy() const override {
            return new SelfType(_qualifier, _startPosition, _endPosition);
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_SELFTYPE_HPP
