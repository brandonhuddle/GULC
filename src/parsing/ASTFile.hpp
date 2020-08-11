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
#ifndef GULC_ASTFILE_HPP
#define GULC_ASTFILE_HPP

#include <vector>
#include <ast/Decl.hpp>
#include <ast/decls/ImportDecl.hpp>

namespace gulc {
    class ASTFile {
    public:
        unsigned int sourceFileID;
        std::vector<Decl*> declarations;
        // This will be filled by a compiler pass.
        std::vector<ImportDecl*> imports;
        // All extensions within the current scope
        std::vector<ExtensionDecl*> scopeExtensions;

        ASTFile()
                : sourceFileID(0) {}
        ASTFile(unsigned int sourceFileID, std::vector<Decl*> declarations)
                : sourceFileID(sourceFileID), declarations(std::move(declarations)) {}

        std::vector<ExtensionDecl*>* getTypeExtensions(Type const* forType);

    protected:
        std::map<Type const*, std::vector<ExtensionDecl*>> _cachedTypeExtensions;

    };
}

#endif //GULC_ASTFILE_HPP
