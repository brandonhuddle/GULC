#include <iostream>
#include <ast/types/BuiltInType.hpp>
#include "ExprTypeResolver.hpp"

void gulc::ExprTypeResolver::processFiles(std::vector<ASTFile>& files) {
    for (ASTFile& file : files) {
        _currentFileID = file.sourceFileID;

        for (Decl* decl : file.declarations) {
            processDecl(decl, true);
        }
    }
}

void gulc::ExprTypeResolver::printError(std::string const& message, gulc::TextPosition startPosition,
                                        gulc::TextPosition endPosition) const {
    std::cerr << "gulc resolver error[" << _filePaths[_currentFileID] << ", "
                                    "{" << startPosition.line << ", " << startPosition.column << " "
                                    "to " << endPosition.line << ", " << endPosition.column << "}]: "
              << message << std::endl;
}

void gulc::ExprTypeResolver::printWarning(std::string const& message, gulc::TextPosition startPosition,
                                          gulc::TextPosition endPosition) const {
    std::cerr << "gulc resolver warning[" << _filePaths[_currentFileID] << ", "
                                      "{" << startPosition.line << ", " << startPosition.column << " "
                                      "to " << endPosition.line << ", " << endPosition.column << "}]: "
              << message << std::endl;
}

bool gulc::ExprTypeResolver::resolveType(gulc::Type*& type) const {
    // TODO: Pass to `TypeResolver::resolveType`
    return false;
}

void gulc::ExprTypeResolver::processDecl(gulc::Decl* decl, bool isGlobal) const {
    switch (decl->getDeclKind()) {
        case Decl::Kind::Import:
            // We skip imports, they're no longer useful here...
            break;

        case Decl::Kind::Function:
            processFunctionDecl(llvm::dyn_cast<FunctionDecl>(decl));
            break;
        case Decl::Kind::Namespace:
            processNamespaceDecl(llvm::dyn_cast<NamespaceDecl>(decl));
            break;
        case Decl::Kind::Variable:
            processVariableDecl(llvm::dyn_cast<VariableDecl>(decl), isGlobal);
            break;

        default:
            printError("unknown declaration found!",
                       decl->startPosition(), decl->endPosition());
    }
}

void gulc::ExprTypeResolver::processFunctionDecl(gulc::FunctionDecl* functionDecl) const {
    for (ParameterDecl* parameter : functionDecl->parameters()) {
        processParameterDecl(parameter);
    }

#ifndef NDEBUG
    if (functionDecl->returnType == nullptr) {
        printError("[INTERNAL] `functionDecl->returnType` is null in `ExprTypeResolver`!",
                   functionDecl->startPosition(), functionDecl->endPosition());
    }
#endif

    processCompoundStmt(functionDecl->body());
}

void gulc::ExprTypeResolver::processNamespaceDecl(gulc::NamespaceDecl* namespaceDecl) const {
    for (Decl* decl : namespaceDecl->nestedDecls()) {
        processDecl(decl, true);
    }
}

void gulc::ExprTypeResolver::processParameterDecl(gulc::ParameterDecl* parameterDecl) const {
    if (parameterDecl->defaultValue != nullptr) {
        processExpr(parameterDecl->defaultValue);
    }
}

void gulc::ExprTypeResolver::processVariableDecl(gulc::VariableDecl* variableDecl, bool isGlobal) const {
#ifndef NDEBUG
    if (variableDecl->type == nullptr) {
        printError("[INTERNAL] `variableDecl->type` is null in `ExprTypeResolver`!",
                   variableDecl->startPosition(), variableDecl->endPosition());
    }
#endif

    if (variableDecl->hasInitialValue()) {
        processExpr(variableDecl->initialValue);
    }
}

// Stmts --------------------------------------------------------------------------------------------------------------
void gulc::ExprTypeResolver::processStmt(gulc::Stmt*& stmt) const {
    switch (stmt->getStmtKind()) {
        // The below statements do not require any processing, they cannot contain types
        case Stmt::Kind::Break:
        case Stmt::Kind::Continue:
        case Stmt::Kind::Goto:
            break;

        case Stmt::Kind::Case:
            processCaseStmt(llvm::dyn_cast<CaseStmt>(stmt));
            break;
        case Stmt::Kind::Catch:
            processCatchStmt(llvm::dyn_cast<CatchStmt>(stmt));
            break;
        case Stmt::Kind::Compound:
            processCompoundStmt(llvm::dyn_cast<CompoundStmt>(stmt));
            break;
        case Stmt::Kind::Do:
            processDoStmt(llvm::dyn_cast<DoStmt>(stmt));
            break;
        case Stmt::Kind::For:
            processForStmt(llvm::dyn_cast<ForStmt>(stmt));
            break;
        case Stmt::Kind::If:
            processIfStmt(llvm::dyn_cast<IfStmt>(stmt));
            break;
        case Stmt::Kind::Labeled:
            processLabeledStmt(llvm::dyn_cast<LabeledStmt>(stmt));
            break;
        case Stmt::Kind::Return:
            processReturnStmt(llvm::dyn_cast<ReturnStmt>(stmt));
            break;
        case Stmt::Kind::Switch:
            processSwitchStmt(llvm::dyn_cast<SwitchStmt>(stmt));
            break;
        case Stmt::Kind::Try:
            processTryStmt(llvm::dyn_cast<TryStmt>(stmt));
            break;
        case Stmt::Kind::While:
            processWhileStmt(llvm::dyn_cast<WhileStmt>(stmt));
            break;
        case Stmt::Kind::Expr: {
            auto expr = llvm::dyn_cast<Expr>(stmt);
            processExpr(expr);
            stmt = expr;
            break;
        }
        default:
            printError("unknown statement!",
                       stmt->startPosition(), stmt->endPosition());
    }
}

void gulc::ExprTypeResolver::processCaseStmt(gulc::CaseStmt* caseStmt) const {
    processExpr(caseStmt->condition);
    processStmt(caseStmt->trueStmt);
}

void gulc::ExprTypeResolver::processCatchStmt(gulc::CatchStmt* catchStmt) const {
    if (catchStmt->hasExceptionType()) {
        resolveType(catchStmt->exceptionType);
    }

    processCompoundStmt(catchStmt->body());
}

void gulc::ExprTypeResolver::processCompoundStmt(gulc::CompoundStmt* compoundStmt) const {
    for (Stmt*& statement : compoundStmt->statements) {
        processStmt(statement);
    }
}

void gulc::ExprTypeResolver::processDoStmt(gulc::DoStmt* doStmt) const {
    processExpr(doStmt->condition);
    processCompoundStmt(doStmt->body());
}

void gulc::ExprTypeResolver::processForStmt(gulc::ForStmt* forStmt) const {
    if (forStmt->init != nullptr) {
        processExpr(forStmt->init);
    }

    if (forStmt->condition != nullptr) {
        processExpr(forStmt->condition);
    }

    if (forStmt->iteration != nullptr) {
        processExpr(forStmt->iteration);
    }

    processCompoundStmt(forStmt->body());
}

void gulc::ExprTypeResolver::processIfStmt(gulc::IfStmt* ifStmt) const {
    processExpr(ifStmt->condition);
    processCompoundStmt(ifStmt->trueBody());

    if (ifStmt->hasFalseBody()) {
        Stmt* falseBody = ifStmt->falseBody();

        if (llvm::isa<IfStmt>(falseBody)) {
            processIfStmt(llvm::dyn_cast<IfStmt>(falseBody));
        } else if (llvm::isa<CompoundStmt>(falseBody)) {
            processCompoundStmt(llvm::dyn_cast<CompoundStmt>(falseBody));
        }
    }
}

void gulc::ExprTypeResolver::processLabeledStmt(gulc::LabeledStmt* labeledStmt) const {
    processStmt(labeledStmt->labeledStmt);
}

void gulc::ExprTypeResolver::processReturnStmt(gulc::ReturnStmt* returnStmt) const {
    if (returnStmt->returnValue != nullptr) {
        processExpr(returnStmt->returnValue);
    }
}

void gulc::ExprTypeResolver::processSwitchStmt(gulc::SwitchStmt* switchStmt) const {
    processExpr(switchStmt->condition);

    for (Stmt*& statement : switchStmt->statements) {
        processStmt(statement);
    }
}

void gulc::ExprTypeResolver::processTryStmt(gulc::TryStmt* tryStmt) const {
    processCompoundStmt(tryStmt->body());

    for (CatchStmt* catchStmt : tryStmt->catchStatements()) {
        processCatchStmt(catchStmt);
    }

    if (tryStmt->hasFinallyStatement()) {
        processCompoundStmt(tryStmt->finallyStatement());
    }
}

void gulc::ExprTypeResolver::processWhileStmt(gulc::WhileStmt* whileStmt) const {
    processExpr(whileStmt->condition);
    processCompoundStmt(whileStmt->body());
}

// Exprs --------------------------------------------------------------------------------------------------------------
void gulc::ExprTypeResolver::processExpr(Expr*& expr) const {
    switch (expr->getExprKind()) {
        // The below expressions do not require any processing, they cannot contain types
        case Expr::Kind::ValueLiteral:
            break;

        case Expr::Kind::As:
            processAsExpr(llvm::dyn_cast<AsExpr>(expr));
            break;
        case Expr::Kind::AssignmentOperator:
            processAssignmentOperatorExpr(llvm::dyn_cast<AssignmentOperatorExpr>(expr));
            break;
        case Expr::Kind::FunctionCall:
            processFunctionCallExpr(llvm::dyn_cast<FunctionCallExpr>(expr));
            break;
        case Expr::Kind::Identifier:
            processIdentifierExpr(llvm::dyn_cast<IdentifierExpr>(expr));
            break;
        case Expr::Kind::IndexerCall:
            processIndexerCallExpr(llvm::dyn_cast<IndexerCallExpr>(expr));
            break;
        case Expr::Kind::InfixOperator:
            processInfixOperatorExpr(llvm::dyn_cast<InfixOperatorExpr>(expr));
            break;
        case Expr::Kind::Is:
            processIsExpr(llvm::dyn_cast<IsExpr>(expr));
            break;
        case Expr::Kind::MemberAccessCall:
            processMemberAccessCallExpr(llvm::dyn_cast<MemberAccessCallExpr>(expr));
            break;
        case Expr::Kind::Paren:
            processParenExpr(llvm::dyn_cast<ParenExpr>(expr));
            break;
        case Expr::Kind::PostfixOperator:
            processPostfixOperatorExpr(llvm::dyn_cast<PostfixOperatorExpr>(expr));
            break;
        case Expr::Kind::PrefixOperator:
            processPrefixOperatorExpr(llvm::dyn_cast<PrefixOperatorExpr>(expr));
            break;
        case Expr::Kind::Ternary:
            processTernaryExpr(llvm::dyn_cast<TernaryExpr>(expr));
            break;
        case Expr::Kind::VariableDecl:
            processVariableDeclExpr(llvm::dyn_cast<VariableDeclExpr>(expr));
            break;
        default:
            printError("unknown expression!",
                       expr->startPosition(), expr->endPosition());
            break;
    }
}

void gulc::ExprTypeResolver::processAsExpr(gulc::AsExpr* asExpr) const {
    processExpr(asExpr->expr);

    resolveType(asExpr->asType);
}

void gulc::ExprTypeResolver::processAssignmentOperatorExpr(gulc::AssignmentOperatorExpr* assignmentOperatorExpr) const {
    processExpr(assignmentOperatorExpr->leftValue);
    processExpr(assignmentOperatorExpr->rightValue);
}

void gulc::ExprTypeResolver::processFunctionCallExpr(gulc::FunctionCallExpr* functionCallExpr) const {
    processExpr(functionCallExpr->functionReference);

    for (Expr*& argument : functionCallExpr->arguments) {
        processExpr(argument);
    }
}

void gulc::ExprTypeResolver::processIdentifierExpr(gulc::IdentifierExpr* identifierExpr) const {
    // TODO: Once this contains template arguments we need to process them here
}

void gulc::ExprTypeResolver::processIndexerCallExpr(gulc::IndexerCallExpr* indexerCallExpr) const {
    processExpr(indexerCallExpr->indexerReference);

    for (Expr*& argument : indexerCallExpr->arguments) {
        processExpr(argument);
    }
}

void gulc::ExprTypeResolver::processInfixOperatorExpr(gulc::InfixOperatorExpr* infixOperatorExpr) const {
    processExpr(infixOperatorExpr->leftValue);
    processExpr(infixOperatorExpr->rightValue);
}

void gulc::ExprTypeResolver::processIsExpr(gulc::IsExpr* isExpr) const {
    processExpr(isExpr->expr);

    resolveType(isExpr->isType);
}

void gulc::ExprTypeResolver::processMemberAccessCallExpr(gulc::MemberAccessCallExpr* memberAccessCallExpr) const {
    processExpr(memberAccessCallExpr->objectRef);
}

void gulc::ExprTypeResolver::processParenExpr(gulc::ParenExpr* parenExpr) const {
    processExpr(parenExpr->nestedExpr);
}

void gulc::ExprTypeResolver::processPostfixOperatorExpr(gulc::PostfixOperatorExpr* postfixOperatorExpr) const {
    processExpr(postfixOperatorExpr->nestedExpr);
}

void gulc::ExprTypeResolver::processPrefixOperatorExpr(gulc::PrefixOperatorExpr* prefixOperatorExpr) const {
    processExpr(prefixOperatorExpr->nestedExpr);
}

void gulc::ExprTypeResolver::processTernaryExpr(gulc::TernaryExpr* ternaryExpr) const {
    processExpr(ternaryExpr->condition);
    processExpr(ternaryExpr->trueExpr);
    processExpr(ternaryExpr->falseExpr);
}

void gulc::ExprTypeResolver::processValueLiteralExpr(gulc::ValueLiteralExpr* valueLiteralExpr) const {

}

void gulc::ExprTypeResolver::processVariableDeclExpr(gulc::VariableDeclExpr* variableDeclExpr) const {
    if (variableDeclExpr->type == nullptr) {
        // TODO: Should we set to `auto`?
    } else {
        resolveType(variableDeclExpr->type);
    }

    if (variableDeclExpr->initialValue != nullptr) {
        processExpr(variableDeclExpr->initialValue);
    }
}
