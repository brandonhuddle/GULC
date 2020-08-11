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
#ifndef GULC_ALIASTYPE_HPP
#define GULC_ALIASTYPE_HPP

#include <ast/Type.hpp>
#include <ast/decls/TypeAliasDecl.hpp>

namespace gulc {
    class AliasType : public Type {
    public:
        static bool classof(const Type* type) { return type->getTypeKind() == Type::Kind::Alias; }

        AliasType(Qualifier qualifier, TypeAliasDecl* decl, TextPosition startPosition, TextPosition endPosition)
                : Type(Type::Kind::Alias, qualifier, false),
                  _decl(decl), _startPosition(startPosition), _endPosition(endPosition) {}

        TypeAliasDecl* decl() { return _decl; }
        TypeAliasDecl const* decl() const { return _decl; }
        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        std::string toString() const override {
            return _decl->identifier().name();
        }

        Type* deepCopy() const override {
            auto result = new AliasType(_qualifier, _decl, _startPosition, _endPosition);
            result->setIsLValue(_isLValue);
            return result;
        }

    protected:
        TypeAliasDecl* _decl;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_ALIASTYPE_HPP
