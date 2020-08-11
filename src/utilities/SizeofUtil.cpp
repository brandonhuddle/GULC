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
#include <ast/types/BuiltInType.hpp>
#include <llvm/Support/Casting.h>
#include <ast/types/PointerType.hpp>
#include <ast/types/ReferenceType.hpp>
#include <ast/types/StructType.hpp>
#include <iostream>
#include <ast/types/VTableType.hpp>
#include <ast/types/BoolType.hpp>
#include "SizeofUtil.hpp"

gulc::SizeAndAlignment gulc::SizeofUtil::getSizeAndAlignmentOf(const gulc::Target& target, gulc::Type* type) {
    if (llvm::isa<BuiltInType>(type)) {
        auto builtInType = llvm::dyn_cast<BuiltInType>(type);

        return gulc::SizeAndAlignment(builtInType->sizeInBytes(), builtInType->sizeInBytes());
    } else if (llvm::isa<BoolType>(type)) {
        return SizeAndAlignment(1, 1);
    } else if (llvm::isa<PointerType>(type) || llvm::isa<ReferenceType>(type) || llvm::isa<VTableType>(type)) {
        // TODO: This is the same for `FunctionPointerType`
        return gulc::SizeAndAlignment(target.sizeofPtr(), target.sizeofPtr());
    } else if (llvm::isa<StructType>(type)) {
        auto structType = llvm::dyn_cast<StructType>(type);

        if (!structType->decl()->isInstantiated) {
            std::cerr << "[INTERNAL ERROR] uninstantiated struct found in `SizeofUtil::getSizeAndAlignmentOf`!" << std::endl;
            std::exit(1);
        }

        return gulc::SizeAndAlignment(structType->decl()->dataSizeWithPadding, target.alignofStruct());
    }

    std::cerr << "[INTERNAL ERROR] unknown type found in `SizeofUtil::getSizeAndAlignmentOf`!" << std::endl;
    std::exit(1);

    return gulc::SizeAndAlignment(0, 0);
}
