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
#include "FunctionDecl.hpp"
#include "StructDecl.hpp"
#include "TraitDecl.hpp"
#include "PropertyDecl.hpp"
#include "SubscriptOperatorDecl.hpp"

bool gulc::FunctionDecl::isMemberFunction() const {
    if (container == nullptr) {
        return false;
    } else if (llvm::isa<PropertyDecl>(container)) {
        auto checkProperty = llvm::dyn_cast<PropertyDecl>(container);

        if (checkProperty->container != nullptr &&
            (llvm::isa<StructDecl>(checkProperty->container) || llvm::isa<TraitDecl>(checkProperty->container))) {
            return true;
        }
    } else if (llvm::isa<SubscriptOperatorDecl>(container)) {
        auto checkSubscript = llvm::dyn_cast<SubscriptOperatorDecl>(container);

        if (checkSubscript->container != nullptr &&
            (llvm::isa<StructDecl>(checkSubscript->container) || llvm::isa<TraitDecl>(checkSubscript->container))) {
            return true;
        }
    } else if (llvm::isa<StructDecl>(container) || llvm::isa<TraitDecl>(container)) {
        return true;
    }

    return false;
}
