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
#ifndef GULC_TEMPLATEPARAMETERDECL_HPP
#define GULC_TEMPLATEPARAMETERDECL_HPP

#include <ast/Decl.hpp>
#include <ast/Type.hpp>
#include <ast/Expr.hpp>

namespace gulc {
    class TemplateParameterDecl : public Decl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::TemplateParameter; }

        enum class TemplateParameterKind {
            // A type `struct Example<T> == Example<int>`
            Typename,
            // A const variable `struct Example<const param: int> == Example<12>`
            Const
        };

        // When the parameter kind is `const` this is the type of the const value
        // When the parameter kind is `typename` this is the specialized type (i.e. `struct Example<T: View>`)
        Type* type;
        // When the parameter kind is `const` this should be const-solvable expression (i.e. `12`, `"constString", etc.)
        // When the parameter kind is `typename` this should be a `TypeExpr`
        Expr* defaultValue;
        TemplateParameterKind templateParameterKind() const { return _templateParameterKind; }

        TemplateParameterDecl(unsigned int sourceFileID, std::vector<Attr*> attributes,
                              TemplateParameterKind templateParameterKind, Identifier identifier, Type* type,
                              Expr* defaultValue, TextPosition startPosition, TextPosition endPosition)
                : Decl(Decl::Kind::TemplateParameter, sourceFileID, std::move(attributes),
                       Decl::Visibility::Unassigned, true, std::move(identifier),
                       DeclModifiers::None),
                  type(type), defaultValue(defaultValue), _templateParameterKind(templateParameterKind),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Decl* deepCopy() const override {
            std::vector<Attr*> copiedAttributes;
            copiedAttributes.reserve(_attributes.size());
            std::vector<Type*> copiedInheritedTypes;
            copiedInheritedTypes.reserve(inheritedTypes.size());
            Type* copiedType = nullptr;
            Expr* copiedDefaultValue = nullptr;

            for (Attr* attribute : _attributes) {
                copiedAttributes.push_back(attribute->deepCopy());
            }

            for (Type* inheritedType : inheritedTypes) {
                copiedInheritedTypes.push_back(inheritedType->deepCopy());
            }

            if (type != nullptr) {
                copiedType = type->deepCopy();
            }

            if (defaultValue != nullptr) {
                copiedDefaultValue = defaultValue->deepCopy();
            }

            auto result = new TemplateParameterDecl(_sourceFileID, copiedAttributes,
                                                    _templateParameterKind, _identifier, copiedType,
                                                    copiedDefaultValue,
                                                    _startPosition, _endPosition);
            result->container = container;
            result->containedInTemplate = containedInTemplate;
            result->inheritedTypes = copiedInheritedTypes;
            result->originalDecl = (originalDecl == nullptr ? this : originalDecl);
            return result;
        }

        std::string getPrototypeString() const override {
            std::string result = getDeclModifiersString(_declModifiers);

            if (!result.empty()) result += " ";

            if (_templateParameterKind == TemplateParameterKind::Const) {
                result += "const ";
            }

            result += _identifier.name();

            if (type != nullptr) {
                result += ": " + type->toString();
            }

            return result;
        }

        ~TemplateParameterDecl() override {
            delete type;
            delete defaultValue;

            for (Type* inheritedType : inheritedTypes) {
                delete inheritedType;
            }
        }

        // These will be defined by `where` contracts
        // NOTE: This will not apply to `const` template parameters
        // TODO: We should delete this. Instead of doing this here we should create a new `ImaginaryType` or something
        //       that will be used to emulate the template type for validation.
        std::vector<Type*> inheritedTypes;
        // TODO: We'll also have to define `members` and probably `constructors`
        //       We shouldn't define a destructor as that should be implied (ALL types have destructors,
        //       some just can be deleted implicitly. e.g. `i32` "has" a destructor but it is worthless and never even
        //       considered)

    protected:
        TemplateParameterKind _templateParameterKind;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_TEMPLATEPARAMETERDECL_HPP
