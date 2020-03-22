#ifndef GULC_BASERESOLVER_HPP
#define GULC_BASERESOLVER_HPP

#include <vector>
#include <string>
#include <parsing/ASTFile.hpp>
#include <ast/decls/TemplateParameterDecl.hpp>
#include <ast/decls/FunctionDecl.hpp>
#include <ast/decls/NamespaceDecl.hpp>
#include <ast/decls/StructDecl.hpp>
#include <ast/decls/TemplateFunctionDecl.hpp>
#include <ast/decls/TemplateStructDecl.hpp>
#include <ast/decls/VariableDecl.hpp>

namespace gulc {
    /**
     * The `BaseResolver` handles resolving the base types for class/structs, traits, enums, etc.
     */
    class BaseResolver {
    public:
        explicit BaseResolver(std::vector<std::string> const& filePaths)
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
        void printWarning(std::string const& message, TextPosition startPosition, TextPosition endPosition) const;

        bool resolveType(Type*& type) const;

        void processDecl(Decl* decl);
//        void processFunctionDecl(FunctionDecl* functionDecl);
        void processNamespaceDecl(NamespaceDecl* namespaceDecl);
//        void processParameterDecl(ParameterDecl* parameterDecl) const;
        void processStructDecl(StructDecl* structDecl);
//        void processTemplateFunctionDecl(TemplateFunctionDecl* templateFunctionDecl);
        void processTemplateParameterDecl(TemplateParameterDecl* templateParameterDecl) const;
        void processTemplateStructDecl(TemplateStructDecl* templateStructDecl);
//        void processVariableDecl(VariableDecl* variableDecl, bool isGlobal) const;

        // This attempts to process the expression as a type OR a const expression (used for template arguments)
        void processExprTypeOrConst(Expr*& expr) const;

    };
}

#endif //GULC_BASERESOLVER_HPP
