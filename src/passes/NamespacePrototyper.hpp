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
#ifndef GULC_NAMESPACEPROTOTYPER_HPP
#define GULC_NAMESPACEPROTOTYPER_HPP

#include <vector>
#include <ast/decls/NamespaceDecl.hpp>
#include <parsing/ASTFile.hpp>

namespace gulc {
    /**
     * NamespacePrototyper creates a global list of all namespaces visible to the current project
     */
    class NamespacePrototyper {
    public:
        NamespacePrototyper()
                : currentNamespace(nullptr) {}

        std::vector<NamespaceDecl*> generatePrototypes(std::vector<ASTFile>& files);

    protected:
        NamespaceDecl* getNamespacePrototype(std::vector<NamespaceDecl*>& result, std::string const& name);

        void generateNamespaceDecl(std::vector<NamespaceDecl*>& result, NamespaceDecl* namespaceDecl);

        NamespaceDecl* currentNamespace;

    };
}

#endif //GULC_NAMESPACEPROTOTYPER_HPP
