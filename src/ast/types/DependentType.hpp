#ifndef GULC_DEPENDENTTYPE_HPP
#define GULC_DEPENDENTTYPE_HPP

#include <ast/Type.hpp>

namespace gulc {
    /**
     * Type to make it easier to represent types dependent on uninstantiated templates
     *
     * Allows for easy representation of `Example<T>::NestedExample<G>::DeepNestedType`
     */
    class DependentType : public Type {
    public:
        static bool classof(const Type* type) { return type->getTypeKind() == Type::Kind::Dependent; }

        // The container the type is dependent on
        Type* container;
        // The type that depends on the container
        Type* dependent;

        DependentType(Qualifier qualifier, Type* container, Type* dependent)
                : Type(Type::Kind::Dependent, qualifier, false),
                  container(container), dependent(dependent) {}

        TextPosition startPosition() const override { return container->startPosition(); }
        TextPosition endPosition() const override { return dependent->endPosition(); }

        std::string toString() const override {
            return container->toString() + "." + dependent->toString();
        }

        Type* deepCopy() const override {
            return new DependentType(_qualifier, container->deepCopy(), dependent->deepCopy());
        }

        ~DependentType() override {
            delete container;
            delete dependent;
        }

    };
}

#endif //GULC_DEPENDENTTYPE_HPP
