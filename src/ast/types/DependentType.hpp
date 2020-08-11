/*
 * Copyright (C) 2020 Brandon Huddle
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
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
            auto result = new DependentType(_qualifier, container->deepCopy(), dependent->deepCopy());
            result->setIsLValue(_isLValue);
            return result;
        }

        ~DependentType() override {
            delete container;
            delete dependent;
        }

    };
}

#endif //GULC_DEPENDENTTYPE_HPP
