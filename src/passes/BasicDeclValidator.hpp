#ifndef GULC_BASICDECLVALIDATOR_HPP
#define GULC_BASICDECLVALIDATOR_HPP

#include <vector>
#include <string>
#include <ast/decls/NamespaceDecl.hpp>
#include <parsing/ASTFile.hpp>
#include <ast/decls/TemplateParameterDecl.hpp>
#include <ast/decls/FunctionDecl.hpp>
#include <ast/decls/PropertyDecl.hpp>
#include <ast/decls/StructDecl.hpp>
#include <ast/decls/SubscriptOperatorDecl.hpp>
#include <ast/decls/TemplateFunctionDecl.hpp>
#include <ast/decls/TemplateStructDecl.hpp>
#include <ast/decls/TemplateTraitDecl.hpp>
#include <ast/decls/TypeAliasDecl.hpp>
#include <ast/decls/OperatorDecl.hpp>
#include <ast/decls/CallOperatorDecl.hpp>

namespace gulc {
    class BasicDeclValidator {
    public:
        BasicDeclValidator(std::vector<std::string> const& filePaths, std::vector<NamespaceDecl*>& namespacePrototypes)
                : _filePaths(filePaths), _namespacePrototypes(namespacePrototypes), _currentFile() {}

        void processFiles(std::vector<ASTFile>& files);

    private:
        std::vector<std::string> const& _filePaths;
        std::vector<NamespaceDecl*>& _namespacePrototypes;
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

        void validateImports(std::vector<ImportDecl*>& imports) const;
        bool resolveImport(std::vector<Identifier> const& importPath, std::size_t pathIndex,
                           NamespaceDecl* checkNamespace, NamespaceDecl** foundNamespace) const;

        Decl* getRedefinition(std::string const& findName, Decl* skipDecl) const;

        void validateParameters(std::vector<ParameterDecl*> const& parameters) const;
        void validateTemplateParameters(std::vector<TemplateParameterDecl*> const& templateParameters) const;

        void validateDecl(Decl* decl, bool isGlobal = true);
        void validateCallOperatorDecl(CallOperatorDecl* callOperatorDecl) const;
        void validateConstructorDecl(ConstructorDecl* constructorDecl) const;
        void validateDestructorDecl(DestructorDecl* destructorDecl) const;
        void validateFunctionDecl(FunctionDecl* functionDecl) const;
        void validateNamespaceDecl(NamespaceDecl* namespaceDecl);
        void validateOperatorDecl(OperatorDecl* operatorDecl) const;
        void validatePropertyDecl(PropertyDecl* propertyDecl, bool isGlobal);
        void validatePropertyGetDecl(PropertyGetDecl* propertyGetDecl);
        void validatePropertySetDecl(PropertySetDecl* propertySetDecl);
        void validateStructDecl(StructDecl* structDecl, bool checkForRedefinition);
        void validateSubscriptOperatorDecl(SubscriptOperatorDecl* subscriptOperatorDecl, bool isGlobal);
        void validateSubscriptOperatorGetDecl(SubscriptOperatorGetDecl* subscriptOperatorGetDecl);
        void validateSubscriptOperatorSetDecl(SubscriptOperatorSetDecl* subscriptOperatorSetDecl);
        void validateTemplateFunctionDecl(TemplateFunctionDecl* templateFunctionDecl);
        void validateTemplateStructDecl(TemplateStructDecl* templateStructDecl);
        void validateTemplateTraitDecl(TemplateTraitDecl* templateTraitDecl);
        void validateTraitDecl(TraitDecl* traitDecl, bool checkForRedefinition);
        void validateTypeAliasDecl(TypeAliasDecl* typeAliasDecl);
        void validateVariableDecl(VariableDecl* variableDecl, bool isGlobal) const;

    };
}

#endif //GULC_BASICDECLVALIDATOR_HPP
