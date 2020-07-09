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
#include <ast/stmts/DoStmt.hpp>
#include <ast/stmts/ForStmt.hpp>
#include <ast/stmts/GotoStmt.hpp>
#include <ast/stmts/IfStmt.hpp>
#include <ast/stmts/LabeledStmt.hpp>
#include <ast/stmts/ReturnStmt.hpp>
#include <ast/stmts/SwitchStmt.hpp>
#include <ast/stmts/TryStmt.hpp>
#include <ast/stmts/WhileStmt.hpp>
#include <ast/exprs/ArrayLiteralExpr.hpp>
#include <ast/exprs/AsExpr.hpp>
#include <ast/exprs/AssignmentOperatorExpr.hpp>
#include <ast/exprs/CheckExtendsTypeExpr.hpp>
#include <ast/exprs/FunctionCallExpr.hpp>
#include <ast/exprs/HasExpr.hpp>
#include <ast/exprs/IndexerCallExpr.hpp>
#include <ast/exprs/IsExpr.hpp>
#include <ast/exprs/LabeledArgumentExpr.hpp>
#include <ast/exprs/MemberAccessCallExpr.hpp>
#include <ast/exprs/ParenExpr.hpp>
#include <ast/exprs/PostfixOperatorExpr.hpp>
#include <ast/exprs/PrefixOperatorExpr.hpp>
#include <ast/exprs/TemplateConstRefExpr.hpp>
#include <ast/exprs/TernaryExpr.hpp>
#include <ast/exprs/VariableDeclExpr.hpp>

namespace gulc {
    /**
     * CodeProcessor is the main processor. It handles processing the body of functions, resolving `Decl` references,
     * etc.
     */
    // TODO: At this point all redefinition checks for `Decl`s have already been finished so we don't have to handle
    //       those. What we do need to handle are:
    //        * Process `Stmt`
    //        * Resolve ALL `Expr`s to their referenced `Decl` or local variables
    //        * Pass any new template instantiations back through `DeclInstantiator`?
    //        * Resolve any operator overloading, subscript overloading, etc. to their `Decl`
    //        * Handle giving all `Expr`s their `Type`
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

        gulc::Target const& _target;
        std::vector<std::string> const& _filePaths;
        std::vector<NamespaceDecl*>& _namespacePrototypes;
        ASTFile* _currentFile;
        // The current container decl (NOTE: This is one of `NamespaceDecl`, `StructDecl`, etc. this is NOT the
        // current `Decl` being processed such as `FunctionDecl` etc.)
        Decl* _currentContainer;

        std::vector<std::vector<TemplateParameterDecl*>*> _allTemplateParameters;
        std::vector<ParameterDecl*>* _currentParameters;
        std::vector<VariableDeclExpr*> _localVariables;

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

        void processStmt(Stmt* stmt);
        void processBreakStmt(BreakStmt* breakStmt);
        void processCaseStmt(CaseStmt* caseStmt);
        void processCatchStmt(CatchStmt* catchStmt);
        void processCompoundStmt(CompoundStmt* compoundStmt);
        void processContinueStmt(ContinueStmt* continueStmt);
        void processDoStmt(DoStmt* doStmt);
        void processForStmt(ForStmt* forStmt);
        void processGotoStmt(GotoStmt* gotoStmt);
        void processIfStmt(IfStmt* ifStmt);
        void processLabeledStmt(LabeledStmt* labeledStmt);
        void processReturnStmt(ReturnStmt* returnStmt);
        void processSwitchStmt(SwitchStmt* switchStmt);
        void processTryStmt(TryStmt* tryStmt);
        void processWhileStmt(WhileStmt* whileStmt);

        void processExpr(Expr* expr);
        void processArrayLiteralExpr(ArrayLiteralExpr* arrayLiteralExpr);
        void processAsExpr(AsExpr* asExpr);
        void processAssignmentOperatorExpr(AssignmentOperatorExpr* assignmentOperatorExpr);
        void processCheckExtendsTypeExpr(CheckExtendsTypeExpr* checkExtendsTypeExpr);
        void processFunctionCallExpr(FunctionCallExpr* functionCallExpr);
        // NOTE: Returns true if a single match was found
        bool fillListOfMatchingConstructors(Type* fromType, std::vector<LabeledArgumentExpr*> const& arguments,
                                            std::vector<MatchingDecl>& matchingDecls);
        bool fillListOfMatchingCallOperators(Type* fromType, std::vector<LabeledArgumentExpr*> const& arguments,
                                             std::vector<MatchingDecl>& matchingDecls);
        bool fillListOfMatchingFunctors(Decl* fromContainer, IdentifierExpr* identifierExpr,
                                        std::vector<LabeledArgumentExpr*> const& arguments,
                                        std::vector<MatchingDecl>& matchingDecls);
        void processHasExpr(HasExpr* hasExpr);
        void processIdentifierExpr(IdentifierExpr* identifierExpr);
        void processIndexerCallExpr(IndexerCallExpr* indexerCallExpr);
        void processInfixOperatorExpr(InfixOperatorExpr* infixOperatorExpr);
        void processIsExpr(IsExpr* isExpr);
        void processLabeledArgumentExpr(LabeledArgumentExpr* labeledArgumentExpr);
        void processMemberAccessCallExpr(MemberAccessCallExpr* memberAccessCallExpr);
        void processParenExpr(ParenExpr* parenExpr);
        void processPostfixOperatorExpr(PostfixOperatorExpr* postfixOperatorExpr);
        void processPrefixOperatorExpr(PrefixOperatorExpr* prefixOperatorExpr);
        void processTemplateConstRefExpr(TemplateConstRefExpr* templateConstRefExpr);
        void processTernaryExpr(TernaryExpr* ternaryExpr);
        void processTypeExpr(TypeExpr* typeExpr);
        void processValueLiteralExpr(ValueLiteralExpr* valueLiteralExpr);
        void processVariableDeclExpr(VariableDeclExpr* variableDeclExpr);

    };
}

#endif //GULC_CODEPROCESSOR_HPP
