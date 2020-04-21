#ifndef GULC_FLATARRAYTYPE_HPP
#define GULC_FLATARRAYTYPE_HPP

#include <ast/Type.hpp>
#include <ast/Expr.hpp>
#include <llvm/Support/Casting.h>

namespace gulc {
    class FlatArrayType : public Type {
    public:
        static bool classof(const Type* type) { return type->getTypeKind() == Type::Kind::FlatArray; }

        Type* indexType;
        Expr* length;

        FlatArrayType(Qualifier qualifier, Type* nestedType, Expr* length)
                : Type(Type::Kind::FlatArray, qualifier, false), indexType(nestedType), length(length) {}

        TextPosition startPosition() const override { return indexType->startPosition(); }
        TextPosition endPosition() const override { return indexType->endPosition(); }

        std::string toString() const override {
            std::string lengthString = "...";

            // TODO: To improve debugging we should detect if `length` is a `ValueLiteralExpr` and output the number...

            return "[" + indexType->toString() + "; " + lengthString + "]";
        }

        Type* deepCopy() const override {
            return new FlatArrayType(_qualifier, indexType->deepCopy(), length->deepCopy());
        }

        ~FlatArrayType() override {
            delete indexType;
        }

    };
}

#endif //GULC_FLATARRAYTYPE_HPP
