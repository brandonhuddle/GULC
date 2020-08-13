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
#ifndef GULC_CODETRANSFORMER_HPP
#define GULC_CODETRANSFORMER_HPP

#include <Target.hpp>
#include <string>
#include <vector>
#include <parsing/ASTFile.hpp>
#include <ast/Decl.hpp>
#include <ast/Stmt.hpp>
#include <ast/Expr.hpp>
#include <ast/decls/EnumDecl.hpp>
#include <ast/decls/ExtensionDecl.hpp>
#include <ast/decls/NamespaceDecl.hpp>
#include <ast/decls/FunctionDecl.hpp>
#include <ast/decls/PropertyDecl.hpp>
#include <ast/decls/StructDecl.hpp>
#include <ast/decls/SubscriptOperatorDecl.hpp>
#include <ast/decls/TemplateFunctionDecl.hpp>
#include <ast/decls/TemplateStructDecl.hpp>
#include <ast/decls/TemplateTraitDecl.hpp>
#include <ast/exprs/VariableDeclExpr.hpp>
#include <ast/stmts/CaseStmt.hpp>
#include <ast/stmts/CatchStmt.hpp>
#include <ast/stmts/DoCatchStmt.hpp>
#include <ast/stmts/DoWhileStmt.hpp>
#include <ast/stmts/ForStmt.hpp>
#include <ast/stmts/IfStmt.hpp>
#include <ast/stmts/LabeledStmt.hpp>
#include <ast/stmts/ReturnStmt.hpp>
#include <ast/stmts/SwitchStmt.hpp>
#include <ast/stmts/WhileStmt.hpp>
#include <ast/exprs/ArrayLiteralExpr.hpp>
#include <ast/exprs/AsExpr.hpp>
#include <ast/exprs/AssignmentOperatorExpr.hpp>
#include <ast/exprs/ConstructorCallExpr.hpp>
#include <ast/exprs/MemberPostfixOperatorCallExpr.hpp>
#include <ast/exprs/MemberPrefixOperatorCallExpr.hpp>
#include <ast/exprs/MemberPropertyRefExpr.hpp>
#include <ast/exprs/MemberSubscriptCallExpr.hpp>
#include <ast/exprs/MemberVariableRefExpr.hpp>
#include <ast/exprs/ParenExpr.hpp>
#include <ast/exprs/PropertyRefExpr.hpp>
#include <ast/exprs/SubscriptRefExpr.hpp>
#include <ast/exprs/TernaryExpr.hpp>
#include <ast/exprs/TryExpr.hpp>
#include <ast/exprs/LValueToRValueExpr.hpp>
#include <ast/exprs/ImplicitCastExpr.hpp>
#include <ast/exprs/MemberFunctionCallExpr.hpp>
#include <ast/exprs/IsExpr.hpp>
#include <ast/exprs/LocalVariableRefExpr.hpp>

namespace gulc {
    /**
     * CodeTransformer handles the following:
     *  * Verify all Stmts and Exprs are valid
     *  * Add temporary values to the AST
     *  * Insert destructor calls and other deferred statements
     *
     * The purpose of this is to validate the code that has come in from the source file (mostly unchanged since
     * parsing) then it transforms the code into something much easier for `CodeGen` to work with.
     */
    class CodeTransformer {
    public:
        CodeTransformer(gulc::Target const& target, std::vector<std::string> const& filePaths,
                        std::vector<NamespaceDecl*>& namespacePrototypes)
                : _target(target), _filePaths(filePaths), _namespacePrototypes(namespacePrototypes), _currentFile() {}

        void processFiles(std::vector<ASTFile>& files);

    protected:
        gulc::Target const& _target;
        std::vector<std::string> const& _filePaths;
        std::vector<NamespaceDecl*>& _namespacePrototypes;
        ASTFile* _currentFile;
        // This is the current function being processed (e.g. `init`, `deinit`, `func`, `prop::get`, etc.)
        FunctionDecl* _currentFunction;
        std::vector<ParameterDecl*>* _currentParameters;
        std::vector<VariableDeclExpr*> _localVariables;
        // List of temporary values that will need cleaned up later.
        std::vector<std::vector<VariableDeclExpr*>> _temporaryValues;
        // This is a number kept just to keep temporary value variable names unique
        int _temporaryValueVarNumber = 0;

        void printError(std::string const& message, TextPosition startPosition, TextPosition endPosition) const;
        void printWarning(std::string const& message, TextPosition startPosition, TextPosition endPosition) const;

        void processDecl(Decl* decl);
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
        void processTemplateStructDecl(TemplateStructDecl* templateStructDecl);
        void processTemplateTraitDecl(TemplateTraitDecl* templateTraitDecl);
        void processTraitDecl(TraitDecl* traitDecl);
        void processVariableDecl(VariableDecl* variableDecl);

        void processStmt(Stmt*& stmt);
        void processCaseStmt(CaseStmt* caseStmt);
        void processCatchStmt(CatchStmt* catchStmt);
        void processCompoundStmt(CompoundStmt* compoundStmt);
        // NOTE: This is a special case to process `processCompoundStmt` with temporary value awareness without
        //       allowing the compound statement to be replaced (like `processStmt` allows)
        void processCompoundStmtHandleTempValues(CompoundStmt* compoundStmt);
        void processDoCatchStmt(DoCatchStmt* doCatchStmt);
        void processDoWhileStmt(DoWhileStmt* doWhileStmt);
        void processForStmt(ForStmt* forStmt);
        void processIfStmt(IfStmt* ifStmt);
        // NOTE: This is a special case to process `processIfStmt` with temporary value awareness without
        //       allowing the if statement to be replaced (like `processStmt` allows)
        void processIfStmtHandleTempValues(IfStmt* ifStmt);
        void processLabeledStmt(LabeledStmt* labeledStmt);
        void processReturnStmt(ReturnStmt* returnStmt);
        void processSwitchStmt(SwitchStmt* switchStmt);
        void processWhileStmt(WhileStmt* whileStmt);

        void processExpr(Expr*& expr);
        void processArrayLiteralExpr(ArrayLiteralExpr* arrayLiteralExpr);
        void processAsExpr(AsExpr* asExpr);
        void processAssignmentOperatorExpr(AssignmentOperatorExpr* assignmentOperatorExpr);
        void processConstructorCallExpr(ConstructorCallExpr* constructorCallExpr);
        void processFunctionCallExpr(Expr*& expr);
        void processImplicitCastExpr(ImplicitCastExpr* implicitCastExpr);
        void processInfixOperatorExpr(InfixOperatorExpr* infixOperatorExpr);
        void processIsExpr(IsExpr* isExpr);
        void processLabeledArgumentExpr(LabeledArgumentExpr* labeledArgumentExpr);
        void processLValueToRValueExpr(LValueToRValueExpr* lValueToRValueExpr);
        void processMemberFunctionCallExpr(Expr*& expr);
        void processMemberPostfixOperatorCallExpr(Expr*& expr);
        void processMemberPrefixOperatorCallExpr(Expr*& expr);
        void processMemberPropertyRefExpr(MemberPropertyRefExpr* memberPropertyRefExpr);
        void processMemberSubscriptCallExpr(MemberSubscriptCallExpr* memberSubscriptCallExpr);
        void processMemberVariableRefExpr(MemberVariableRefExpr* memberVariableRefExpr);
        void processParenExpr(ParenExpr* parenExpr);
        void processPostfixOperatorExpr(PostfixOperatorExpr* postfixOperatorExpr);
        void processPrefixOperatorExpr(PrefixOperatorExpr* prefixOperatorExpr);
        void processTernaryExpr(TernaryExpr* ternaryExpr);
        void processTryExpr(TryExpr* tryExpr);
        void processVariableDeclExpr(VariableDeclExpr* variableDeclExpr);

        LocalVariableRefExpr* createTemporaryValue(Type* type, TextPosition startPosition, TextPosition endPosition);

    };
}

#endif //GULC_CODETRANSFORMER_HPP
