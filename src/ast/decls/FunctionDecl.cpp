#include "FunctionDecl.hpp"
#include "StructDecl.hpp"
#include "TraitDecl.hpp"

bool gulc::FunctionDecl::isMemberFunction() const {
    return container != nullptr && (llvm::isa<StructDecl>(container) || llvm::isa<TraitDecl>(container));
}
