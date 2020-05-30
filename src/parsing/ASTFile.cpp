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
