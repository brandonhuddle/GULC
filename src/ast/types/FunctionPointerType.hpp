#ifndef GULC_FUNCTIONPOINTERTYPE_HPP
#define GULC_FUNCTIONPOINTERTYPE_HPP

#include <ast/Type.hpp>
#include <vector>
#include <llvm/Support/Casting.h>
#include "LabeledType.hpp"
#include "BuiltInType.hpp"

namespace gulc {
    /// Examples:
    ///  `func()`
    ///  `func(_: i32)`
    ///  `func(_: i32) -> i32`
    /// Potential Future Additions:
    ///  `func() throws`
    ///  `func(len: i32) requires len > 12` // Contracts are a core part of the function signature. You CANNOT
    ///                                     // reference a function with contracts into a non-contracted function
    ///                                     // pointer as that is illegal weakening. (you CAN reference a
    ///                                     // non-contracted function into a contracted function pointer as that is
    ///                                     // legal strengthening)
    class FunctionPointerType : public Type {
    public:
        static bool classof(const Type* type) { return type->getTypeKind() == Type::Kind::FunctionPointer; }

        std::vector<LabeledType*> parameters;
        Type* returnType;

        FunctionPointerType(Qualifier qualifier, std::vector<LabeledType*> parameters, Type* returnType,
                            TextPosition startPosition, TextPosition endPosition)
                : Type(Type::Kind::FunctionPointer, qualifier, false),
                  parameters(std::move(parameters)), returnType(returnType),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        std::string toString() const override {
            std::string parametersString;
            // NOTE: If the result type is null or `void` we keep it empty. This will contain the "->" in it.
            std::string returnTypeString;

            for (std::size_t i = 0; i < parameters.size(); ++i) {
                if (i != 0) {
                    parametersString += ", ";
                }

                parametersString += parameters[i]->toString();
            }

            if (returnType != nullptr) {
                // All types except a `void` type will have a `returnTypeString`
                if (!llvm::isa<BuiltInType>(returnType) ||
                        llvm::dyn_cast<BuiltInType>(returnType)->sizeInBytes() > 0) {
                    returnTypeString = " -> " + returnType->toString();
                }
            }

            return "func(" + parametersString + ")" + returnTypeString;
        }

        Type* deepCopy() const override {
            std::vector<LabeledType*> copiedParameters;
            copiedParameters.reserve(parameters.size());
            Type* copiedReturnType = nullptr;

            for (LabeledType* parameter : parameters) {
                copiedParameters.push_back(llvm::dyn_cast<LabeledType>(parameter->deepCopy()));
            }

            if (returnType != nullptr) {
                copiedReturnType = returnType->deepCopy();
            }

            return new FunctionPointerType(_qualifier, copiedParameters, copiedReturnType,
                                           _startPosition, _endPosition);
        }

        ~FunctionPointerType() override {
            for (LabeledType* parameter : parameters) {
                delete parameter;
            }

            delete returnType;
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_FUNCTIONPOINTERTYPE_HPP
