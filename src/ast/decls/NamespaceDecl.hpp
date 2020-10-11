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
#ifndef GULC_NAMESPACEDECL_HPP
#define GULC_NAMESPACEDECL_HPP

#include <ast/Decl.hpp>
#include <llvm/Support/Casting.h>
#include <map>

namespace gulc {
    class ExtensionDecl;
    class ASTFile;

    class NamespaceDecl : public Decl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::Namespace; }

        NamespaceDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Identifier identifier,
                      TextPosition startPosition, TextPosition endPosition)
                : Decl(Kind::Namespace, sourceFileID, std::move(attributes),
                       Decl::Visibility::Unassigned, false, std::move(identifier),
                       DeclModifiers::None),
                  prototype(nullptr), _startPosition(startPosition), _endPosition(endPosition), _nestedDecls(),
                  _isPrototype(false) {}
        NamespaceDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Identifier identifier,
                      TextPosition startPosition, TextPosition endPosition, bool isPrototype,
                      std::vector<Decl*> nestedDecls)
                : Decl(Kind::Namespace, sourceFileID, std::move(attributes),
                       Decl::Visibility::Unassigned, false, std::move(identifier),
                       DeclModifiers::None),
                  prototype(nullptr), _startPosition(startPosition), _endPosition(endPosition),
                  _nestedDecls(std::move(nestedDecls)), _isPrototype(isPrototype) {
            for (Decl* nestedDecl : _nestedDecls) {
                nestedDecl->container = this;
            }
        }

        std::vector<Decl*>& nestedDecls() { return _nestedDecls; }
        const std::vector<Decl*>& nestedDecls() const { return _nestedDecls; }

        void addNestedDecl(Decl* nestedDecl) {
            _nestedDecls.push_back(nestedDecl);
            nestedDecl->container = this;
        }

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Decl* deepCopy() const override {
            std::vector<Attr*> copiedAttributes;
            copiedAttributes.reserve(_attributes.size());
            std::vector<Decl*> copiedNestedDecls;
            copiedNestedDecls.reserve(_nestedDecls.size());

            for (Attr* attribute : _attributes) {
                copiedAttributes.push_back(attribute->deepCopy());
            }

            for (Decl* nestedDecl : _nestedDecls) {
                copiedNestedDecls.push_back(nestedDecl->deepCopy());
            }

            auto result = new NamespaceDecl(_sourceFileID, copiedAttributes, _identifier,
                                            _startPosition, _endPosition, _isPrototype,
                                            copiedNestedDecls);
            result->container = container;
            result->containedInTemplate = containedInTemplate;
            result->originalDecl = (originalDecl == nullptr ? this : originalDecl);
            return result;
        }

        std::string getPrototypeString() const override {
            std::string result = "namespace " + _identifier.name();

            return result;
        }

        ~NamespaceDecl() override {
            for (Decl* decl : _nestedDecls) {
                if (_isPrototype && !llvm::isa<NamespaceDecl>(decl)) {
                    // We're not allowed to delete anything but namespaces if the namespace is a prototype
                    continue;
                }

                delete decl;
            }

            // We own the types that are used as keys for the cached type extensions, we delete those here
            for (auto const& cachedTypeIterator : _cachedTypeExtensions) {
                delete cachedTypeIterator.first;
            }
        }

        // We use this to give us a link to the actual namespace with ALL `nestedDecls` from all sources
        NamespaceDecl* prototype;
        // All extensions within the current scope
        std::vector<ExtensionDecl*> scopeExtensions;

        std::vector<ExtensionDecl*>* getTypeExtensions(ASTFile& scopeFile, Type const* forType);

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;
        std::vector<Decl*> _nestedDecls;
        // If this is true it means we only own a nested `Decl` if it is a namespace, all other cannot be deleted by us.
        bool _isPrototype;
        std::map<Type const*, std::vector<ExtensionDecl*>> _cachedTypeExtensions;

    };
}

#endif //GULC_NAMESPACEDECL_HPP
