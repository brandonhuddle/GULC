#ifndef GULC_TEMPLATEINSTHELPER_HPP
#define GULC_TEMPLATEINSTHELPER_HPP

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
#include <ast/exprs/VariableDeclExpr.hpp>
#include <ast/decls/TemplateTraitDecl.hpp>

namespace gulc {
    /**
     * The purpose of this class is to handle the instantiation of any template Decl
     *
     * Since instantiation is such a complex problem that requires replacing
     */
    class TemplateInstHelper {
    public:
        TemplateInstHelper()
                : templateParameters(), templateArguments(), processBodyStmts(false) {}

        /**
         * Instantiate the `TemplateStructInstDecl` using the parent `TemplateStructDecl`
         *
         * @param parentTemplateStruct
         * @param templateStructInstDecl
         * @param processBodyStmts Tells the function if it should also process the body `Stmt` for `FunctionDecl`s
         *                         etc. If this is false we will only deep copy the `Stmt` without processing it.
         *                         (NOTE: This will leave template parameter references)
         */
        void instantiateTemplateStructInstDecl(TemplateStructDecl* parentTemplateStruct,
                                               TemplateStructInstDecl* templateStructInstDecl,
                                               bool processBodyStmts);
        void instantiateTemplateTraitInstDecl(TemplateTraitDecl* parentTemplateTrait,
                                              TemplateTraitInstDecl* templateTraitInstDecl,
                                              bool processBodyStmts);

    protected:
        std::vector<TemplateParameterDecl*>* templateParameters;
        std::vector<Expr*>* templateArguments;
        bool processBodyStmts;

        void instantiateAttr(Attr* attr) const;
        void instantiateCont(Cont* cont) const;
        void instantiateType(Type*& type) const;
        void instantiateDecl(Decl* decl) const;
        void instantiateStmt(Stmt* stmt) const;
        void instantiateExpr(Expr* expr) const;

        void instantiateConstructorDecl(ConstructorDecl* constructorDecl) const;
        void instantiateDestructorDecl(DestructorDecl* destructorDecl) const;
        void instantiateFunctionDecl(FunctionDecl* functionDecl) const;
//        void instantiateImportDecl(ImportDecl* importDecl) const;
//        void instantiateNamespaceDecl(NamespaceDecl* namespaceDecl) const;
        void instantiateParameterDecl(ParameterDecl* parameterDecl) const;
        void instantiateStructDecl(StructDecl* structDecl) const;
        void instantiateTemplateFunctionDecl(TemplateFunctionDecl* templateFunctionDecl) const;
        void instantiateTemplateParameterDecl(TemplateParameterDecl* templateParameterDecl) const;
        void instantiateTemplateStructDecl(TemplateStructDecl* templateStructDecl) const;
        void instantiateTemplateStructInstDecl(TemplateStructInstDecl* templateStructInstDecl) const;
        void instantiateVariableDecl(VariableDecl* variableDecl) const;

//        void instantiateBreakStmt(BreakStmt* breakStmt) const;
        void instantiateCaseStmt(CaseStmt* caseStmt) const;
        void instantiateCatchStmt(CatchStmt* catchStmt) const;
        void instantiateCompoundStmt(CompoundStmt* compoundStmt) const;
//        void instantiateContinueStmt(ContinueStmt* continueStmt) const;
        void instantiateDoStmt(DoStmt* doStmt) const;
        void instantiateForStmt(ForStmt* forStmt) const;
//        void instantiateGotoStmt(GotoStmt* gotoStmt) const;
        void instantiateIfStmt(IfStmt* ifStmt) const;
        void instantiateLabeledStmt(LabeledStmt* labeledStmt) const;
        void instantiateReturnStmt(ReturnStmt* returnStmt) const;
        void instantiateSwitchStmt(SwitchStmt* switchStmt) const;
        void instantiateTryStmt(TryStmt* tryStmt) const;
        void instantiateWhileStmt(WhileStmt* whileStmt) const;

        void instantiateArrayLiteralExpr(ArrayLiteralExpr* arrayLiteralExpr) const;
        void instantiateAsExpr(AsExpr* asExpr) const;
        void instantiateAssignmentOperatorExpr(AssignmentOperatorExpr* assignmentOperatorExpr) const;
        void instantiateFunctionCallExpr(FunctionCallExpr* functionCallExpr) const;
        void instantiateHasExpr(HasExpr* hasExpr) const;
        void instantiateIdentifierExpr(IdentifierExpr* identifierExpr) const;
        void instantiateIndexerCallExpr(IndexerCallExpr* indexerCallExpr) const;
        void instantiateInfixOperatorExpr(InfixOperatorExpr* infixOperatorExpr) const;
        void instantiateIsExpr(IsExpr* isExpr) const;
        void instantiateLabeledArgumentExpr(LabeledArgumentExpr* labeledArgumentExpr) const;
        void instantiateMemberAccessCallExpr(MemberAccessCallExpr* memberAccessCallExpr) const;
        void instantiateParenExpr(ParenExpr* parenExpr) const;
        void instantiatePostfixOperatorExpr(PostfixOperatorExpr* postfixOperatorExpr) const;
        void instantiatePrefixOperatorExpr(PrefixOperatorExpr* prefixOperatorExpr) const;
        void instantiateTernaryExpr(TernaryExpr* ternaryExpr) const;
        void instantiateTypeExpr(TypeExpr* typeExpr) const;
//        void instantiateValueLiteralExpr(ValueLiteralExpr* valueLiteralExpr) const;
        void instantiateVariableDeclExpr(VariableDeclExpr* variableDeclExpr) const;

    };
}

#endif //GULC_TEMPLATEINSTHELPER_HPP
