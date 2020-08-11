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
#ifndef GULC_ENUMCONSTDECL_HPP
#define GULC_ENUMCONSTDECL_HPP

#include <ast/Decl.hpp>
#include <ast/Expr.hpp>

namespace gulc {
    class EnumConstDecl : public Decl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::EnumConst; }

        // NOTE: This can be null at some points of execution...
        Expr* constValue;

        EnumConstDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Identifier identifier,
                      TextPosition startPosition, TextPosition endPosition, Expr* constValue)
                : Decl(Kind::EnumConst, sourceFileID, std::move(attributes),
                       Decl::Visibility::Unassigned, false, std::move(identifier),
                       DeclModifiers::None),
                  _startPosition(startPosition), _endPosition(endPosition), constValue(constValue) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Decl* deepCopy() const override {
            std::vector<Attr*> copiedAttributes;
            copiedAttributes.reserve(_attributes.size());
            Expr* copiedConstValue = nullptr;

            for (Attr* attribute : _attributes) {
                copiedAttributes.push_back(attribute->deepCopy());
            }

            if (constValue != nullptr) {
                copiedConstValue = constValue->deepCopy();
            }

            auto result = new EnumConstDecl(_sourceFileID, copiedAttributes, _identifier,
                                            _startPosition, _endPosition, copiedConstValue);
            result->container = container;
            result->containedInTemplate = containedInTemplate;
            result->originalDecl = (originalDecl == nullptr ? this : originalDecl);
            return result;
        }

        ~EnumConstDecl() override {
            delete constValue;
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_ENUMCONSTDECL_HPP
