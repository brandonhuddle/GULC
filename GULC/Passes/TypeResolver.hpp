// Copyright (C) 2019 Michael Brandon Huddle
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef GULC_TYPERESOLVER_HPP
#define GULC_TYPERESOLVER_HPP

#include <AST/FileAST.hpp>
#include <AST/Decls/EnumDecl.hpp>
#include <AST/Decls/FunctionDecl.hpp>
#include <AST/Decls/GlobalVariableDecl.hpp>
#include <AST/Stmts/CaseStmt.hpp>
#include <AST/Stmts/DoStmt.hpp>
#include <AST/Stmts/ForStmt.hpp>
#include <AST/Stmts/IfStmt.hpp>
#include <AST/Stmts/LabeledStmt.hpp>
#include <AST/Stmts/ReturnStmt.hpp>
#include <AST/Stmts/SwitchStmt.hpp>
#include <AST/Stmts/TryStmt.hpp>
#include <AST/Stmts/WhileStmt.hpp>
#include <AST/Exprs/BinaryOperatorExpr.hpp>
#include <AST/Exprs/CharacterLiteralExpr.hpp>
#include <AST/Exprs/ExplicitCastExpr.hpp>
#include <AST/Exprs/FloatLiteralExpr.hpp>
#include <AST/Exprs/FunctionCallExpr.hpp>
#include <AST/Exprs/ImplicitCastExpr.hpp>
#include <AST/Exprs/IndexerCallExpr.hpp>
#include <AST/Exprs/IntegerLiteralExpr.hpp>
#include <AST/Exprs/LocalVariableDeclExpr.hpp>
#include <AST/Exprs/MemberAccessCallExpr.hpp>
#include <AST/Exprs/ParenExpr.hpp>
#include <AST/Exprs/PostfixOperatorExpr.hpp>
#include <AST/Exprs/PrefixOperatorExpr.hpp>
#include <AST/Exprs/ResolvedTypeRefExpr.hpp>
#include <AST/Exprs/StringLiteralExpr.hpp>
#include <AST/Exprs/TernaryExpr.hpp>

namespace gulc {
    /**
     * TypeResolver handles resolving the types ONLY on Decl objects.
     * Because we support overloading functions, operators, etc. we have to handle resolving types alone before
     * we resolve anything else.
     */
    class TypeResolver {
    public:
        void processFile(FileAST& fileAst);

        static Type* deepCopy(const Type* type);

    private:
        void printError(const std::string& message, TextPosition startPosition, TextPosition endPosition);
        void printWarning(const std::string& message, TextPosition startPosition, TextPosition endPosition);
        void printDebugWarning(const std::string& message);

        bool resolveType(Type*& type);

        void processDecl(Decl* decl);
        void processStmt(Stmt*& stmt);
        void processExpr(Expr*& expr);

        void processEnumDecl(EnumDecl* enumDecl);
        void processFunctionDecl(FunctionDecl* functionDecl);
        void processGlobalVariableDecl(GlobalVariableDecl* globalVariableDecl);

        void processCaseStmt(CaseStmt* caseStmt);
        void processCompoundStmt(CompoundStmt* compoundStmt);
        void processDoStmt(DoStmt* doStmt);
        void processForStmt(ForStmt* forStmt);
        void processIfStmt(IfStmt* ifStmt);
        void processLabeledStmt(LabeledStmt* labeledStmt);
        void processReturnStmt(ReturnStmt* returnStmt);
        void processSwitchStmt(SwitchStmt* switchStmt);
        void processTryStmt(TryStmt* tryStmt);
        void processTryCatchStmt(TryCatchStmt* tryCatchStmt);
        void processTryFinallyStmt(TryFinallyStmt* tryFinallyStmt);
        void processWhileStmt(WhileStmt* whileStmt);

        void processBinaryOperatorExpr(BinaryOperatorExpr* binaryOperatorExpr);
        void processCharacterLiteralExpr(CharacterLiteralExpr* characterLiteralExpr);
        void processExplicitCastExpr(ExplicitCastExpr* explicitCastExpr);
        void processFloatLiteralExpr(FloatLiteralExpr* floatLiteralExpr);
        void processFunctionCallExpr(FunctionCallExpr* functionCallExpr);
        void processIdentifierExpr(Expr*& expr);
        void processImplicitCastExpr(ImplicitCastExpr* implicitCastExpr);
        void processIndexerCallExpr(IndexerCallExpr* indexerCallExpr);
        void processIntegerLiteralExpr(IntegerLiteralExpr* integerLiteralExpr);
        void processLocalVariableDeclExpr(LocalVariableDeclExpr* localVariableDeclExpr);
        void processLocalVariableDeclOrPrefixOperatorCallExpr(Expr*& expr);
        void processMemberAccessCallExpr(MemberAccessCallExpr* memberAccessCallExpr);
        void processParenExpr(ParenExpr* parenExpr);
        void processPostfixOperatorExpr(PostfixOperatorExpr* postfixOperatorExpr);
        void processPotentialExplicitCastExpr(Expr*& expr);
        void processPrefixOperatorExpr(PrefixOperatorExpr* prefixOperatorExpr);
        void processResolvedTypeRefExpr(ResolvedTypeRefExpr* resolvedTypeRefExpr);
        void processStringLiteralExpr(StringLiteralExpr* stringLiteralExpr);
        void processTernaryExpr(TernaryExpr* ternaryExpr);
        void processUnresolvedTypeRefExpr(Expr*& expr);

        // Context management
        FileAST* currentFileAst;
        const std::vector<TemplateParameterDecl*>* functionTemplateParams;

    };
}

#endif //GULC_TYPERESOLVER_HPP