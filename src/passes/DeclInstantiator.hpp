#ifndef GULC_DECLINSTANTIATOR_HPP
#define GULC_DECLINSTANTIATOR_HPP

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
#include <Target.hpp>
#include <ast/exprs/ValueLiteralExpr.hpp>
#include <ast/decls/PropertyDecl.hpp>
#include <ast/decls/SubscriptOperatorDecl.hpp>
#include <ast/decls/TraitDecl.hpp>
#include <ast/decls/TemplateTraitDecl.hpp>

namespace gulc {
    /**
     * DeclInstantiator handles resolving `StructDecl` base types, resolving `TemplatedType`s to their
     * `Template*InstDecl` counterparts, creates `StructDecl` vtable, creates missing destructors, creates the memory
     * layout for `StructDecl` members, and calculates the sizes for `StructDecl`s
     *
     * NOTE: `TemplateStructDecl` will continue to contain `TemplatedType`s, these are NOT resolved currently
     */
    class DeclInstantiator {
    public:
        DeclInstantiator(gulc::Target const& target, std::vector<std::string> const& filePaths)
                : _target(target), _filePaths(filePaths), _currentFile() {}

        void processFiles(std::vector<ASTFile>& files);

    private:
        gulc::Target const& _target;
        std::vector<std::string> const& _filePaths;
        ASTFile* _currentFile;
        // List of Decls currently being worked on.
        //
        // Example:
        //        `struct Child : Parent`
        //    We need `Parent` to be processed for `Child` to be able to be processed. So the `_workingDecls` will be:
        //        { Child, Parent }
        //    If `Parent` ends up as `struct Parent : Child` we will detect that and error out.
        //
        // Note: This is ONLY `StructDecl`, `TraitDecl`, etc. NOT `FunctionDecl`
        //       (`TemplateFunctionDecl` might be added in the future for `const` solving)
        std::vector<gulc::Decl*> _workingDecls;
        // Used to detect circular references with `structUsesStructTypeAsValue` when `checkBaseStruct == true`
        std::vector<gulc::StructDecl*> _baseCheckStructDecls;

        void printError(std::string const& message, TextPosition startPosition, TextPosition endPosition) const;
        void printWarning(std::string const& message, TextPosition startPosition, TextPosition endPosition) const;

        bool resolveType(Type*& type);

        void processDecl(Decl* decl, bool isGlobal = true);
        void processFunctionDecl(FunctionDecl* functionDecl);
        void processNamespaceDecl(NamespaceDecl* namespaceDecl);
        void processParameterDecl(ParameterDecl* parameterDecl);
        void processPropertyDecl(PropertyDecl* propertyDecl);
        void processPropertyGetDecl(PropertyGetDecl* propertyGetDecl);
        void processPropertySetDecl(PropertySetDecl* propertySetDecl);
        void processStructDecl(StructDecl* structDecl, bool calculateSizeAndVTable = true);
        void processSubscriptOperatorDecl(SubscriptOperatorDecl* subscriptOperatorDecl);
        void processSubscriptOperatorGetDecl(SubscriptOperatorGetDecl* subscriptOperatorGetDecl);
        void processSubscriptOperatorSetDecl(SubscriptOperatorSetDecl* subscriptOperatorSetDecl);
        void processTemplateFunctionDecl(TemplateFunctionDecl* templateFunctionDecl);
        void processTemplateParameterDecl(TemplateParameterDecl* templateParameterDecl);
        void processTemplateStructDecl(TemplateStructDecl* templateStructDecl);
        void processTemplateStructInstDecl(TemplateStructInstDecl* templateStructInstDecl);
        void processTemplateTraitDecl(TemplateTraitDecl* templateTraitDecl);
        void processTemplateTraitInstDecl(TemplateTraitInstDecl* templateTraitInstDecl);
        void processTraitDecl(TraitDecl* traitDecl);
        void processVariableDecl(VariableDecl* variableDecl, bool isGlobal);

        // This will process a `Decl` while also checking for any circular dependencies using `_workingDecls`
        void processDependantDecl(Decl* decl);

        bool structUsesStructTypeAsValue(StructDecl* structType, StructDecl* checkStruct, bool checkBaseStruct);

        void processConstExpr(Expr* expr);
        void processTypeExpr(TypeExpr* typeExpr);
        void processValueLiteralExpr(ValueLiteralExpr* valueLiteralExpr) const;

    };
}

#endif //GULC_DECLINSTANTIATOR_HPP
