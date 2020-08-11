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
#ifndef GULC_DIMENSIONTYPE_HPP
#define GULC_DIMENSIONTYPE_HPP

#include <ast/Type.hpp>
#include <cstddef>

namespace gulc {
    /**
     * A dimension type is basically just a C#-esque multidimensional array. E.g. `int[,,,]` being a 4D array
     *
     * This type exists like this (rather than being built in) as there isn't really a good way that I can think of
     * to allow both of the following work at the same time without needing a hidden struct in the background:
     *
     *     let stackIntArray : int[,,] = int[12, 4, 3];
     *     let heapIntArray : int[,,] = new! int[12, 4, 3];
     *
     * I don't like building too many things into the compiler that aren't extendable. It ends up making the language
     * feel broken when you remove a standard library (like how C++ will feel broken if you don't use the C++ standard
     * library). I want GUL to have as much of the language not be tied up on the standard library as possible while
     * keeping the language feeling as easy to use as C#.
     *
     * The default for this type will usually be a standard struct like so:
     *
     *     struct dynamic_array<G, const dimensions: usize>
     *     {
     *         // index operator, internal `G` box/pointers, etc.
     *     }
     *
     * This CAN lead to abuse if someone creates their own type suffix overload for this to make `int[,,]` be something
     * that makes no sense but you can do that with anything. You can make `operator infix +(...)` subtract and you
     * can make `func add(...)` multiply. Don't abuse a language's features.
     *
     * Potential syntax for overloading this type:
     *
     *     public typesuffix<G, const dimensions: usize> [] = std.dynamic_array<G, dimensions>;
     *
     */
    class DimensionType : public Type {
    public:
        static bool classof(const Type* type) { return type->getTypeKind() == Type::Kind::Pointer; }

        Type* nestedType;

        DimensionType(Qualifier qualifier, Type* nestedType, std::size_t dimensions)
                : Type(Type::Kind::Pointer, qualifier, false),
                  nestedType(nestedType), _dimensions(dimensions) {}

        TextPosition startPosition() const override { return nestedType->startPosition(); }
        TextPosition endPosition() const override { return nestedType->endPosition(); }

        std::size_t dimensions() const { return _dimensions; }

        std::string toString() const override {
            std::string result = "[";

            for (int i = 1; i < _dimensions; ++i) {
                result += ",";
            }

            return result + "]" + nestedType->toString();
        }

        Type* deepCopy() const override {
            auto result = new DimensionType(_qualifier, nestedType->deepCopy(), _dimensions);
            result->setIsLValue(_isLValue);
            return result;
        }

        ~DimensionType() override {
            delete nestedType;
        }

    protected:
        std::size_t _dimensions;

    };
}

#endif //GULC_DIMENSIONTYPE_HPP
