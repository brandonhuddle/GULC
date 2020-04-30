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
