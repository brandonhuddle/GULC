#ifndef GULC_BUILTINTYPE_HPP
#define GULC_BUILTINTYPE_HPP

#include <ast/Type.hpp>
#include <string>

namespace gulc {
    class BuiltInType : public Type {
    public:
        static bool classof(const Type* type) { return type->getTypeKind() == Type::Kind::BuiltIn; }

        explicit BuiltInType(Qualifier qualifier, std::string name, unsigned short sizeInBytes, bool isFloating,
                             bool isSigned, TextPosition startPosition, TextPosition endPosition)
                : Type(Type::Kind::BuiltIn, qualifier, false),
                  _name(std::move(name)), _sizeInBytes(sizeInBytes), _isFloating(isFloating), _isSigned(isSigned),
                  _startPosition(startPosition), _endPosition(endPosition) {

        }

        std::string const& name() const { return _name; }
        unsigned short sizeInBytes() const { return _sizeInBytes; }
        bool isFloating() const { return _isFloating; }
        bool isSigned() const { return _isSigned; }

        TextPosition startPosition() const override { return _startPosition;}
        TextPosition endPosition() const override { return _endPosition; }

        static BuiltInType* get(Qualifier qualifier, std::string const& name,
                                TextPosition startPosition, TextPosition endPosition) {
            unsigned short sizeInBytes;
            bool isFloating;
            bool isSigned;

            // TODO: Should we include `char` or make that a non-built-in type? I think Rust's way of making `char`
            //       a 4-byte built-in is a good idea. It handles the issues I've thought of with '\u0000\u0000' etc.
            //       But does it handle skintone emojis?
            if (name == "void") {
                sizeInBytes = 0;
                isFloating = false;
                isSigned = false;
            } else if (name == "i8") {
                sizeInBytes = 1;
                isFloating = false;
                isSigned = true;
            } else if (name == "u8" || name == "bool") {
                sizeInBytes = 1;
                isFloating = false;
                isSigned = false;
            } else if (name == "i16") {
                sizeInBytes = 2;
                isFloating = false;
                isSigned = true;
            } else if (name == "u16") {
                sizeInBytes = 2;
                isFloating = false;
                isSigned = false;
            } else if (name == "f16") {
                sizeInBytes = 2;
                isFloating = true;
                isSigned = true;
            } else if (name == "i32") {
                sizeInBytes = 4;
                isFloating = false;
                isSigned = true;
            } else if (name == "u32") {
                sizeInBytes = 4;
                isFloating = false;
                isSigned = false;
            } else if (name == "f32") {
                sizeInBytes = 4;
                isFloating = true;
                isSigned = true;
            } else if (name == "i64") {
                sizeInBytes = 8;
                isFloating = false;
                isSigned = true;
            } else if (name == "u64") {
                sizeInBytes = 8;
                isFloating = false;
                isSigned = false;
            } else if (name == "f64") {
                sizeInBytes = 8;
                isFloating = true;
                isSigned = true;
            } else {
                // Else default to `i32`
                sizeInBytes = 4;
                isFloating = false;
                isSigned = true;
            }

            return new BuiltInType(qualifier, name, sizeInBytes, isFloating, isSigned, startPosition, endPosition);
        }

        static bool isBuiltInType(std::string const& name) {
            return name == "void" ||
                   name == "i8" ||
                   name == "u8" || name == "bool" ||
                   name == "i16" ||
                   name == "u16" ||
                   name == "f16" ||
                   name == "i32" ||
                   name == "u32" ||
                   name == "f32" ||
                   name == "i64" ||
                   name == "u64" ||
                   name == "f64";
        }

        std::string toString() const override { return _name; }

        Type* deepCopy() const override {
            return new BuiltInType(_qualifier, _name, _sizeInBytes, _isFloating, _isSigned,
                                   _startPosition, _endPosition);
        }

    protected:
        std::string _name;
        unsigned short _sizeInBytes;
        bool _isFloating;
        bool _isSigned;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_BUILTINTYPE_HPP
