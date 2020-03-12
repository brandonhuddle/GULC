#ifndef GULC_CIRCULARREFERENCECHECKER_HPP
#define GULC_CIRCULARREFERENCECHECKER_HPP

#include <vector>
#include <string>
#include <parsing/ASTFile.hpp>
#include <ast/decls/TemplateParameterDecl.hpp>
#include <ast/decls/NamespaceDecl.hpp>
#include <ast/decls/StructDecl.hpp>
#include <ast/decls/TemplateStructDecl.hpp>

namespace gulc {
    /**
     * Checks for circular references in the inheritance list and in template parameters
     *
     * NOTE: This does NOT check for circular value members, that will be handled in the actual `Inheriter` pass
     */
    class CircularReferenceChecker {
    public:
        explicit CircularReferenceChecker(std::vector<std::string> const& filePaths)
                : _filePaths(filePaths), _currentFile() {}

        void processFiles(std::vector<ASTFile>& files);

    private:
        std::vector<std::string> const& _filePaths;
        ASTFile* _currentFile;
        // List of template parameter lists. Used to account for multiple levels of templates:
        //     struct Example1<T> { struct Example2<S> { func example3<U>(); } }
        // The above would result in three separate lists.
        std::vector<std::vector<TemplateParameterDecl*>*> _templateParameters;
        // The container of the currently processing Decl.
        // Example:
        //     namespace example { func ex() {} }
        // For the above `func ex` the _containingDecls would just have `namespace example`
        std::vector<gulc::Decl*> _containingDecls;

        void printError(std::string const& message, TextPosition startPosition, TextPosition endPosition) const;

        void processDecl(Decl* decl);
        void processNamespaceDecl(NamespaceDecl* namespaceDecl);
        void processStructDecl(StructDecl* structDecl);
        void processTemplateStructDecl(TemplateStructDecl* templateStructDecl);

    };
}

#endif //GULC_CIRCULARREFERENCECHECKER_HPP
