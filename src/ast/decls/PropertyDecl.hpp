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
#ifndef GULC_PROPERTYDECL_HPP
#define GULC_PROPERTYDECL_HPP

#include <ast/Decl.hpp>
#include <ast/Type.hpp>
#include <ast/DeclModifiers.hpp>
#include "PropertyGetDecl.hpp"
#include "PropertySetDecl.hpp"

namespace gulc {
    class PropertyDecl : public Decl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::Property; }

        // The type that will be used for the generic `get`/`set`
        Type* type;

        PropertyDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                     bool isConstExpr, Identifier identifier, Type* type,
                     TextPosition startPosition, TextPosition endPosition, DeclModifiers declModifiers,
                     std::vector<PropertyGetDecl*> getters, PropertySetDecl* setter)
                : Decl(Decl::Kind::Property, sourceFileID, std::move(attributes), visibility, isConstExpr,
                       std::move(identifier), declModifiers),
                  type(type), _startPosition(startPosition), _endPosition(endPosition),
                  _getters(std::move(getters)), _setter(setter) {
            for (PropertyGetDecl* getter : _getters) {
                getter->container = this;
            }

            if (_setter != nullptr) {
                _setter->container = this;
            }
        }

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        std::vector<PropertyGetDecl*>& getters() { return _getters; }
        std::vector<PropertyGetDecl*> const& getters() const { return _getters; }
        PropertySetDecl* setter() { return _setter; }
        PropertySetDecl const* setter() const { return _setter; }
        bool hasSetter() const { return _setter != nullptr; }

        Decl* deepCopy() const override {
            std::vector<Attr*> copiedAttributes;
            copiedAttributes.reserve(_attributes.size());
            std::vector<PropertyGetDecl*> copiedGetters;
            copiedGetters.reserve(_getters.size());
            PropertySetDecl* copiedSetter = nullptr;

            for (Attr* attribute : _attributes) {
                copiedAttributes.push_back(attribute->deepCopy());
            }

            for (PropertyGetDecl* getter : _getters) {
                copiedGetters.push_back(llvm::dyn_cast<PropertyGetDecl>(getter->deepCopy()));
            }

            if (_setter != nullptr) {
                copiedSetter = llvm::dyn_cast<PropertySetDecl>(_setter->deepCopy());
            }

            auto result = new PropertyDecl(_sourceFileID, copiedAttributes, _declVisibility, _isConstExpr,
                                           _identifier, type->deepCopy(),
                                           _startPosition, _endPosition,
                                           _declModifiers, copiedGetters, copiedSetter);
            result->container = container;
            result->containedInTemplate = containedInTemplate;
            result->originalDecl = (originalDecl == nullptr ? this : originalDecl);
            return result;
        }

        ~PropertyDecl() override {
            delete type;
            delete _setter;

            for (PropertyGetDecl* getter : _getters) {
                delete getter;
            }
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;
        std::vector<PropertyGetDecl*> _getters;
        PropertySetDecl* _setter;

    };
}

#endif //GULC_PROPERTYDECL_HPP
