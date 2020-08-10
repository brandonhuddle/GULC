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
#include <ast/stmts/DoWhileStmt.hpp>
#include <ast/stmts/ForStmt.hpp>
#include <ast/stmts/IfStmt.hpp>
#include <ast/stmts/LabeledStmt.hpp>
#include <ast/stmts/ReturnStmt.hpp>
#include <ast/stmts/SwitchStmt.hpp>
#include <ast/stmts/DoCatchStmt.hpp>
#include <ast/stmts/WhileStmt.hpp>
#include <ast/exprs/ArrayLiteralExpr.hpp>
#include <ast/exprs/AsExpr.hpp>
#include <ast/exprs/AssignmentOperatorExpr.hpp>
#include <ast/exprs/FunctionCallExpr.hpp>
#include <ast/exprs/HasExpr.hpp>
#include <ast/exprs/SubscriptCallExpr.hpp>
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
#include <ast/decls/TypeAliasDecl.hpp>
#include <ast/decls/EnumDecl.hpp>
#include <ast/decls/ExtensionDecl.hpp>
#include <ast/exprs/CheckExtendsTypeExpr.hpp>
#include <ast/stmts/BreakStmt.hpp>
#include <ast/stmts/ContinueStmt.hpp>
#include <ast/stmts/GotoStmt.hpp>
#include <ast/exprs/TryExpr.hpp>

namespace gulc {
    /**
     * BasicTypeResolver resolves types as much as possible for all top level AST Nodes
     *
     * This pass will handle resolving types to their absolute form UNLESS the type is a template, in that case we
     * return a `TemplatedType` that contains the potential template `Decl` they could resolve to.
     *
     * This will handle top level types AND types contained within `FunctionDecl` bodies.
     *
     * NOTE: This will also handle some basic validation within `Stmt` instances
     */
    class BasicTypeResolver {
    public:
        BasicTypeResolver(std::vector<std::string> const& filePaths, std::vector<NamespaceDecl*>& namespacePrototypes)
                : _filePaths(filePaths), _namespacePrototypes(namespacePrototypes), _currentFile() {}

        void processFiles(std::vector<ASTFile>& files);

    private:
        struct LabelStatus {
            bool status;
            TextPosition startPosition;
            TextPosition endPosition;

            LabelStatus()
                    : LabelStatus(false, {}, {}) {}
            LabelStatus(bool status, TextPosition startPosition, TextPosition endPosition)
                    : status(status), startPosition(startPosition), endPosition(endPosition) {}

        };

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

        // List of resolved and unresolved labels (if the boolean is true then it is resolved, else it isn't found)
        std::map<std::string, LabelStatus> _labelIdentifiers;

        void printError(std::string const& message, TextPosition startPosition, TextPosition endPosition) const;
        void printWarning(std::string const& message, TextPosition startPosition, TextPosition endPosition) const;

        bool resolveType(Type*& type);

        // After a type is resolved we will do a second pass to process it (mainly just process template parameters)
        void processType(Type*& type);

        void processContracts(std::vector<Cont*>& contracts);

        void processDecl(Decl* decl, bool isGlobal = true);
        void processEnumDecl(EnumDecl* enumDecl);
        void processExtensionDecl(ExtensionDecl* extensionDecl);
        void processFunctionDecl(FunctionDecl* functionDecl);
        void processNamespaceDecl(NamespaceDecl* namespaceDecl);
        void processParameterDecl(ParameterDecl* parameterDecl);
        void processPropertyDecl(PropertyDecl* propertyDecl);
        void processPropertyGetDecl(PropertyGetDecl* propertyGetDecl);
        void processPropertySetDecl(PropertySetDecl* propertySetDecl);
        void processStructDecl(StructDecl* structDecl);
        void processSubscriptOperatorDecl(SubscriptOperatorDecl* subscriptOperatorDecl);
        void processSubscriptOperatorGetDecl(SubscriptOperatorGetDecl* subscriptOperatorGetDecl);
        void processSubscriptOperatorSetDecl(SubscriptOperatorSetDecl* subscriptOperatorSetDecl);
        void processTemplateFunctionDecl(TemplateFunctionDecl* templateFunctionDecl);
        void processTemplateParameterDecl(TemplateParameterDecl* templateParameterDecl);
        void processTemplateStructDecl(TemplateStructDecl* templateStructDecl);
        void processTemplateTraitDecl(TemplateTraitDecl* templateTraitDecl);
        void processTraitDecl(TraitDecl* traitDecl);
        void processTypeAliasDecl(TypeAliasDecl* typeAliasDecl);
        void processVariableDecl(VariableDecl* variableDecl, bool isGlobal);

        void processStmt(Stmt* stmt);
        void processBreakStmt(BreakStmt* breakStmt);
        void processCaseStmt(CaseStmt* caseStmt);
        void processCatchStmt(CatchStmt* catchStmt);
        void processCompoundStmt(CompoundStmt* compoundStmt);
        void processContinueStmt(ContinueStmt* continueStmt);
        void processDoCatchStmt(DoCatchStmt* doCatchStmt);
        void processDoWhileStmt(DoWhileStmt* doWhileStmt);
        void processForStmt(ForStmt* forStmt);
        void processGotoStmt(GotoStmt* gotoStmt);
        void processIfStmt(IfStmt* ifStmt);
        void processLabeledStmt(LabeledStmt* labeledStmt);
        void processReturnStmt(ReturnStmt* returnStmt);
        void processSwitchStmt(SwitchStmt* switchStmt);
        void processWhileStmt(WhileStmt* whileStmt);

        void processTemplateArgumentExpr(Expr*& expr);
        void processExpr(Expr* expr);
        void processArrayLiteralExpr(ArrayLiteralExpr* arrayLiteralExpr);
        void processAsExpr(AsExpr* asExpr);
        void processAssignmentOperatorExpr(AssignmentOperatorExpr* assignmentOperatorExpr);
        void processCheckExtendsTypeExpr(CheckExtendsTypeExpr* checkExtendsTypeExpr);
        void processFunctionCallExpr(FunctionCallExpr* functionCallExpr);
        void processHasExpr(HasExpr* hasExpr);
        void processIdentifierExpr(IdentifierExpr* identifierExpr);
        void processInfixOperatorExpr(InfixOperatorExpr* infixOperatorExpr);
        void processIsExpr(IsExpr* isExpr);
        void processLabeledArgumentExpr(LabeledArgumentExpr* labeledArgumentExpr);
        void processMemberAccessCallExpr(MemberAccessCallExpr* memberAccessCallExpr);
        void processParenExpr(ParenExpr* parenExpr);
        void processPostfixOperatorExpr(PostfixOperatorExpr* postfixOperatorExpr);
        void processPrefixOperatorExpr(PrefixOperatorExpr* prefixOperatorExpr);
        void processSubscriptCallExpr(SubscriptCallExpr* indexerCallExpr);
        void processTernaryExpr(TernaryExpr* ternaryExpr);
        void processTryExpr(TryExpr* tryExpr);
        void processTypeExpr(TypeExpr* typeExpr);
        void processValueLiteralExpr(ValueLiteralExpr* valueLiteralExpr) const;
        void processVariableDeclExpr(VariableDeclExpr* variableDeclExpr);

    };
}

#endif //GULC_BASICTYPERESOLVER_HPP
