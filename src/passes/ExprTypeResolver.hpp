#ifndef GULC_EXPRTYPERESOLVER_HPP
#define GULC_EXPRTYPERESOLVER_HPP

#include <vector>
#include <string>
#include <parsing/ASTFile.hpp>
#include <ast/Type.hpp>
#include <ast/decls/FunctionDecl.hpp>
#include <ast/decls/NamespaceDecl.hpp>
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
#include <ast/exprs/AsExpr.hpp>
#include <ast/exprs/AssignmentOperatorExpr.hpp>
#include <ast/exprs/FunctionCallExpr.hpp>
#include <ast/exprs/IdentifierExpr.hpp>
#include <ast/exprs/IndexerCallExpr.hpp>
#include <ast/exprs/IsExpr.hpp>
#include <ast/exprs/MemberAccessCallExpr.hpp>
#include <ast/exprs/ParenExpr.hpp>
#include <ast/exprs/PostfixOperatorExpr.hpp>
#include <ast/exprs/PrefixOperatorExpr.hpp>
#include <ast/exprs/TernaryExpr.hpp>
#include <ast/exprs/VariableDeclExpr.hpp>
#include <ast/stmts/BreakStmt.hpp>
#include <ast/stmts/ContinueStmt.hpp>
#include <ast/stmts/GotoStmt.hpp>
#include <ast/exprs/ValueLiteralExpr.hpp>

namespace gulc {
    /**
     * Resolves any types found within `Stmt` and `Expr` instances
     * Resolves any calls to the Decls being called
     * Also assigns what type every `Expr` will result in
     */
    class ExprTypeResolver {
    public:
        explicit ExprTypeResolver(std::vector<std::string> const& filePaths)
                : _filePaths(filePaths), _currentFileID(0) {}

        void processFiles(std::vector<ASTFile>& files);

    private:
        std::vector<std::string> const& _filePaths;
        std::size_t _currentFileID;

        void printError(std::string const& message, TextPosition startPosition, TextPosition endPosition) const;
        void printWarning(std::string const& message, TextPosition startPosition, TextPosition endPosition) const;

        bool resolveType(Type*& type) const;

        void processDecl(Decl* decl, bool isGlobal) const;
        void processFunctionDecl(FunctionDecl* functionDecl) const;
        void processNamespaceDecl(NamespaceDecl* namespaceDecl) const;
        void processParameterDecl(ParameterDecl* parameterDecl) const;
        void processVariableDecl(VariableDecl* variableDecl, bool isGlobal) const;

        void processStmt(Stmt*& stmt) const;
//        void processBreakStmt(BreakStmt* breakStmt) const;
        void processCaseStmt(CaseStmt* caseStmt) const;
        void processCatchStmt(CatchStmt* catchStmt) const;
        void processCompoundStmt(CompoundStmt* compoundStmt) const;
//        void processContinueStmt(ContinueStmt* continueStmt) const;
        void processDoStmt(DoStmt* doStmt) const;
        void processForStmt(ForStmt* forStmt) const;
//        void processGotoStmt(GotoStmt* gotoStmt) const;
        void processIfStmt(IfStmt* ifStmt) const;
        void processLabeledStmt(LabeledStmt* labeledStmt) const;
        void processReturnStmt(ReturnStmt* returnStmt) const;
        void processSwitchStmt(SwitchStmt* switchStmt) const;
        void processTryStmt(TryStmt* tryStmt) const;
        void processWhileStmt(WhileStmt* whileStmt) const;

        void processExpr(Expr*& expr) const;
        void processAsExpr(AsExpr* asExpr) const;
        void processAssignmentOperatorExpr(AssignmentOperatorExpr* assignmentOperatorExpr) const;
        void processFunctionCallExpr(FunctionCallExpr* functionCallExpr) const;
        void processIdentifierExpr(IdentifierExpr* identifierExpr) const;
        void processIndexerCallExpr(IndexerCallExpr* indexerCallExpr) const;
        void processInfixOperatorExpr(InfixOperatorExpr* infixOperatorExpr) const;
        void processIsExpr(IsExpr* isExpr) const;
        void processMemberAccessCallExpr(MemberAccessCallExpr* memberAccessCallExpr) const;
        void processParenExpr(ParenExpr* parenExpr) const;
        void processPostfixOperatorExpr(PostfixOperatorExpr* postfixOperatorExpr) const;
        void processPrefixOperatorExpr(PrefixOperatorExpr* prefixOperatorExpr) const;
        void processTernaryExpr(TernaryExpr* ternaryExpr) const;
        void processValueLiteralExpr(ValueLiteralExpr* valueLiteralExpr) const;
        void processVariableDeclExpr(VariableDeclExpr* variableDeclExpr) const;

    };
}

#endif //GULC_EXPRTYPERESOLVER_HPP
