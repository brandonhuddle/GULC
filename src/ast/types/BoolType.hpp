#ifndef GULC_BOOLTYPE_HPP
#define GULC_BOOLTYPE_HPP

#include <ast/Type.hpp>

namespace gulc {
    // I'm making `bool` a special type to try to prevent simple errors
    class BoolType : public Type {
    public:
        static bool classof(const Type* type) { return type->getTypeKind() == Type::Kind::Bool; }

        BoolType(Qualifier qualifier, TextPosition startPosition, TextPosition endPosition)
                : Type(Type::Kind::Bool, qualifier, false),
                  _startPosition(startPosition), _endPosition(endPosition) {}
        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        std::string toString() const override {
            return "bool";
        }

        Type* deepCopy() const override {
            auto result = new BoolType(_qualifier, _startPosition, _endPosition);
            result->setIsLValue(_isLValue);
            return result;
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_BOOLTYPE_HPP
