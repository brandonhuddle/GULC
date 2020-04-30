#include "NamespacePrototyper.hpp"

std::vector<gulc::NamespaceDecl*> gulc::NamespacePrototyper::generatePrototypes(std::vector<ASTFile>& files) {
    std::vector<NamespaceDecl *> result;

    for (ASTFile& fileAst : files) {
        for (Decl *decl : fileAst.declarations) {
            if (llvm::isa<NamespaceDecl>(decl)) {
                generateNamespaceDecl(result, llvm::dyn_cast<NamespaceDecl>(decl));
            } else if (llvm::isa<ImportDecl>(decl)) {
                // Fill our imports list. We will handle adding the prototype references at a later point.
                fileAst.imports.push_back(llvm::dyn_cast<ImportDecl>(decl));
            }
        }
    }

    return result;
}

gulc::NamespaceDecl* gulc::NamespacePrototyper::getNamespacePrototype(std::vector<NamespaceDecl*>& result,
                                                                      std::string const& name) {
    if (currentNamespace == nullptr) {
        for (NamespaceDecl *namespaceDecl : result) {
            if (namespaceDecl->identifier().name() == name) {
                return namespaceDecl;
            }
        }
    } else {
        for (Decl* checkDecl : currentNamespace->nestedDecls()) {
            if (llvm::isa<NamespaceDecl>(checkDecl)) {
                if (checkDecl->identifier().name() == name) {
                    return llvm::dyn_cast<NamespaceDecl>(checkDecl);
                }
            }
        }
    }

    auto newNamespace = new NamespaceDecl(-1, {}, Identifier({}, {}, name), {}, {},
                                          true, {});

    if (currentNamespace == nullptr) {
        result.push_back(newNamespace);
    } else {
        currentNamespace->addNestedDecl(newNamespace);
    }

    return newNamespace;
}

void gulc::NamespacePrototyper::generateNamespaceDecl(std::vector<NamespaceDecl*>& result,
                                                      gulc::NamespaceDecl* namespaceDecl) {
    NamespaceDecl* oldNamespace = currentNamespace;
    currentNamespace = getNamespacePrototype(result, namespaceDecl->identifier().name());

    // Set the `namespaceDecl`'s prototype so it can access all `nestedDecls` in the package
    namespaceDecl->prototype = currentNamespace;

    for (Decl* decl : namespaceDecl->nestedDecls()) {
        if (llvm::isa<NamespaceDecl>(decl)) {
            generateNamespaceDecl(result, llvm::dyn_cast<NamespaceDecl>(decl));
        } else {
            currentNamespace->addNestedDecl(decl);
        }
    }

    currentNamespace = oldNamespace;
}
