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
#include "ASTFile.hpp"
#include <ast/decls/ExtensionDecl.hpp>
#include <utilities/TypeCompareUtil.hpp>

std::vector<gulc::ExtensionDecl*>* gulc::ASTFile::getTypeExtensions(const gulc::Type* forType) {
    auto foundExtensions = _cachedTypeExtensions.find(forType);

    if (foundExtensions == _cachedTypeExtensions.end()) {
        // The key doesn't already exist, we will have to generate the list...
        std::vector<ExtensionDecl*> typeExtensions;
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
