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
#include <ast/decls/TypeAliasDecl.hpp>
#include <ast/decls/EnumDecl.hpp>
#include <ast/decls/ExtensionDecl.hpp>
#include <queue>
#include <set>
#include <ast/exprs/CheckExtendsTypeExpr.hpp>
#include <ast/conts/WhereCont.hpp>
#include <ast/exprs/TemplateConstRefExpr.hpp>
#include <ast/exprs/TryExpr.hpp>
#include <ast/decls/TraitPrototypeDecl.hpp>
#include <ast/decls/ImaginaryTypeDecl.hpp>

namespace gulc {
    struct TemplateDeclMatch {
        enum class Kind {
            // The match is entirely exact
            Exact,
            // The match is exact but requires parameter default values
            DefaultValues,
            // The match is not exact, you need to apply implicit casts for it to work.
            Castable,
            Unknown
        };

        Kind kind;
        Decl* decl;
        // For template types that are specialized we need to know the `strength` of the specialized type match
        // (NOTE: Strength is distance in the inheritance list from one type to the other)
        std::vector<std::size_t> argMatchStrengths;

        TemplateDeclMatch(Kind kind, Decl* decl, std::vector<std::size_t> argMatchStrengths)
                : kind(kind), decl(decl), argMatchStrengths(std::move(argMatchStrengths)) {}

    };

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
                : _target(target), _filePaths(filePaths), _files(nullptr), _currentFile() {}

        void processFiles(std::vector<ASTFile>& files);
        TemplateStructInstDecl* instantiateTemplateStruct(ASTFile* file, TemplateStructDecl* templateStructDecl,
                                                          std::vector<Expr*>& templateArguments,
                                                          std::string const& errorMessageName,
                                                          TextPosition errorStartPosition,
                                                          TextPosition errorEndPosition);
        TemplateTraitInstDecl* instantiateTemplateTrait(ASTFile* file, TemplateTraitDecl* templateTraitDecl,
                                                        std::vector<Expr*>& templateArguments,
                                                        std::string const& errorMessageName,
                                                        TextPosition errorStartPosition,
                                                        TextPosition errorEndPosition);
        TemplateFunctionInstDecl* instantiateTemplateFunction(ASTFile* file, TemplateFunctionDecl* templateFunctionDecl,
                                                              std::vector<Expr*>& templateArguments,
                                                              std::string const& errorMessageName,
                                                              TextPosition errorStartPosition,
                                                              TextPosition errorEndPosition);

    private:
        gulc::Target const& _target;
        std::vector<std::string> const& _filePaths;
        std::vector<ASTFile>* _files;
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
        // List of `Decl`s that have had their instantiation delayed for any reason
        std::set<gulc::Decl*> _delayInstantiationDeclsSet;
        std::queue<gulc::Decl*> _delayInstantiationDecls;

        void handleDelayedInstantiationDecls();

        void printError(std::string const& message, TextPosition startPosition, TextPosition endPosition) const;
        void printWarning(std::string const& message, TextPosition startPosition, TextPosition endPosition) const;

        // `delayInstantiation` will make it so the instantiation of uninstantiated `Decl`s will be be delayed until
        // the end of the `ASTFile` processing at the latest.
        // `containDependents` means if the type is found to be contained within a template we will make it a dependent
        bool resolveType(Type*& type, bool delayInstantiation = false, bool containDependents = true);
        Type* resolveDependentType(std::vector<Decl*>& checkDecls, Identifier const& identifier,
                                   std::vector<Expr*>& templateArguments);

        void compareDeclTemplateArgsToParams(std::vector<Expr*> const& args,
                                             std::vector<TemplateParameterDecl*> const& params,
                                             bool* outIsMatch, bool* outIsExact,
                                             std::vector<std::size_t>& outArgMatchStrengths) const;

        // Process the where contracts and output an easy to read error message for any failed contracts
        void errorOnWhereContractFailure(std::vector<Cont*> const& contracts,
                                         std::vector<Expr*> const& args,
                                         std::vector<TemplateParameterDecl*> const& params,
                                         std::string const& errorMessageName,
                                         TextPosition errorStartPosition, TextPosition errorEndPosition);

        void processContract(Cont* contract);

        void processDecl(Decl* decl, bool isGlobal = true);
        void processPrototypeDecl(Decl* decl, bool isGlobal = true);
        void processEnumDecl(EnumDecl* enumDecl);
        void processExtensionDecl(ExtensionDecl* extensionDecl);
        void processFunctionDecl(FunctionDecl* functionDecl);
        void processNamespaceDecl(NamespaceDecl* namespaceDecl);
        void processParameterDecl(ParameterDecl* parameterDecl);
        void processPropertyDecl(PropertyDecl* propertyDecl);
        void processPropertyGetDecl(PropertyGetDecl* propertyGetDecl);
        void processPropertySetDecl(PropertySetDecl* propertySetDecl);
        void processStructDeclInheritedTypes(StructDecl* structDecl);
        void processStructDecl(StructDecl* structDecl, bool calculateSizeAndVTable = true);
        void processSubscriptOperatorDecl(SubscriptOperatorDecl* subscriptOperatorDecl);
        void processSubscriptOperatorGetDecl(SubscriptOperatorGetDecl* subscriptOperatorGetDecl);
        void processSubscriptOperatorSetDecl(SubscriptOperatorSetDecl* subscriptOperatorSetDecl);
        void processTemplateFunctionDecl(TemplateFunctionDecl* templateFunctionDecl);
        void processTemplateFunctionDeclContracts(TemplateFunctionDecl* templateFunctionDecl);
        void processTemplateFunctionInstDecl(TemplateFunctionInstDecl* templateFunctionInstDecl);
        void processTemplateParameterDecl(TemplateParameterDecl* templateParameterDecl);
        void processTemplateStructDecl(TemplateStructDecl* templateStructDecl);
        void processTemplateStructDeclContracts(TemplateStructDecl* templateStructDecl);
        void processTemplateStructInstDecl(TemplateStructInstDecl* templateStructInstDecl);
        void processTemplateTraitDecl(TemplateTraitDecl* templateTraitDecl);
        void processTemplateTraitDeclContracts(TemplateTraitDecl* templateTraitDecl);
        void processTemplateTraitInstDecl(TemplateTraitInstDecl* templateTraitInstDecl);
        void processTraitDeclInheritedTypes(TraitDecl* traitDecl);
        void processTraitDecl(TraitDecl* traitDecl);
        void processTraitPrototypeDecl(TraitPrototypeDecl* traitPrototypeDecl);
        void processTypeAliasDecl(TypeAliasDecl* typeAliasDecl);
        void processTypeAliasDeclContracts(TypeAliasDecl* typeAliasDecl);
        void processVariableDecl(VariableDecl* variableDecl, bool isGlobal);

        // This will process the `where` contract and modify the referenced template parameter accordingly
        // (e.g. this will fill `TemplateParameterDecl::inheritedTypes` etc.)
        void descriptTemplateParameterForWhereCont(WhereCont* whereCont);
        std::vector<Expr*> createImaginaryTemplateArguments(std::vector<TemplateParameterDecl*>& templateParameters,
                                                            std::vector<Cont*>& contracts);
        void createValidationTemplateFunctionInstDecl(TemplateFunctionDecl* templateFunctionDecl);
        void createValidationTemplateStructInstDecl(TemplateStructDecl* templateStructDecl);
        void createValidationTemplateTraitInstDecl(TemplateTraitDecl* templateTraitDecl);
        void processImaginaryTypeDecl(ImaginaryTypeDecl* imaginaryTypeDecl);

        // This will process a `Decl` while also checking for any circular dependencies using `_workingDecls`
        void processDependantDecl(Decl* decl);

        bool structUsesStructTypeAsValue(StructDecl* structType, StructDecl* checkStruct, bool checkBaseStruct);

        void processStmt(Stmt* stmt);
        void processCaseStmt(CaseStmt* caseStmt);
        void processCatchStmt(CatchStmt* catchStmt);
        void processCompoundStmt(CompoundStmt* compoundStmt);
        void processDoCatchStmt(DoCatchStmt* doCatchStmt);
        void processForStmt(ForStmt* forStmt);
        void processIfStmt(IfStmt* ifStmt);
        void processLabeledStmt(LabeledStmt* labeledStmt);
        void processRepeatWhileStmt(RepeatWhileStmt* repeatWhileStmt);
        void processReturnStmt(ReturnStmt* returnStmt);
        void processSwitchStmt(SwitchStmt* switchStmt);
        void processWhileStmt(WhileStmt* whileStmt);

        void processConstExpr(Expr* expr);
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
        void processSubscriptCallExpr(SubscriptCallExpr* subscriptCallExpr);
        void processTemplateConstRefExpr(TemplateConstRefExpr* templateConstRefExpr);
        void processTernaryExpr(TernaryExpr* ternaryExpr);
        void processTryExpr(TryExpr* tryExpr);
        void processTypeExpr(TypeExpr* typeExpr);
        void processValueLiteralExpr(ValueLiteralExpr* valueLiteralExpr) const;
        void processVariableDeclExpr(VariableDeclExpr* variableDeclExpr);

    };
}

#endif //GULC_DECLINSTANTIATOR_HPP
