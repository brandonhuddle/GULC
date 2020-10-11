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
#ifndef GULC_IMAGINARYTYPE_HPP
#define GULC_IMAGINARYTYPE_HPP

#include <ast/Type.hpp>
#include <ast/decls/ImaginaryTypeDecl.hpp>

namespace gulc {
    // An "imaginary" type is a type that isn't _actually_ a type. It is an imaginary type created from the
    // requirements of a template type. I.e. `struct Example<T> {}` here `T` is the imaginary type with nothing
    // predefined for its use. It can only be used as a reference or a pointer.
    class ImaginaryType : public Type {
    public:
        static bool classof(const Type* type) { return type->getTypeKind() == Type::Kind::Imaginary; }

        ImaginaryType(Qualifier qualifier, ImaginaryTypeDecl* decl, TextPosition startPosition, TextPosition endPosition)
                : Type(Type::Kind::Imaginary, qualifier, false),
                  _decl(decl), _startPosition(startPosition), _endPosition(endPosition) {}

        ImaginaryTypeDecl* decl() { return _decl; }
        ImaginaryTypeDecl const* decl() const { return _decl; }
        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        std::string toString() const override {
            return _decl->identifier().name();
        }

        Type* deepCopy() const override {
            auto result = new ImaginaryType(_qualifier, _decl, _startPosition, _endPosition);
            result->setIsLValue(_isLValue);
            return result;
        }

    protected:
        ImaginaryTypeDecl* _decl;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_IMAGINARYTYPE_HPP
