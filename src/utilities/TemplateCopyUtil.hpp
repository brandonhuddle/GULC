#ifndef GULC_TEMPLATECOPYUTIL_HPP
#define GULC_TEMPLATECOPYUTIL_HPP

#include <ast/Decl.hpp>
#include <ast/Cont.hpp>
#include <ast/Stmt.hpp>
#include <ast/Expr.hpp>
#include <ast/decls/CallOperatorDecl.hpp>
#include <ast/decls/ConstructorDecl.hpp>
#include <ast/decls/DestructorDecl.hpp>
#include <ast/decls/EnumDecl.hpp>
#include <ast/decls/OperatorDecl.hpp>
#include <ast/decls/PropertyDecl.hpp>
#include <ast/decls/StructDecl.hpp>
#include <ast/decls/SubscriptOperatorDecl.hpp>
#include <ast/decls/TemplateFunctionDecl.hpp>
#include <ast/decls/TemplateStructDecl.hpp>
#include <ast/decls/TraitDecl.hpp>
#include <ast/decls/TemplateTraitDecl.hpp>
#include <ast/decls/TypeAliasDecl.hpp>

namespace gulc {
    /**
     * Copying template declarations can be tedious and requrie more than just `->deepCopy()`
     *
     * This util is designed to handle replacing any template parameter references with their copied counterparts
     */
    class TemplateCopyUtil {
    public:
        TemplateCopyUtil()
                : oldTemplateParameters(nullptr), newTemplateParameters(nullptr) {}

        void instantiateTemplateStructCopy(std::vector<TemplateParameterDecl*> const* oldTemplateParameters,
                                           TemplateStructDecl* templateStruct);
        void instantiateTemplateTraitCopy(std::vector<TemplateParameterDecl*> const* oldTemplateParameters,
                                          TemplateTraitDecl* templateTrait);
        void instantiateTemplateFunctionCopy(std::vector<TemplateParameterDecl*> const* oldTemplateParameters,
                                             TemplateFunctionDecl* templateFunction);

    protected:
        std::vector<TemplateParameterDecl*> const* oldTemplateParameters;
        std::vector<TemplateParameterDecl*>* newTemplateParameters;

        void instantiateAttr(Attr* attr) const;
        void instantiateCont(Cont* cont) const;
        void instantiateType(Type*& type) const;
        void instantiateDecl(Decl* decl) const;
        void instantiateStmt(Stmt* stmt) const;
        void instantiateExpr(Expr* expr) const;

        void instantiateCallOperatorDecl(CallOperatorDecl* callOperatorDecl) const;
        void instantiateConstructorDecl(ConstructorDecl* constructorDecl) const;
        void instantiateDestructorDecl(DestructorDecl* destructorDecl) const;
        void instantiateEnumDecl(EnumDecl* enumDecl) const;
        void instantiateFunctionDecl(FunctionDecl* functionDecl) const;
//        void instantiateImportDecl(ImportDecl* importDecl) const;
//        void instantiateNamespaceDecl(NamespaceDecl* namespaceDecl) const;
        void instantiateOperatorDecl(OperatorDecl* operatorDecl) const;
        void instantiateParameterDecl(ParameterDecl* parameterDecl) const;
        void instantiatePropertyDecl(PropertyDecl* propertyDecl) const;
        void instantiatePropertyGetDecl(PropertyGetDecl* propertyGetDecl) const;
        void instantiatePropertySetDecl(PropertySetDecl* propertySetDecl) const;
        void instantiateStructDecl(StructDecl* structDecl) const;
        void instantiateSubscriptOperatorDecl(SubscriptOperatorDecl* subscriptOperatorDecl) const;
        void instantiateSubscriptOperatorGetDecl(SubscriptOperatorGetDecl* subscriptOperatorGetDecl) const;
        void instantiateSubscriptOperatorSetDecl(SubscriptOperatorSetDecl* subscriptOperatorSetDecl) const;
        void instantiateTemplateFunctionDecl(TemplateFunctionDecl* templateFunctionDecl) const;
        void instantiateTemplateParameterDecl(TemplateParameterDecl* templateParameterDecl) const;
        void instantiateTemplateStructDecl(TemplateStructDecl* templateStructDecl) const;
        void instantiateTemplateStructInstDecl(TemplateStructInstDecl* templateStructInstDecl) const;
        void instantiateTemplateTraitDecl(TemplateTraitDecl* templateTraitDecl) const;
        void instantiateTemplateTraitInstDecl(TemplateTraitInstDecl* templateTraitInstDecl) const;
        void instantiateTraitDecl(TraitDecl* traitDecl) const;
        void instantiateTypeAliasDecl(TypeAliasDecl* typeAliasDecl) const;
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
        void instantiateInfixOperatorExpr(InfixOperatorExpr* infixOperatorExpr) const;
        void instantiateIsExpr(IsExpr* isExpr) const;
        void instantiateLabeledArgumentExpr(LabeledArgumentExpr* labeledArgumentExpr) const;
        void instantiateMemberAccessCallExpr(MemberAccessCallExpr* memberAccessCallExpr) const;
        void instantiateParenExpr(ParenExpr* parenExpr) const;
        void instantiatePostfixOperatorExpr(PostfixOperatorExpr* postfixOperatorExpr) const;
        void instantiatePrefixOperatorExpr(PrefixOperatorExpr* prefixOperatorExpr) const;
        void instantiateSubscriptCallExpr(SubscriptCallExpr* subscriptCallExpr) const;
        void instantiateTernaryExpr(TernaryExpr* ternaryExpr) const;
        void instantiateTypeExpr(TypeExpr* typeExpr) const;
//        void instantiateValueLiteralExpr(ValueLiteralExpr* valueLiteralExpr) const;
        void instantiateVariableDeclExpr(VariableDeclExpr* variableDeclExpr) const;

    };
}

#endif //GULC_TEMPLATECOPYUTIL_HPP
