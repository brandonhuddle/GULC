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
#ifndef GULC_CODEGEN_HPP
#define GULC_CODEGEN_HPP

#include <Target.hpp>
#include <parsing/ASTFile.hpp>
#include "Module.hpp"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <ast/decls/FunctionDecl.hpp>
#include <ast/decls/VariableDecl.hpp>
#include <ast/decls/TemplateFunctionDecl.hpp>
#include <ast/decls/ConstructorDecl.hpp>
#include <ast/decls/DestructorDecl.hpp>
#include <ast/decls/PropertyDecl.hpp>
#include <ast/decls/StructDecl.hpp>
#include <ast/decls/SubscriptOperatorDecl.hpp>
#include <ast/decls/TemplateStructDecl.hpp>
#include <ast/decls/TraitDecl.hpp>
#include <ast/decls/TemplateTraitDecl.hpp>
#include <ast/decls/EnumDecl.hpp>
#include <ast/stmts/BreakStmt.hpp>
#include <ast/stmts/ContinueStmt.hpp>
#include <ast/stmts/DoWhileStmt.hpp>
#include <ast/stmts/ForStmt.hpp>
#include <ast/stmts/GotoStmt.hpp>
#include <ast/stmts/IfStmt.hpp>
#include <ast/stmts/LabeledStmt.hpp>
#include <ast/stmts/ReturnStmt.hpp>
#include <ast/stmts/SwitchStmt.hpp>
#include <ast/stmts/DoCatchStmt.hpp>
#include <ast/stmts/WhileStmt.hpp>
#include <ast/exprs/ArrayLiteralExpr.hpp>
#include <ast/exprs/AsExpr.hpp>
#include <ast/exprs/AssignmentOperatorExpr.hpp>
#include <ast/exprs/CallOperatorReferenceExpr.hpp>
#include <ast/exprs/ConstructorCallExpr.hpp>
#include <ast/exprs/CurrentSelfExpr.hpp>
#include <ast/exprs/EnumConstRefExpr.hpp>
#include <ast/exprs/ImplicitCastExpr.hpp>
#include <ast/exprs/LocalVariableRefExpr.hpp>
#include <ast/exprs/MemberFunctionCallExpr.hpp>
#include <ast/exprs/MemberPostfixOperatorCallExpr.hpp>
#include <ast/exprs/MemberPrefixOperatorCallExpr.hpp>
#include <ast/exprs/MemberVariableRefExpr.hpp>
#include <ast/exprs/ParameterRefExpr.hpp>
#include <ast/exprs/ParenExpr.hpp>
#include <ast/exprs/TernaryExpr.hpp>
#include <ast/exprs/VariableDeclExpr.hpp>
#include <ast/exprs/VariableRefExpr.hpp>
#include <ast/exprs/VTableFunctionReferenceExpr.hpp>
#include <ast/exprs/LValueToRValueExpr.hpp>
#include <ast/exprs/TryExpr.hpp>
#include <ast/exprs/BoolLiteralExpr.hpp>
#include <ast/exprs/DestructorCallExpr.hpp>
#include <ast/exprs/TemporaryValueRefExpr.hpp>
#include <ast/exprs/RefExpr.hpp>
#include <ast/exprs/ImplicitDerefExpr.hpp>
#include <ast/exprs/PropertyGetCallExpr.hpp>
#include <ast/exprs/PropertySetCallExpr.hpp>
#include <ast/exprs/SubscriptOperatorGetCallExpr.hpp>
#include <ast/exprs/SubscriptOperatorSetCallExpr.hpp>

namespace gulc {
    class CodeGen {
    public:
        CodeGen(Target const& genTarget, std::vector<std::string> const& filePaths)
                : _target(genTarget), _filePaths(filePaths), _currentFile(nullptr),
                  _llvmContext(nullptr), _irBuilder(nullptr), _llvmModule(nullptr), _funcPassManager(nullptr),
                  _currentLlvmFunction(nullptr), _currentGhoulFunction(nullptr), _entryBlockBuilder(nullptr),
                  _currentFunctionExitBlock(nullptr), _currentLoopBlockContinue(nullptr),
                  _currentLoopBlockBreak(nullptr), _anonLoopNameNumber(0) {}

        gulc::Module generate(ASTFile* file);

    protected:
        gulc::Target const& _target;
        std::vector<std::string> const& _filePaths;
        ASTFile* _currentFile;
        llvm::LLVMContext* _llvmContext;
        llvm::IRBuilder<>* _irBuilder;
        llvm::Module* _llvmModule;
        llvm::legacy::FunctionPassManager* _funcPassManager;

        std::map<std::string, llvm::StructType*> _cachedLlvmStructTypes;

        llvm::Function* _currentLlvmFunction;
        gulc::FunctionDecl const* _currentGhoulFunction;
        std::vector<llvm::AllocaInst*> _currentLlvmFunctionParameters;
        llvm::IRBuilder<>* _entryBlockBuilder;
        llvm::BasicBlock* _currentFunctionExitBlock;
        llvm::AllocaInst* _currentFunctionReturnValue;
        std::map<std::string, llvm::BasicBlock*> _currentLlvmFunctionLabels;
        std::vector<llvm::AllocaInst*> _currentLlvmFunctionLocalVariables;
        std::vector<llvm::AllocaInst*> _currentStmtTemporaryValues;

        llvm::BasicBlock* _currentLoopBlockContinue;
        llvm::BasicBlock* _currentLoopBlockBreak;
        std::vector<llvm::BasicBlock*> _nestedLoopContinues;
        std::vector<llvm::BasicBlock*> _nestedLoopBreaks;
        // For unnamed (anonymous) loop names we keep a tally of their numbers for proper naming
        unsigned int _anonLoopNameNumber;

        void printError(std::string const& message, TextPosition startPosition, TextPosition endPosition);

        llvm::Type* generateLlvmType(gulc::Type const* type);
        std::vector<llvm::Type*> generateLlvmParamTypes(std::vector<ParameterDecl*> const& parameters,
                                                        StructDecl const* parentStruct);
        llvm::StructType* generateLlvmStructType(StructDecl const* structDecl, bool unpadded = false);
        // This is meant to grab the size from `constSize`, `constSize` will be required to be a value literal type
        std::uint64_t generateConstSize(Expr* constSize);

        void generateDecl(Decl const* decl, bool isInternal = true);
        void generateConstructorDecl(ConstructorDecl const* constructorDecl, bool isInternal);
        void generateDestructorDecl(DestructorDecl const* destructorDecl, bool isInternal);
        void generateEnumDecl(EnumDecl const* enumDecl, bool isInternal);
        void generateFunctionDecl(FunctionDecl const* functionDecl, bool isInternal);
        void generateNamespaceDecl(NamespaceDecl const* namespaceDecl);
        void generatePropertyDecl(PropertyDecl const* propertyDecl, bool isInternal);
        void generatePropertyGetDecl(PropertyGetDecl const* propertyGetDecl, bool isInternal);
        void generatePropertySetDecl(PropertySetDecl const* propertySetDecl, bool isInternal);
        void generateStructDecl(StructDecl const* structDecl, bool isInternal);
        void generateSubscriptOperatorDecl(SubscriptOperatorDecl const* subscriptOperatorDecl, bool isInternal);
        void generateSubscriptOperatorGetDecl(SubscriptOperatorGetDecl const* subscriptOperatorGetDecl,
                                              bool isInternal);
        void generateSubscriptOperatorSetDecl(SubscriptOperatorSetDecl const* subscriptOperatorSetDecl,
                                              bool isInternal);
        void generateTemplateFunctionDecl(TemplateFunctionDecl const* templateFunctionDecl, bool isInternal);
        void generateTemplateStructDecl(TemplateStructDecl const* templateStructDecl, bool isInternal);
        void generateTemplateTraitDecl(TemplateTraitDecl const* templateTraitDecl, bool isInternal);
        void generateTraitDecl(TraitDecl const* traitDecl, bool isInternal);
        // Generate a global (non-member) variable declaration.
        void generateVariableDecl(VariableDecl const* variableDecl, bool isInternal);

        void setCurrentFunction(llvm::Function* currentFunction, gulc::FunctionDecl const* currentGhoulFunction);
        llvm::Function* getFunction(FunctionDecl* functionDecl);
        bool currentFunctionLabelsContains(std::string const& labelName);
        void addCurrentFunctionLabel(std::string const& labelName, llvm::BasicBlock* basicBlock);
        void addBlockAndSetInsertionPoint(llvm::BasicBlock* basicBlock);
        llvm::BasicBlock* getBreakBlock(std::string const& blockName);
        llvm::BasicBlock* getContinueBlock(std::string const& blockName);

        // Statement Generation
        void generateStmt(Stmt const* stmt, std::string const& stmtName = "");
        void generateBreakStmt(BreakStmt const* breakStmt);
        void generateCompoundStmt(CompoundStmt const* compoundStmt);
        void generateContinueStmt(ContinueStmt const* continueStmt);
        void generateDoCatchStmt(DoCatchStmt const* doCatchStmt);
        void generateDoWhileStmt(DoWhileStmt const* doWhileStmt, std::string const& stmtName);
        void generateForStmt(ForStmt const* forStmt, std::string const& stmtName);
        void generateGotoStmt(GotoStmt const* gotoStmt);
        void generateIfStmt(IfStmt const* ifStmt);
        void generateLabeledStmt(LabeledStmt const* labeledStmt);
        void generateReturnStmt(ReturnStmt const* returnStmt);
        void generateSwitchStmt(SwitchStmt const* switchStmt);
        void generateWhileStmt(WhileStmt const* whileStmt, std::string const& stmtName);

        void enterNestedLoop(llvm::BasicBlock* continueLoop, llvm::BasicBlock* breakLoop,
                             std::size_t* outOldNestedLoopCount);
        void leaveNestedLoop(std::size_t oldNestedLoopCount);
        void cleanupTemporaryValues(std::vector<VariableDeclExpr*> const& temporaryValues);

        // Expression Generation
        llvm::Constant* generateConstant(Expr const* expr);

        llvm::Value* generateExpr(Expr const* expr);
        llvm::Value* generateArrayLiteralExpr(ArrayLiteralExpr const* arrayLiteralExpr);
        llvm::Value* generateAsExpr(AsExpr const* asExpr);
        llvm::Value* generateAssignmentOperatorExpr(AssignmentOperatorExpr const* assignmentOperatorExpr);
        llvm::Value* generateBoolLiteralExpr(BoolLiteralExpr const* boolLiteralExpr);
        llvm::Value* generateCallOperatorReferenceExpr(CallOperatorReferenceExpr const* callOperatorReferenceExpr);
        llvm::Value* generateConstructorCallExpr(ConstructorCallExpr const* constructorCallExpr);
        llvm::Value* generateCurrentSelfExpr(CurrentSelfExpr const* currentSelfExpr);
        llvm::Value* generateDestructorCallExpr(DestructorCallExpr const* destructorCallExpr);
        llvm::Value* generateEnumConstRefExpr(EnumConstRefExpr const* enumConstRefExpr);
        llvm::Value* generateFunctionCallExpr(FunctionCallExpr const* functionCallExpr);
        llvm::Value* generateFunctionReferenceFromExpr(Expr const* expr);
        llvm::Value* generateImplicitCastExpr(ImplicitCastExpr const* implicitCastExpr);
        llvm::Value* generateImplicitDerefExpr(ImplicitDerefExpr const* implicitDerefExpr);
        llvm::Value* generateInfixOperatorExpr(InfixOperatorExpr const* infixOperatorExpr);
        llvm::Value* generateBuiltInInfixOperator(InfixOperators infixOperators, gulc::Type* operationType,
                                                  gulc::Type* leftType, llvm::Value* leftValue,
                                                  gulc::Type* rightType, llvm::Value* rightValue,
                                                  TextPosition startPosition, TextPosition endPosition);
        llvm::Value* generateLocalVariableRefExpr(LocalVariableRefExpr const* localVariableRefExpr);
        llvm::Value* generateLValueToRValueExpr(LValueToRValueExpr const* lValueToRValueExpr);
        llvm::Value* generateMemberFunctionCallExpr(MemberFunctionCallExpr const* memberFunctionCallExpr);
        llvm::Value* generateMemberPostfixOperatorCallExpr(
                MemberPostfixOperatorCallExpr const* memberPostfixOperatorCallExpr);
        llvm::Value* generateMemberPrefixOperatorCallExpr(
                MemberPrefixOperatorCallExpr const* memberPrefixOperatorCallExpr);
        llvm::Value* generateMemberVariableRefExpr(MemberVariableRefExpr const* memberVariableRefExpr);
        llvm::Value* generateParameterRefExpr(ParameterRefExpr const* parameterRefExpr);
        llvm::Value* generateParenExpr(ParenExpr const* parenExpr);
        llvm::Value* generatePostfixOperatorExpr(PostfixOperatorExpr const* postfixOperatorExpr);
        llvm::Value* generatePrefixOperatorExpr(PrefixOperatorExpr const* prefixOperatorExpr);
        llvm::Value* generatePropertyGetCallExpr(PropertyGetCallExpr const* propertyGetCallExpr);
        llvm::Value* generatePropertySetCallExpr(PropertySetCallExpr const* propertySetCallExpr);
        llvm::Value* generateRefExpr(RefExpr const* refExpr);
        llvm::Value* generateSubscriptOperatorGetCallExpr(
                SubscriptOperatorGetCallExpr const* subscriptOperatorGetCallExpr);
        llvm::Value* generateSubscriptOperatorSetCallExpr(
                SubscriptOperatorSetCallExpr const* subscriptOperatorSetCallExpr);
        llvm::Value* generateTemporaryValueRefExpr(TemporaryValueRefExpr const* temporaryValueRefExpr);
        llvm::Value* generateTemporaryValueVariableDeclExpr(VariableDeclExpr const* variableDeclExpr);
        llvm::Value* generateTernaryExpr(TernaryExpr const* ternaryExpr);
        llvm::Value* generateTryExpr(TryExpr const* tryExpr);
        llvm::Value* generateValueLiteralExpr(ValueLiteralExpr const* valueLiteralExpr);
        llvm::Value* generateVariableDeclExpr(VariableDeclExpr const* variableDeclExpr);
        llvm::Value* generateVariableRefExpr(VariableRefExpr const* variableRefExpr);
        llvm::Value* generateVTableFunctionReferenceExpr(
                VTableFunctionReferenceExpr const* vTableFunctionReferenceExpr);

        llvm::Value* dereferenceReference(llvm::Value* value);
        void castValue(gulc::Type* to, gulc::Type* from, llvm::Value*& value,
                       TextPosition const& startPosition, TextPosition const& endPosition);
        llvm::AllocaInst* addLocalVariable(std::string const& varName, llvm::Type* llvmType);
        llvm::AllocaInst* getLocalVariableOrNull(std::string const& varName);
        llvm::AllocaInst* addTemporaryValue(std::string const& tmpName, llvm::Type* llvmType);
        llvm::AllocaInst* getTemporaryValueOrNull(std::string const& tmpName);
        llvm::Function* getMoveConstructorForType(gulc::Type* type);
        llvm::Function* getCopyConstructorForType(gulc::Type* type);

    };
}

#endif //GULC_CODEGEN_HPP
