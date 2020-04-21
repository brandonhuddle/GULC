#include <ast/types/BuiltInType.hpp>
#include <llvm/Support/Casting.h>
#include <ast/types/PointerType.hpp>
#include <ast/types/ReferenceType.hpp>
#include <ast/types/StructType.hpp>
#include <iostream>
#include <ast/types/VTableType.hpp>
#include "SizeofUtil.hpp"

gulc::SizeAndAlignment gulc::SizeofUtil::getSizeAndAlignmentOf(const gulc::Target& target, gulc::Type* type) {
    if (llvm::isa<BuiltInType>(type)) {
        auto builtInType = llvm::dyn_cast<BuiltInType>(type);

        return gulc::SizeAndAlignment(builtInType->sizeInBytes(), builtInType->sizeInBytes());
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
