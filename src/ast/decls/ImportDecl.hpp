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
#ifndef GULC_IMPORTDECL_HPP
#define GULC_IMPORTDECL_HPP

#include <ast/Decl.hpp>
#include <vector>
#include <optional>
#include "NamespaceDecl.hpp"

namespace gulc {
    class ImportDecl : public Decl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::Import; }

        // Namespace the import points to, we don't own this so we don't free it...
        NamespaceDecl* pointToNamespace;

        ImportDecl(unsigned int sourceFileID, std::vector<Attr*> attributes,
                   TextPosition importStartPosition, TextPosition importEndPosition,
                   std::vector<Identifier> importPath)
                : Decl(Decl::Kind::Import, sourceFileID, std::move(attributes),
                       Decl::Visibility::Unassigned, false, {}, DeclModifiers::None),
                  _importStartPosition(importStartPosition), _importEndPosition(importEndPosition),
                  _importPath(std::move(importPath)), _asStartPosition(), _asEndPosition(),
                  _importAlias() {}

        ImportDecl(unsigned int sourceFileID, std::vector<Attr*> attributes,
                   TextPosition importStartPosition, TextPosition importEndPosition,
                   std::vector<Identifier> importPath, TextPosition asStartPosition, TextPosition asEndPosition,
                   Identifier importAlias)
                : Decl(Decl::Kind::Import, sourceFileID, std::move(attributes),
                       Decl::Visibility::Unassigned, false, {}, DeclModifiers::None),
                  _importStartPosition(importStartPosition), _importEndPosition(importEndPosition),
                  _importPath(std::move(importPath)), _asStartPosition(asStartPosition), _asEndPosition(asEndPosition),
                  _importAlias(importAlias) {}

        TextPosition importStartPosition() const { return _importStartPosition; }
        TextPosition importEndPosition() const { return _importEndPosition; }
        std::vector<Identifier> const& importPath() const { return _importPath; }
        bool hasAlias() const { return _importAlias.has_value(); }
        TextPosition asStartPosition() const { return _asStartPosition; }
        TextPosition asEndPosition() const { return _asEndPosition; }
        Identifier const& importAlias() const { return _importAlias.value(); }

        std::string importPathToString() const {
            std::string result = _importPath[0].name();

            for (std::size_t i = 1; i < _importPath.size(); ++i) {
                result += "." + _importPath[i].name();
            }

            return result;
        }

        TextPosition startPosition() const override { return _importStartPosition; }
        TextPosition endPosition() const override {
            if (_importAlias) {
                return _importAlias->endPosition();
            } else if (_importPath.empty()) {
                // If `importPath` is empty something went wrong but we won't error here. We will error out elsewhere
                // NOTE: This really won't ever happen. The parser will error out before creating this decl
                return _importEndPosition;
            } else {
                return _importPath.back().endPosition();
            }
        }

        Decl* deepCopy() const override {
            std::vector<Attr*> copiedAttributes;
            copiedAttributes.reserve(_attributes.size());

            for (Attr* attribute : _attributes) {
                copiedAttributes.push_back(attribute->deepCopy());
            }

            ImportDecl* result = nullptr;

            if (_importAlias.has_value()) {
                result =  new ImportDecl(_sourceFileID, copiedAttributes,
                                         _importStartPosition, _importEndPosition,
                                         _importPath, _asStartPosition, _asEndPosition,
                                         _importAlias.value());
            } else {
                result =  new ImportDecl(_sourceFileID, copiedAttributes,
                                         _importStartPosition, _importEndPosition,
                                         _importPath);
            }

            result->pointToNamespace = pointToNamespace;
            result->originalDecl = (originalDecl == nullptr ? this : originalDecl);
            return result;
        }

    protected:
        // These are the start and end positions just for the keyword `import`
        const TextPosition _importStartPosition;
        const TextPosition _importEndPosition;
        // E.g. `std.io`
        const std::vector<Identifier> _importPath;
        // These are the start and end positions just for the keyword `as`
        const TextPosition _asStartPosition;
        const TextPosition _asEndPosition;
        const std::optional<Identifier> _importAlias;

    };
}

#endif //GULC_IMPORTDECL_HPP
