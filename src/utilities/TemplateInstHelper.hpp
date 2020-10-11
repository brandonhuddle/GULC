/*
 * Copyright (C) 2020 Brandon Huddle
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef GULC_TEMPLATEINSTHELPER_HPP
#define GULC_TEMPLATEINSTHELPER_HPP

#include <ast/decls/TemplateStructDecl.hpp>
#include <ast/decls/TemplateFunctionDecl.hpp>
#include <ast/decls/VariableDecl.hpp>
#include <ast/stmts/CaseStmt.hpp>
#include <ast/stmts/CatchStmt.hpp>
#include <ast/stmts/RepeatWhileStmt.hpp>
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
#include <ast/exprs/VariableDeclExpr.hpp>
#include <ast/decls/TemplateTraitDecl.hpp>
#include <ast/decls/CallOperatorDecl.hpp>
#include <ast/decls/EnumDecl.hpp>
#include <ast/decls/OperatorDecl.hpp>
#include <ast/decls/PropertyDecl.hpp>
#include <ast/decls/SubscriptOperatorDecl.hpp>
#include <ast/exprs/CheckExtendsTypeExpr.hpp>
//#include <ast/decls/TypeAliasDecl.hpp>

namespace gulc {
    class TypeAliasDecl;

    /**
     * The purpose of this class is to handle the instantiation of any template Decl
     *
     * Since instantiation is such a complex problem that requires replacing
     */
    class TemplateInstHelper {
    public:
        TemplateInstHelper()
                : _templateParameters(), _templateArguments(), _processBodyStmts(false),
                  _currentContainerTemplateType() {}

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
        void instantiateTemplateFunctionInstDecl(TemplateFunctionDecl* parentTemplateFunction,
                                                 TemplateFunctionInstDecl* templateFunctionInstDecl);
        void instantiateType(Type*& type, std::vector<TemplateParameterDecl*>* templateParameters,
                             std::vector<Expr*>* templateArguments);

    protected:
        std::vector<TemplateParameterDecl*>* _templateParameters;
        std::vector<Expr*>* _templateArguments;
        bool _processBodyStmts;
        // This is null unless we're processing decls within a template.
        Type* _currentContainerTemplateType;

        void instantiateAttr(Attr* attr) const;
        void instantiateCont(Cont* cont);
        void instantiateType(Type*& type);
        void instantiateDecl(Decl* decl);
        void instantiateStmt(Stmt* stmt);
        void instantiateExpr(Expr* expr);

        void instantiateCallOperatorDecl(CallOperatorDecl* callOperatorDecl);
        void instantiateConstructorDecl(ConstructorDecl* constructorDecl);
        void instantiateDestructorDecl(DestructorDecl* destructorDecl);
        void instantiateEnumDecl(EnumDecl* enumDecl);
        void instantiateFunctionDecl(FunctionDecl* functionDecl);
//        void instantiateImportDecl(ImportDecl* importDecl) const;
//        void instantiateNamespaceDecl(NamespaceDecl* namespaceDecl) const;
        void instantiateOperatorDecl(OperatorDecl* operatorDecl);
        void instantiateParameterDecl(ParameterDecl* parameterDecl);
        void instantiatePropertyDecl(PropertyDecl* propertyDecl);
        void instantiatePropertyGetDecl(PropertyGetDecl* propertyGetDecl);
        void instantiatePropertySetDecl(PropertySetDecl* propertySetDecl);
        void instantiateStructDecl(StructDecl* structDecl, bool setTemplateContainer);
        void instantiateSubscriptOperatorDecl(SubscriptOperatorDecl* subscriptOperatorDecl);
        void instantiateSubscriptOperatorGetDecl(SubscriptOperatorGetDecl* subscriptOperatorGetDecl);
        void instantiateSubscriptOperatorSetDecl(SubscriptOperatorSetDecl* subscriptOperatorSetDecl);
        void instantiateTemplateFunctionDecl(TemplateFunctionDecl* templateFunctionDecl);
        void instantiateTemplateParameterDecl(TemplateParameterDecl* templateParameterDecl);
        void instantiateTemplateStructDecl(TemplateStructDecl* templateStructDecl);
        void instantiateTemplateStructInstDecl(TemplateStructInstDecl* templateStructInstDecl);
        void instantiateTemplateTraitDecl(TemplateTraitDecl* templateTraitDecl);
        void instantiateTemplateTraitInstDecl(TemplateTraitInstDecl* templateTraitInstDecl);
        void instantiateTraitDecl(TraitDecl* traitDecl, bool setTemplateContainer);
        void instantiateTypeAliasDecl(TypeAliasDecl* typeAliasDecl);
        void instantiateVariableDecl(VariableDecl* variableDecl);

//        void instantiateBreakStmt(BreakStmt* breakStmt) const;
        void instantiateCaseStmt(CaseStmt* caseStmt);
        void instantiateCatchStmt(CatchStmt* catchStmt);
        void instantiateCompoundStmt(CompoundStmt* compoundStmt);
//        void instantiateContinueStmt(ContinueStmt* continueStmt) const;
        void instantiateDoCatchStmt(DoCatchStmt* doCatchStmt);
        void instantiateForStmt(ForStmt* forStmt);
//        void instantiateGotoStmt(GotoStmt* gotoStmt) const;
        void instantiateIfStmt(IfStmt* ifStmt);
        void instantiateLabeledStmt(LabeledStmt* labeledStmt);
        void instantiateRepeatWhileStmt(RepeatWhileStmt* repeatWhileStmt);
        void instantiateReturnStmt(ReturnStmt* returnStmt);
        void instantiateSwitchStmt(SwitchStmt* switchStmt);
        void instantiateWhileStmt(WhileStmt* whileStmt);

        void instantiateArrayLiteralExpr(ArrayLiteralExpr* arrayLiteralExpr);
        void instantiateAsExpr(AsExpr* asExpr);
        void instantiateAssignmentOperatorExpr(AssignmentOperatorExpr* assignmentOperatorExpr);
        void instantiateCheckExtendsTypeExpr(CheckExtendsTypeExpr* checkExtendsTypeExpr);
        void instantiateFunctionCallExpr(FunctionCallExpr* functionCallExpr);
        void instantiateHasExpr(HasExpr* hasExpr);
        void instantiateIdentifierExpr(IdentifierExpr* identifierExpr);
        void instantiateInfixOperatorExpr(InfixOperatorExpr* infixOperatorExpr);
        void instantiateIsExpr(IsExpr* isExpr);
        void instantiateLabeledArgumentExpr(LabeledArgumentExpr* labeledArgumentExpr);
        void instantiateMemberAccessCallExpr(MemberAccessCallExpr* memberAccessCallExpr);
        void instantiateParenExpr(ParenExpr* parenExpr);
        void instantiatePostfixOperatorExpr(PostfixOperatorExpr* postfixOperatorExpr);
        void instantiatePrefixOperatorExpr(PrefixOperatorExpr* prefixOperatorExpr);
        void instantiateSubscriptCallExpr(SubscriptCallExpr* subscriptCallExpr);
        void instantiateTernaryExpr(TernaryExpr* ternaryExpr);
        void instantiateTypeExpr(TypeExpr* typeExpr);
//        void instantiateValueLiteralExpr(ValueLiteralExpr* valueLiteralExpr) const;
        void instantiateVariableDeclExpr(VariableDeclExpr* variableDeclExpr);

    };
}

#endif //GULC_TEMPLATEINSTHELPER_HPP
