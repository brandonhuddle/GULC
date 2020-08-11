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
#include <utilities/TypeCompareUtil.hpp>
#include "NamespaceDecl.hpp"
#include "ExtensionDecl.hpp"
#include <parsing/ASTFile.hpp>

std::vector<gulc::ExtensionDecl*>* gulc::NamespaceDecl::getTypeExtensions(ASTFile& scopeFile, const gulc::Type* forType) {
    auto foundExtensions = _cachedTypeExtensions.find(forType);

    if (foundExtensions == _cachedTypeExtensions.end()) {
        // The key doesn't already exist, we will have to generate the list...
        // NOTE: Since namespaces can only be contained within files or other namespaces we can skip any search and
        //       only check if our container is a namespace
        std::vector<ExtensionDecl*> typeExtensions;

        // If the current namespace is contained within another namespace then we will check it for extensions
        // If we're NOT in a namespace then we will check the file for the extensions
        // We only have these two options as namespaces can only be contained within other namespaces
        // We also only have to call our direct container's `getTypeExtensions` as it will recursively do so until
        // we reach the root file
        if (container != nullptr && llvm::isa<NamespaceDecl>(container)) {
            auto checkNamespace = llvm::dyn_cast<NamespaceDecl>(container);

            // We will copy their list for the type
            std::vector<ExtensionDecl*>* containerExtensions = checkNamespace->getTypeExtensions(scopeFile, forType);

            if (containerExtensions != nullptr) {
                typeExtensions = *containerExtensions;
            }
        } else {
            // We will copy the file list for the type
            std::vector<ExtensionDecl*>* containerExtensions = scopeFile.getTypeExtensions(forType);

            if (containerExtensions != nullptr) {
                typeExtensions = *containerExtensions;
            }
        }

        TypeCompareUtil typeCompareUtil;

        for (ExtensionDecl* checkExtension : scopeExtensions) {
            if (typeCompareUtil.compareAreSameOrInherits(forType, checkExtension->typeToExtend)) {
                typeExtensions.push_back(checkExtension);
            }
        }

        if (typeExtensions.empty()) {
            return nullptr;
        } else {
            _cachedTypeExtensions.insert({forType, typeExtensions});

            return &_cachedTypeExtensions[forType];
        }
    } else {
        return &foundExtensions->second;
    }
}
