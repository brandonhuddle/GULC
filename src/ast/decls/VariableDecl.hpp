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
#ifndef GULC_VARIABLEDECL_HPP
#define GULC_VARIABLEDECL_HPP

#include <ast/Decl.hpp>
#include <ast/Type.hpp>
#include <ast/Expr.hpp>
#include <ast/DeclModifiers.hpp>

namespace gulc {
    class VariableDecl : public Decl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::Variable; }

        Type* type;
        Expr* initialValue;

        bool hasInitialValue() const { return initialValue != nullptr; }
        DeclModifiers declModifiers() const { return _declModifiers; }

        VariableDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                     bool isConstExpr, Identifier identifier, DeclModifiers declModifiers,
                     Type* type, Expr* initialValue, TextPosition startPosition, TextPosition endPosition)
                : Decl(Decl::Kind::Variable, sourceFileID, std::move(attributes), visibility, isConstExpr,
                       std::move(identifier), declModifiers),
                  type(type), initialValue(initialValue), _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Decl* deepCopy() const override {
            std::vector<Attr*> copiedAttributes;
            copiedAttributes.reserve(_attributes.size());
            Expr* copiedInitialValue = nullptr;

            for (Attr* attribute : _attributes) {
                copiedAttributes.push_back(attribute->deepCopy());
            }

            if (initialValue != nullptr) {
                copiedInitialValue = initialValue->deepCopy();
            }

            auto result = new VariableDecl(_sourceFileID, copiedAttributes, _declVisibility, _isConstExpr,
                                           _identifier, _declModifiers, type->deepCopy(),
                                           copiedInitialValue, _startPosition, _endPosition);
            result->container = container;
            result->containedInTemplate = containedInTemplate;
            result->originalDecl = (originalDecl == nullptr ? this : originalDecl);
            return result;
        }

        std::string getPrototypeString() const override {
            std::string result = getDeclModifiersString(_declModifiers);

            if (!result.empty()) result += " ";

            result += "var " + _identifier.name() + ": " + type->toString();

            return result;
        }

        ~VariableDecl() override {
            delete type;
            delete initialValue;
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_VARIABLEDECL_HPP
