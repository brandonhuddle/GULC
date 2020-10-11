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
#ifndef GULC_TRAITPROTOTYPEDECL_HPP
#define GULC_TRAITPROTOTYPEDECL_HPP

#include <ast/Decl.hpp>

namespace gulc {
    // This class is only really used to hold the `trait path.to.Trait` for `expr has trait path.to.Trait`
    class TraitPrototypeDecl : public Decl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::TraitPrototype; }

        // I'm making it a type so we can do `UnidentifiedType` and `TraitType`
        Type* traitType;

        TraitPrototypeDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Type* traitType,
                           TextPosition startPosition, TextPosition endPosition)
                : Decl(Decl::Kind::TraitPrototype, sourceFileID, std::move(attributes), Visibility::Unassigned, true,
                       Identifier({}, {}, "_"), DeclModifiers::None),
                  traitType(traitType),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Decl* deepCopy() const override {
            auto result = new TraitPrototypeDecl(_sourceFileID, {}, traitType->deepCopy(),
                                                 _startPosition, _endPosition);
            result->container = container;
            result->containedInTemplate = containedInTemplate;
//            result->containerTemplateType = (containerTemplateType == nullptr ? nullptr : containerTemplateType->deepCopy());
            result->originalDecl = (originalDecl == nullptr ? this : originalDecl);
            return result;
        }

        std::string getPrototypeString() const override {
            return "trait " + traitType->toString();
        }

        ~TraitPrototypeDecl() override {
            delete traitType;
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_TRAITPROTOTYPEDECL_HPP
