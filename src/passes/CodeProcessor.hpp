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
#ifndef GULC_CODEPROCESSOR_HPP
#define GULC_CODEPROCESSOR_HPP

#include <vector>
#include <Target.hpp>
#include <string>
#include <parsing/ASTFile.hpp>
#include <ast/decls/CallOperatorDecl.hpp>
#include <ast/decls/ConstructorDecl.hpp>
#include <ast/decls/DestructorDecl.hpp>
#include <ast/decls/EnumDecl.hpp>
#include <ast/decls/ExtensionDecl.hpp>
#include <ast/decls/OperatorDecl.hpp>
#include <ast/decls/PropertyDecl.hpp>
#include <ast/decls/VariableDecl.hpp>
#include <ast/decls/TraitDecl.hpp>
#include <ast/decls/StructDecl.hpp>
#include <ast/decls/SubscriptOperatorDecl.hpp>
#include <ast/decls/TemplateFunctionDecl.hpp>
#include <ast/decls/TemplateStructDecl.hpp>
#include <ast/decls/TemplateTraitDecl.hpp>
#include <ast/stmts/BreakStmt.hpp>
#include <ast/stmts/CaseStmt.hpp>
#include <ast/stmts/CatchStmt.hpp>
#include <ast/stmts/ContinueStmt.hpp>
#include <ast/stmts/RepeatWhileStmt.hpp>
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
#include <ast/exprs/CheckExtendsTypeExpr.hpp>
#include <ast/exprs/FunctionCallExpr.hpp>
#include <ast/exprs/HasExpr.hpp>
#include <ast/exprs/SubscriptCallExpr.hpp>
#include <ast/exprs/IsExpr.hpp>
#include <ast/exprs/LabeledArgumentExpr.hpp>
#include <ast/exprs/MemberAccessCallExpr.hpp>
#include <ast/exprs/ParenExpr.hpp>
#include <ast/exprs/PostfixOperatorExpr.hpp>
#include <ast/exprs/PrefixOperatorExpr.hpp>
#include <ast/exprs/TemplateConstRefExpr.hpp>
#include <ast/exprs/TernaryExpr.hpp>
#include <ast/exprs/VariableDeclExpr.hpp>
#include <ast/exprs/CurrentSelfExpr.hpp>
#include <ast/exprs/EnumConstRefExpr.hpp>
#include <ast/exprs/FunctionReferenceExpr.hpp>
#include <ast/exprs/PropertyRefExpr.hpp>
#include <ast/exprs/VariableRefExpr.hpp>
#include <ast/exprs/MemberVariableRefExpr.hpp>
#include <ast/exprs/MemberPropertyRefExpr.hpp>
#include <ast/exprs/ConstructorCallExpr.hpp>
#include <ast/exprs/CallOperatorReferenceExpr.hpp>
#include <ast/exprs/MemberPostfixOperatorCallExpr.hpp>
#include <ast/exprs/MemberPrefixOperatorCallExpr.hpp>
#include <ast/exprs/TryExpr.hpp>
#include <ast/exprs/BoolLiteralExpr.hpp>
#include <ast/exprs/RefExpr.hpp>
#include <ast/exprs/SubscriptOperatorRefExpr.hpp>
#include <ast/exprs/MemberSubscriptOperatorRefExpr.hpp>
#include <utilities/SignatureComparer.hpp>
#include <ast/decls/TraitPrototypeDecl.hpp>

namespace gulc {
    /**
     * CodeProcessor is the main processor. It handles processing the body of functions, resolving `Decl` references,
     * etc.
     */
    // TODO: At this point all redefinition checks for `Decl`s have already been finished so we don't have to handle
    //       those. What we do need to handle are:
    //        * Process `Stmt`
    //        * Pass any new template instantiations back through `DeclInstantiator`?
    //        * Validate all `Expr` usages (i.e. `12 + "44"` should error if there isn't an overload allowing it)
    //        * Apply any implicit cast
    //        * Validate `mut`, `immut`, and `const` usages. (i.e. `const` cannot be reassigned, they're constants;
    //          `immut` cannot be modified, `mut` is needed to modify, etc.)
    //        * ???
    class CodeProcessor {
    public:
        CodeProcessor(gulc::Target const& target, std::vector<std::string> const& filePaths,
                      std::vector<NamespaceDecl*>& namespacePrototypes)
                : _target(target), _filePaths(filePaths), _namespacePrototypes(namespacePrototypes), _currentFile(),
                  _currentContainer(nullptr), _currentParameters(nullptr) {}

        void processFiles(std::vector<ASTFile>& files);

    protected:
        struct MatchingDecl {
            enum class Kind {
                Unknown,
                // It is a match without needing any casts
                Match,
                // It is a match but requires default values
                DefaultValues,
                // It is a match but requires some casts
                Castable
            };

            Kind kind;
            Decl* matchingDecl;

            MatchingDecl()
                    : kind(Kind::Unknown), matchingDecl(nullptr) {}

            MatchingDecl(Kind kind, Decl* matchingDecl)
                    : kind(kind), matchingDecl(matchingDecl) {}

        };

        struct MatchingTemplateDecl {
            MatchingDecl::Kind kind;
            Decl* matchingDecl;
            std::vector<std::size_t> argMatchStrengths;

            MatchingTemplateDecl()
                    : kind(MatchingDecl::Kind::Unknown), matchingDecl(nullptr), argMatchStrengths() {}

            MatchingTemplateDecl(MatchingDecl::Kind kind, Decl* matchingDecl, std::vector<size_t> argMatchStrengths)
                    : kind(kind), matchingDecl(matchingDecl), argMatchStrengths(std::move(argMatchStrengths)) {}


        };

        struct MatchingFunctorDecl {
            MatchingDecl::Kind kind;
            Decl* functorDecl;
            // Depending on `functorDecl` this could be filled or could not be. It all depends.
            // E.g. `VariableDecl` with a `CallOperatorDecl`. The `functor` would be the call operator and the `context`
            //      would be the `VariableDecl`.
            // NOTE: `VariableDecl` can also be the `functor` when `VariableDecl::type == FunctionPointerType`
            Decl* contextDecl;

            MatchingFunctorDecl()
                    : kind(MatchingDecl::Kind::Unknown), functorDecl(nullptr), contextDecl(nullptr) {}

            MatchingFunctorDecl(MatchingDecl::Kind kind, Decl* functorDecl, Decl* contextDecl)
                    : kind(kind), functorDecl(functorDecl), contextDecl(contextDecl) {}

        };

        gulc::Target const& _target;
        std::vector<std::string> const& _filePaths;
        std::vector<NamespaceDecl*>& _namespacePrototypes;
        ASTFile* _currentFile;
        // The current container decl (NOTE: This is one of `NamespaceDecl`, `StructDecl`, etc. this is NOT the
        // current `Decl` being processed such as `FunctionDecl` etc.)
        Decl* _currentContainer;
        // This is the current function being processed (e.g. `init`, `deinit`, `func`, `prop::get`, etc.)
        FunctionDecl* _currentFunction;

        std::vector<std::vector<TemplateParameterDecl*>*> _allTemplateParameters;
        std::vector<ParameterDecl*>* _currentParameters;
        std::vector<VariableDeclExpr*> _localVariables;
        // List of resolved and unresolved labels (if the boolean is true then it is resolved, else it isn't found)
        std::map<std::string, bool> _labelNames;

        void printError(std::string const& message, TextPosition startPosition, TextPosition endPosition) const;
        void printWarning(std::string const& message, TextPosition startPosition, TextPosition endPosition) const;

        /// Get a new reference to the current `self` or print an error if that is impossible.
        CurrentSelfExpr* getCurrentSelfRef(TextPosition startPosition, TextPosition endPosition) const;

        void processDecl(Decl* decl);
        void processPrototypeDecl(Decl* decl);
        void processConstructorDecl(ConstructorDecl* constructorDecl, CompoundStmt* temporaryInitializersCompoundStmt);
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
        void processTraitPrototypeDecl(TraitPrototypeDecl* traitPrototypeDecl);
        void processVariableDecl(VariableDecl* variableDecl);

        void processBaseConstructorCall(StructDecl* structDecl, FunctionCallExpr*& functionCallExpr);
        ConstructorCallExpr* createBaseMoveCopyConstructorCall(ConstructorDecl* baseConstructorDecl,
                                                               ParameterDecl* otherParameterDecl,
                                                               TextPosition startPosition,
                                                               TextPosition endPosition);

        // Creates a compound statement containing initializers for every member in the struct.
        // NOTE: The compound statement will NOT already be processed by `CodeProcessor` you will have to do that
        //       yourself!
        CompoundStmt* createStructMemberInitializersCompoundStmt(StructDecl* structDecl);

        // NOTE: Returns true if the statement returns on all code paths.
        void processStmt(Stmt*& stmt);
        void processBreakStmt(BreakStmt* breakStmt);
        void processCaseStmt(CaseStmt* caseStmt);
        void processCatchStmt(CatchStmt* catchStmt);
        void processCompoundStmt(CompoundStmt* compoundStmt);
        void processContinueStmt(ContinueStmt* continueStmt);
        void processDoCatchStmt(DoCatchStmt* doCatchStmt);
        void processForStmt(ForStmt* forStmt);
        void processGotoStmt(GotoStmt* gotoStmt);
        void processIfStmt(IfStmt* ifStmt);
        void processLabeledStmt(LabeledStmt* labeledStmt);
        void processRepeatWhileStmt(RepeatWhileStmt* repeatWhileStmt);
        void processReturnStmt(ReturnStmt* returnStmt);
        void processSwitchStmt(SwitchStmt* switchStmt);
        void processWhileStmt(WhileStmt* whileStmt);

        void labelResolved(std::string const& labelName);
        void addUnresolvedLabel(std::string const& labelName);

        void processExpr(Expr*& expr);
        void processArrayLiteralExpr(ArrayLiteralExpr* arrayLiteralExpr);
        void processAsExpr(AsExpr* asExpr);
        void processAssignmentOperatorExpr(Expr*& expr);
        void processBoolLiteralExpr(BoolLiteralExpr* boolLiteralExpr);
        void processCallOperatorReferenceExpr(CallOperatorReferenceExpr* callOperatorReferenceExpr);
        void processCheckExtendsTypeExpr(CheckExtendsTypeExpr* checkExtendsTypeExpr);
        void processConstructorCallExpr(ConstructorCallExpr* constructorCallExpr);
        void processConstructorReferenceExpr(ConstructorReferenceExpr* constructorReferenceExpr);
        void processEnumConstRefExpr(EnumConstRefExpr* enumConstRefExpr);
        void processFunctionCallExpr(FunctionCallExpr*& functionCallExpr);
        void fillListOfMatchingTemplatesInContainer(Decl* container, std::string const& findName,
                                                    bool findStaticOnly, std::vector<Expr*> const& templateArguments,
                                                    std::vector<MatchingTemplateDecl>& matchingTemplateDecls);
        void fillListOfMatchingTemplates(std::vector<Decl*>& searchDecls, std::string const& findName,
                                         bool findStaticOnly, std::vector<Expr*> const& templateArguments,
                                         std::vector<MatchingTemplateDecl>& matchingTemplateDecls);
        void fillListOfMatchingConstructors(StructDecl* structDecl, std::vector<LabeledArgumentExpr*> const& arguments,
                                            std::vector<MatchingFunctorDecl>& matchingDecls);
        bool findBestTemplateConstructor(TemplateStructDecl* templateStructDecl,
                                         std::vector<Expr*> const& templateArguments,
                                         std::vector<LabeledArgumentExpr*> const& arguments,
                                         ConstructorDecl** outMatchingConstructor,
                                         SignatureComparer::ArgMatchResult* outArgMatchResult, bool* outIsAmbiguous);
        void fillListOfMatchingFunctorsInContainer(Decl* container, std::string const& findName, bool findStaticOnly,
                                                   std::vector<LabeledArgumentExpr*> const& arguments,
                                                   std::vector<MatchingFunctorDecl>& outMatchingDecls);
        void fillListOfMatchingFunctors(std::vector<Decl*>& searchDecls, std::string const& findName,
                                        bool findStaticOnly, std::vector<LabeledArgumentExpr*> const& arguments,
                                        std::vector<MatchingFunctorDecl>& outMatchingDecls);
        // Supports `FunctionPointerType` and `CallOperatorDecl`
        void fillListOfMatchingFunctorsInType(Type* checkType, Decl* contextDecl,
                                              std::vector<LabeledArgumentExpr*> const& arguments,
                                              std::vector<MatchingFunctorDecl>& outMatchingDecls);
        void fillListOfMatchingCallOperators(std::vector<Decl*>& searchDecls, bool findMut, Decl* contextDecl,
                                             std::vector<LabeledArgumentExpr*> const& arguments,
                                             std::vector<MatchingFunctorDecl>& outMatchingDecls);
        MatchingTemplateDecl* findMatchingTemplateDecl(
                std::vector<std::vector<MatchingTemplateDecl>>& allMatchingTemplateDecls,
                std::vector<Expr*> const& templateArguments, std::vector<LabeledArgumentExpr*> const& arguments,
                TextPosition errorStartPosition, TextPosition errorEndPosition, bool* outIsAmbiguous);
        MatchingFunctorDecl* findMatchingFunctorDecl(std::vector<MatchingFunctorDecl>& allMatchingFunctorDecls,
                                                     bool findExactOnly, bool* outIsAmbiguous);
        MatchingFunctorDecl getTemplateFunctorInstantiation(MatchingTemplateDecl* matchingTemplateDecl,
                                                            std::vector<Expr*>& templateArguments,
                                                            std::vector<LabeledArgumentExpr*> const& arguments,
                                                            std::string const& errorString,
                                                            TextPosition errorStartPosition,
                                                            TextPosition errorEndPosition);
        /// Create a static reference to the specified function. Function could be a template or normal.
        Expr* createStaticFunctionReference(Expr* forExpr, Decl* function) const;
        void processFunctionReferenceExpr(FunctionReferenceExpr* functionReferenceExpr);
        void processHasExpr(HasExpr* hasExpr);
        void processIdentifierExpr(Expr*& expr);
        bool findMatchingDeclInContainer(Decl* container, std::string const& findName, Decl** outFoundDecl,
                                         bool* outIsAmbiguous);
        // TODO: This can probably be merged with `findMatchingMemberDecl`
        bool findMatchingDecl(std::vector<Decl*> const& searchDecls, std::string const& findName,
                              Decl** outFoundDecl, bool* outIsAmbiguous);
        void processInfixOperatorExpr(InfixOperatorExpr*& infixOperatorExpr);
        bool fillListOfMatchingInfixOperators(std::vector<Decl*>& decls, InfixOperators findOperator, Type* argType,
                                              std::vector<MatchingDecl>& matchingDecls);
        void processIsExpr(IsExpr* isExpr);
        void processLabeledArgumentExpr(LabeledArgumentExpr* labeledArgumentExpr);
        void processMemberAccessCallExpr(Expr*& expr);
        Decl* findMatchingMemberDecl(std::vector<Decl*> const& searchDecls, std::string const& findName,
                                     bool searchForStatic, bool* outIsAmbiguous);
        void processMemberPostfixOperatorCallExpr(MemberPostfixOperatorCallExpr* memberPostfixOperatorCallExpr);
        void processMemberPrefixOperatorCallExpr(MemberPrefixOperatorCallExpr* memberPrefixOperatorCallExpr);
        void processMemberPropertyRefExpr(MemberPropertyRefExpr* memberPropertyRefExpr);
        void processMemberSubscriptOperatorRefExpr(MemberSubscriptOperatorRefExpr* memberSubscriptOperatorRefExpr);
        void processMemberVariableRefExpr(MemberVariableRefExpr* memberVariableRefExpr);
        void processParenExpr(ParenExpr* parenExpr);
        void processPostfixOperatorExpr(PostfixOperatorExpr*& postfixOperatorExpr);
        void processPrefixOperatorExpr(PrefixOperatorExpr*& prefixOperatorExpr);
        void processPropertyRefExpr(PropertyRefExpr* propertyRefExpr);
        void processRefExpr(RefExpr* refExpr);
        void processSubscriptCallExpr(Expr*& expr);
        SubscriptOperatorDecl* findMatchingSubscriptOperator(std::vector<Decl*>& searchDecls,
                                                             std::vector<LabeledArgumentExpr*>& arguments,
                                                             bool findStatic, bool* outIsAmbiguous);
        void processSubscriptOperatorRefExpr(SubscriptOperatorRefExpr* subscriptOperatorRefExpr);
        void processTemplateConstRefExpr(TemplateConstRefExpr* templateConstRefExpr);
        void processTernaryExpr(TernaryExpr* ternaryExpr);
        void processTryExpr(TryExpr* tryExpr);
        void processTypeExpr(TypeExpr* typeExpr);
        void processValueLiteralExpr(ValueLiteralExpr* valueLiteralExpr);
        void processVariableDeclExpr(VariableDeclExpr* variableDeclExpr);
        void processVariableRefExpr(VariableRefExpr* variableRefExpr);

        // If the expression is an lvalue we wrap it in an lvalue to rvalue converter.
        Expr* convertLValueToRValue(Expr* potentialLValue) const;
        /// Cast arguments and convert any lvalues to rvalues when required
        void handleArgumentCasting(std::vector<ParameterDecl*> const& parameters,
                                   std::vector<LabeledArgumentExpr*>& arguments);
        // Returns true if the argument potentially had a type change
        bool handleArgumentCasting(ParameterDecl* parameter, Expr*& argument);
        // If the expression is a reference we dereference it.
        Expr* dereferenceReference(Expr* potentialReference) const;

        // TODO: Cleanup `handleGetter` and `handleRefGetter`. They both repeat a lot of the same code between
        //       properties and subscripts.
        // If the expression is a property or subscript we call the getter for it.
        Expr* handleGetter(Expr* potentialGetSet) const;
        Expr* handleRefGetter(PropertyRefExpr* propertyRefExpr, bool isRefMut) const;
        Expr* handleRefGetter(SubscriptOperatorRefExpr* subscriptOperatorRefExpr, bool isRefMut) const;

    };
}

#endif //GULC_CODEPROCESSOR_HPP
