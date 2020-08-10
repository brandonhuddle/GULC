#ifndef GULC_LABELEDTYPE_HPP
#define GULC_LABELEDTYPE_HPP

#include <ast/Type.hpp>

namespace gulc {
    /// A `LabeledType` is just a type with an argument label. It is needed for function pointers:
    /// `func(param1: i32)` = `FunctionPointerType` with parameters = `[ LabeledType(param1, i32) ]`
    class LabeledType : public Type {
    public:
        static bool classof(const Type* type) { return type->getTypeKind() == Type::Kind::LabeledType; }

        Type* type;

        LabeledType(Qualifier qualifier, std::string label, Type* type,
                    TextPosition startPosition, TextPosition endPosition)
                : Type(Type::Kind::LabeledType, qualifier, false), type(type),
                  _label(std::move(label)), _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        std::string const& label() const { return _label; }

        std::string toString() const override {
            return _label + ": " + type->toString();
        }

        Type* deepCopy() const override {
            auto result = new LabeledType(_qualifier, _label, type->deepCopy(),
                                          _startPosition, _endPosition);
            result->setIsLValue(_isLValue);
            return result;
        }

        ~LabeledType() override {
            delete type;
        }

    protected:
        std::string _label;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_LABELEDTYPE_HPP
