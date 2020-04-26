#ifndef GULC_BASICTYPERESOLVER_HPP
#define GULC_BASICTYPERESOLVER_HPP

#include <vector>
#include <string>
#include <parsing/ASTFile.hpp>
#include <ast/decls/TemplateParameterDecl.hpp>
#include <ast/decls/NamespaceDecl.hpp>
#include <ast/decls/StructDecl.hpp>
#include <ast/decls/TemplateStructDecl.hpp>
#include <ast/decls/TemplateFunctionDecl.hpp>
#include <ast/decls/VariableDecl.hpp>
#include <ast/stmts/CaseStmt.hpp>
#include <ast/stmts/CatchStmt.hpp>
#include <ast/stmts/DoStmt.hpp>
#include <ast/stmts/ForStmt.hpp>
#include <ast/stmts/IfStmt.hpp>
#include <ast/stmts/LabeledStmt.hpp>
#include <ast/stmts/ReturnStmt.hpp>
#include <ast/stmts/SwitchStmt.hpp>
#include <ast/stmts/TryStmt.hpp>
#include <ast/stmts/WhileStmt.hpp>
#include <ast/exprs/ArrayLiteralExpr.hpp>
#include <ast/exprs/AsExpr.hpp>
#include <ast/exprs/AssignmentOperatorExpr.hpp>
#include <ast/exprs/FunctionCallExpr.hpp>
#include <ast/exprs/HasExpr.hpp>
#include <ast/exprs/IndexerCallExpr.hpp>
#include <ast/exprs/IsExpr.hpp>
#include <ast/exprs/LabeledArgumentExpr.hpp>
#include <ast/exprs/MemberAccessCallExpr.hpp>
#include <ast/exprs/ParenExpr.hpp>
#include <ast/exprs/PostfixOperatorExpr.hpp>
#include <ast/exprs/PrefixOperatorExpr.hpp>
#include <ast/exprs/TernaryExpr.hpp>
#include <ast/exprs/ValueLiteralExpr.hpp>
#include <ast/exprs/VariableDeclExpr.hpp>
#include <ast/decls/PropertyDecl.hpp>
#include <ast/decls/SubscriptOperatorDecl.hpp>
#include <ast/decls/TemplateTraitDecl.hpp>

namespace gulc {
    /**
     * BasicTypeResolver resolves types as much as possible for all top level AST Nodes
     *
     * This pass will handle resolving types to their absolute form UNLESS the type is a template, in that case we
     * return a `TemplatedType` that contains the potential template `Decl` they could resolve to.
     *
     * This will handle top level types AND types contained within `FunctionDecl` bodies.
     */
    class BasicTypeResolver {
    public:
        explicit BasicTypeResolver(std::vector<std::string> const& filePaths)
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

        // After a type is resolved we will do a second pass to process it (mainly just process template arguments)
        void processType(Type* type) const;

        void processDecl(Decl* decl, bool isGlobal = true);
        void processFunctionDecl(FunctionDecl* functionDecl);
        void processNamespaceDecl(NamespaceDecl* namespaceDecl);
        void processParameterDecl(ParameterDecl* parameterDecl) const;
        void processPropertyDecl(PropertyDecl* propertyDecl);
        void processPropertyGetDecl(PropertyGetDecl* propertyGetDecl);
        void processPropertySetDecl(PropertySetDecl* propertySetDecl);
        void processStructDecl(StructDecl* structDecl);
        void processSubscriptOperatorDecl(SubscriptOperatorDecl* subscriptOperatorDecl);
        void processSubscriptOperatorGetDecl(SubscriptOperatorGetDecl* subscriptOperatorGetDecl);
        void processSubscriptOperatorSetDecl(SubscriptOperatorSetDecl* subscriptOperatorSetDecl);
        void processTemplateFunctionDecl(TemplateFunctionDecl* templateFunctionDecl);
        void processTemplateParameterDecl(TemplateParameterDecl* templateParameterDecl) const;
        void processTemplateStructDecl(TemplateStructDecl* templateStructDecl);
        void processTemplateTraitDecl(TemplateTraitDecl* templateTraitDecl);
        void processTraitDecl(TraitDecl* traitDecl);
        void processVariableDecl(VariableDecl* variableDecl, bool isGlobal) const;

        void processStmt(Stmt* stmt) const;
        void processCaseStmt(CaseStmt* caseStmt) const;
        void processCatchStmt(CatchStmt* catchStmt) const;
        void processCompoundStmt(CompoundStmt* compoundStmt) const;
        void processDoStmt(DoStmt* doStmt) const;
        void processForStmt(ForStmt* forStmt) const;
        void processIfStmt(IfStmt* ifStmt) const;
        void processLabeledStmt(LabeledStmt* labeledStmt) const;
        void processReturnStmt(ReturnStmt* returnStmt) const;
        void processSwitchStmt(SwitchStmt* switchStmt) const;
        void processTryStmt(TryStmt* tryStmt) const;
        void processWhileStmt(WhileStmt* whileStmt) const;

        void processTemplateArgumentExpr(Expr*& expr) const;
        void processExpr(Expr* expr) const;
        void processArrayLiteralExpr(ArrayLiteralExpr* arrayLiteralExpr) const;
        void processAsExpr(AsExpr* asExpr) const;
        void processAssignmentOperatorExpr(AssignmentOperatorExpr* assignmentOperatorExpr) const;
        void processFunctionCallExpr(FunctionCallExpr* functionCallExpr) const;
        void processHasExpr(HasExpr* hasExpr) const;
        void processIdentifierExpr(IdentifierExpr* identifierExpr) const;
        void processIndexerCallExpr(IndexerCallExpr* indexerCallExpr) const;
        void processInfixOperatorExpr(InfixOperatorExpr* infixOperatorExpr) const;
        void processIsExpr(IsExpr* isExpr) const;
        void processLabeledArgumentExpr(LabeledArgumentExpr* labeledArgumentExpr) const;
        void processMemberAccessCallExpr(MemberAccessCallExpr* memberAccessCallExpr) const;
        void processParenExpr(ParenExpr* parenExpr) const;
        void processPostfixOperatorExpr(PostfixOperatorExpr* postfixOperatorExpr) const;
        void processPrefixOperatorExpr(PrefixOperatorExpr* prefixOperatorExpr) const;
        void processTernaryExpr(TernaryExpr* ternaryExpr) const;
        void processTypeExpr(TypeExpr* typeExpr) const;
        void processValueLiteralExpr(ValueLiteralExpr* valueLiteralExpr) const;
        void processVariableDeclExpr(VariableDeclExpr* variableDeclExpr) const;

    };
}

#endif //GULC_BASICTYPERESOLVER_HPP
