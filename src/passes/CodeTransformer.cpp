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
#include <iostream>
#include <ast/types/BuiltInType.hpp>
#include <ast/exprs/ParameterRefExpr.hpp>
#include "CodeTransformer.hpp"
#include <make_reverse_iterator.hpp>
#include <ast/exprs/CurrentSelfExpr.hpp>
#include <ast/types/ReferenceType.hpp>
#include <ast/exprs/TemporaryValueRefExpr.hpp>

void gulc::CodeTransformer::processFiles(std::vector<ASTFile>& files) {
    for (ASTFile& file : files) {
        _currentFile = &file;

        for (Decl* decl : file.declarations) {
            processDecl(decl);
        }
    }
}

void gulc::CodeTransformer::printError(std::string const& message, gulc::TextPosition startPosition,
                                       gulc::TextPosition endPosition) const {
    std::cerr << "gulc error[" << _filePaths[_currentFile->sourceFileID] << ", "
                            "{" << startPosition.line << ", " << startPosition.column << " "
                            "to " << endPosition.line << ", " << endPosition.column << "}]: "
              << message << std::endl;
    std::exit(1);
}

void gulc::CodeTransformer::printWarning(std::string const& message, gulc::TextPosition startPosition,
                                         gulc::TextPosition endPosition) const {
    std::cout << "gulc warning[" << _filePaths[_currentFile->sourceFileID] << ", "
                              "{" << startPosition.line << ", " << startPosition.column << " "
                              "to " << endPosition.line << ", " << endPosition.column << "}]: "
              << message << std::endl;
}

void gulc::CodeTransformer::processDecl(gulc::Decl* decl) {
    switch (decl->getDeclKind()) {
        case Decl::Kind::Enum:
            processEnumDecl(llvm::dyn_cast<EnumDecl>(decl));
            break;
        case Decl::Kind::Extension:
            processExtensionDecl(llvm::dyn_cast<ExtensionDecl>(decl));
            break;
        case Decl::Kind::CallOperator:
        case Decl::Kind::Constructor:
        case Decl::Kind::Destructor:
        case Decl::Kind::Operator:
        case Decl::Kind::Function:
            processFunctionDecl(llvm::dyn_cast<FunctionDecl>(decl));
            break;
        case Decl::Kind::Import:
            // We can safely ignore imports at this point
            break;
        case Decl::Kind::Namespace:
            processNamespaceDecl(llvm::dyn_cast<NamespaceDecl>(decl));
            break;
        case Decl::Kind::Parameter:
            processParameterDecl(llvm::dyn_cast<ParameterDecl>(decl));
            break;
        case Decl::Kind::Property:
            processPropertyDecl(llvm::dyn_cast<PropertyDecl>(decl));
            break;
        case Decl::Kind::Struct:
            processStructDecl(llvm::dyn_cast<StructDecl>(decl));
            break;
        case Decl::Kind::SubscriptOperator:
            processSubscriptOperatorDecl(llvm::dyn_cast<SubscriptOperatorDecl>(decl));
            break;
        case Decl::Kind::TemplateFunction:
            processTemplateFunctionDecl(llvm::dyn_cast<TemplateFunctionDecl>(decl));
            break;
        case Decl::Kind::TemplateStruct:
            processTemplateStructDecl(llvm::dyn_cast<TemplateStructDecl>(decl));
            break;
        case Decl::Kind::TemplateTrait:
            processTemplateTraitDecl(llvm::dyn_cast<TemplateTraitDecl>(decl));
            break;
        case Decl::Kind::Trait:
            processTraitDecl(llvm::dyn_cast<TraitDecl>(decl));
            break;
        case Decl::Kind::TypeAlias:
            // We can safely ignore type aliases at this point
            break;
        case Decl::Kind::TypeSuffix:
            // We can safely ignore type suffixes at this point
            break;
        case Decl::Kind::Variable:
            processVariableDecl(llvm::dyn_cast<VariableDecl>(decl));
            break;
        default:
            printError("[INTERNAL ERROR] unexpected `Decl` found in `CodeTransformer::processDecl`!",
                       decl->startPosition(), decl->endPosition());
            break;
    }
}

void gulc::CodeTransformer::processEnumDecl(gulc::EnumDecl* enumDecl) {
    for (Decl* member : enumDecl->ownedMembers()) {
        processDecl(member);
    }
}

void gulc::CodeTransformer::processExtensionDecl(gulc::ExtensionDecl* extensionDecl) {
    for (ConstructorDecl* constructor : extensionDecl->constructors()) {
        processDecl(constructor);
    }

    for (Decl* ownedMember : extensionDecl->ownedMembers()) {
        processDecl(ownedMember);
    }
}

void gulc::CodeTransformer::processFunctionDecl(gulc::FunctionDecl* functionDecl) {
    FunctionDecl* oldFunction = _currentFunction;
    _currentFunction = functionDecl;
    std::vector<ParameterDecl*>* oldParameters = _currentParameters;
    _currentParameters = &functionDecl->parameters();

    for (ParameterDecl* parameter : functionDecl->parameters()) {
        processParameterDecl(parameter);
    }

    _validateGotoVariables.clear();

    // Prototypes don't have bodies
    if (!functionDecl->isPrototype()) {
        bool returnsOnAllCodePaths = processCompoundStmtHandleTempValues(functionDecl->body());

        if (!returnsOnAllCodePaths) {
            // If the function doesn't return on all code paths but the function returns `void` we add in a default
            // `return` to the end.
            // If the function returns non-void we error out saying the function doesn't return on all code paths, this
            // is to prevent potential errors the programmer might miss.
            if (functionDecl->returnType == nullptr ||
                    (llvm::isa<BuiltInType>(functionDecl->returnType) &&
                     llvm::dyn_cast<BuiltInType>(functionDecl->returnType)->sizeInBytes() == 0)) {
                auto defaultReturn = new ReturnStmt({}, {});
                // We pass it in to `processReturnStmt` so that all values are properly destructed.
                // I don't believe we need to tell it not to destruct local variables as `processCompoundStmt` should
                // clean up any local variables already. `processReturnStmt` shouldn't be able to seel the old locals
                processReturnStmt(defaultReturn);
                functionDecl->body()->statements.push_back(defaultReturn);
            } else {
                printError("function `" + functionDecl->identifier().name() + "` does not return on all code paths!",
                           functionDecl->startPosition(), functionDecl->endPosition());
            }
        }
    }

    _currentParameters = oldParameters;
    _currentFunction = oldFunction;
}

void gulc::CodeTransformer::processNamespaceDecl(gulc::NamespaceDecl* namespaceDecl) {
    for (Decl* nestedDecl : namespaceDecl->nestedDecls()) {
        processDecl(nestedDecl);
    }
}

void gulc::CodeTransformer::processParameterDecl(gulc::ParameterDecl* parameterDecl) {
    // TODO: I believe parameters are done by this point. We shouldn't have anything to do here, right?
}

void gulc::CodeTransformer::processPropertyDecl(gulc::PropertyDecl* propertyDecl) {
    for (PropertyGetDecl* getter : propertyDecl->getters()) {
        processPropertyGetDecl(getter);
    }

    if (propertyDecl->hasSetter()) {
        processPropertySetDecl(propertyDecl->setter());
    }
}

void gulc::CodeTransformer::processPropertyGetDecl(gulc::PropertyGetDecl* propertyGetDecl) {
    processFunctionDecl(propertyGetDecl);
}

void gulc::CodeTransformer::processPropertySetDecl(gulc::PropertySetDecl* propertySetDecl) {
    processFunctionDecl(propertySetDecl);
}

void gulc::CodeTransformer::processStructDecl(gulc::StructDecl* structDecl) {
    for (ConstructorDecl* constructor : structDecl->constructors()) {
        processFunctionDecl(constructor);
    }

    if (structDecl->destructor != nullptr) {
        processFunctionDecl(structDecl->destructor);
    }

    for (Decl* member : structDecl->ownedMembers()) {
        processDecl(member);
    }
}

void gulc::CodeTransformer::processSubscriptOperatorDecl(gulc::SubscriptOperatorDecl* subscriptOperatorDecl) {
    // TODO: We're going to have to change how we do this a little. `SubscriptOperatorDecl` declares its own parameters
    //       that will need to be accessible from the `get`s and `set`. `set` also defines its own parameter that will
    //       need to be accounted for.
    for (SubscriptOperatorGetDecl* getter : subscriptOperatorDecl->getters()) {
        processSubscriptOperatorGetDecl(getter);
    }

    if (subscriptOperatorDecl->hasSetter()) {
        processSubscriptOperatorSetDecl(subscriptOperatorDecl->setter());
    }
}

void gulc::CodeTransformer::processSubscriptOperatorGetDecl(gulc::SubscriptOperatorGetDecl* subscriptOperatorGetDecl) {
    processFunctionDecl(subscriptOperatorGetDecl);
}

void gulc::CodeTransformer::processSubscriptOperatorSetDecl(gulc::SubscriptOperatorSetDecl* subscriptOperatorSetDecl) {
    processFunctionDecl(subscriptOperatorSetDecl);
}

void gulc::CodeTransformer::processTemplateFunctionDecl(gulc::TemplateFunctionDecl* templateFunctionDecl) {
    for (auto templateFunctionInstDecl : templateFunctionDecl->templateInstantiations()) {
        processFunctionDecl(templateFunctionInstDecl);
    }
}

void gulc::CodeTransformer::processTemplateStructDecl(gulc::TemplateStructDecl* templateStructDecl) {
    for (auto templateStructInstDecl : templateStructDecl->templateInstantiations()) {
        processStructDecl(templateStructInstDecl);
    }
}

void gulc::CodeTransformer::processTemplateTraitDecl(gulc::TemplateTraitDecl* templateTraitDecl) {
    for (auto templateTraitInstDecl : templateTraitDecl->templateInstantiations()) {
        processTraitDecl(templateTraitInstDecl);
    }
}

void gulc::CodeTransformer::processTraitDecl(gulc::TraitDecl* traitDecl) {
    // TODO:
}

void gulc::CodeTransformer::processVariableDecl(gulc::VariableDecl* variableDecl) {
    if (variableDecl->hasInitialValue()) {
        processExpr(variableDecl->initialValue);
    }
}

bool gulc::CodeTransformer::processStmt(gulc::Stmt*& stmt) {
    std::size_t oldTemporaryValuesSize = _temporaryValues.size();
    _temporaryValues.emplace_back(std::vector<VariableDeclExpr*>());

    bool returnsOnAllCodePaths = false;

    switch (stmt->getStmtKind()) {
        case Stmt::Kind::Break:
            returnsOnAllCodePaths = processBreakStmt(llvm::dyn_cast<BreakStmt>(stmt));
            break;
        case Stmt::Kind::Case:
            returnsOnAllCodePaths = processCaseStmt(llvm::dyn_cast<CaseStmt>(stmt));
            break;
        case Stmt::Kind::Catch:
            // TODO: I don't think this should be included in the general area either...
            returnsOnAllCodePaths = processCatchStmt(llvm::dyn_cast<CatchStmt>(stmt));
            break;
        case Stmt::Kind::Compound:
            returnsOnAllCodePaths = processCompoundStmt(llvm::dyn_cast<CompoundStmt>(stmt));
            break;
        case Stmt::Kind::Continue:
            returnsOnAllCodePaths = processContinueStmt(llvm::dyn_cast<ContinueStmt>(stmt));
            break;
        case Stmt::Kind::DoCatch:
            returnsOnAllCodePaths = processDoCatchStmt(llvm::dyn_cast<DoCatchStmt>(stmt));
            break;
        case Stmt::Kind::DoWhile:
            returnsOnAllCodePaths = processDoWhileStmt(llvm::dyn_cast<DoWhileStmt>(stmt));
            break;
        case Stmt::Kind::For:
            returnsOnAllCodePaths = processForStmt(llvm::dyn_cast<ForStmt>(stmt));
            break;
        case Stmt::Kind::Goto:
            returnsOnAllCodePaths = processGotoStmt(llvm::dyn_cast<GotoStmt>(stmt));
            break;
        case Stmt::Kind::If:
            returnsOnAllCodePaths = processIfStmt(llvm::dyn_cast<IfStmt>(stmt));
            break;
        case Stmt::Kind::Labeled:
            returnsOnAllCodePaths = processLabeledStmt(llvm::dyn_cast<LabeledStmt>(stmt));
            break;
        case Stmt::Kind::Return:
            returnsOnAllCodePaths = processReturnStmt(llvm::dyn_cast<ReturnStmt>(stmt));
            break;
        case Stmt::Kind::Switch:
            returnsOnAllCodePaths = processSwitchStmt(llvm::dyn_cast<SwitchStmt>(stmt));
            break;
        case Stmt::Kind::While:
            returnsOnAllCodePaths = processWhileStmt(llvm::dyn_cast<WhileStmt>(stmt));
            break;

        case Stmt::Kind::Expr: {
            auto expr = llvm::dyn_cast<Expr>(stmt);
            processExpr(expr);
            stmt = expr;
            break;
        }

        default:
            printError("[INTERNAL ERROR] unsupported `Stmt` found in `CodeTransformer::processStmt`!",
                       stmt->startPosition(), stmt->endPosition());
            break;
    }

    // Add the last index to `Stmt::temporaryValues` so we don't lose the pointers and leak memory.
    auto checkLastTemporaryValueList = _temporaryValues.end() - 1;

    if (!checkLastTemporaryValueList->empty()) {
        stmt->temporaryValues = *checkLastTemporaryValueList;
    }

    _temporaryValues.resize(oldTemporaryValuesSize);

    return returnsOnAllCodePaths;
}

bool gulc::CodeTransformer::processBreakStmt(gulc::BreakStmt* breakStmt) {
    if (!breakStmt->hasBreakLabel()) {
        destructLocalVariablesDeclaredAfterLoop(_currentLoop, breakStmt->preBreakDeferred);
    } else {
        LabeledStmt* breakLabel = _currentFunction->labeledStmts[breakStmt->breakLabel().name()];
        // If the labeled statement isn't a loop that will be handled in the `CodeVerifier` since that isn't allowed.
        destructLocalVariablesDeclaredAfterLoop(breakLabel->labeledStmt, breakStmt->preBreakDeferred);
    }

    // NOTE: This is an iffy scenario. We need a way to say "all code after this is unreachable" but not that there is
    // a return
    return false;
}

bool gulc::CodeTransformer::processCaseStmt(gulc::CaseStmt* caseStmt) {
    if (!caseStmt->isDefault()) {
        processExpr(caseStmt->condition);
    }

    bool returnsOnAllCodePaths = false;
    std::size_t oldLocalVariableCount = _localVariables.size();

    for (Stmt* stmt : caseStmt->body) {
        if (processStmt(stmt)) {
            returnsOnAllCodePaths = true;
        }
    }

    // TODO: Is this correct for case statements?
    if (!returnsOnAllCodePaths) {
        for (std::int64_t i = static_cast<std::int64_t>(_localVariables.size()) - 1;
             i >= static_cast<std::int64_t>(oldLocalVariableCount); --i) {
            auto destructLocalVariableExpr = destructLocalVariable(_localVariables[i]);

            if (destructLocalVariableExpr != nullptr) {
                caseStmt->body.push_back(destructLocalVariableExpr);
            }
        }
    }

    _localVariables.resize(oldLocalVariableCount);

    for (GotoStmt* gotoStmt : _validateGotoVariables) {
        // Only lower the number of local variables, never raise it
        // This is to make it so we find the common parent between the `goto` and the labeled statement
        if (_localVariables.size() < gotoStmt->currentNumLocalVariables) {
            gotoStmt->currentNumLocalVariables = _localVariables.size();
        }
    }

    return returnsOnAllCodePaths;
}

bool gulc::CodeTransformer::processCaseStmtHandleTempValues(gulc::CaseStmt* caseStmt) {
    Stmt* stmt = caseStmt;
    return processStmt(stmt);
}

bool gulc::CodeTransformer::processCatchStmt(gulc::CatchStmt* catchStmt) {
    return processCompoundStmtHandleTempValues(catchStmt->body());
}

bool gulc::CodeTransformer::processCompoundStmt(gulc::CompoundStmt* compoundStmt) {
    // For compound statements since they are an array of statements that run one after the other we only have to detect
    // if a single statement returns on its code path. If so then we return on all code paths.
    bool returnsOnAllCodePaths = false;
    std::size_t oldLocalVariableCount = _localVariables.size();

    for (Stmt* stmt : compoundStmt->statements) {
        if (processStmt(stmt)) {
            returnsOnAllCodePaths = true;
        }
    }

    if (!returnsOnAllCodePaths) {
        for (std::int64_t i = static_cast<std::int64_t>(_localVariables.size()) - 1;
                i >= static_cast<std::int64_t>(oldLocalVariableCount); --i) {
            auto destructLocalVariableExpr = destructLocalVariable(_localVariables[i]);

            if (destructLocalVariableExpr != nullptr) {
                compoundStmt->statements.push_back(destructLocalVariableExpr);
            }
        }
    }

    _localVariables.resize(oldLocalVariableCount);

    for (GotoStmt* gotoStmt : _validateGotoVariables) {
        // Only lower the number of local variables, never raise it
        // This is to make it so we find the common parent between the `goto` and the labeled statement
        if (_localVariables.size() < gotoStmt->currentNumLocalVariables) {
            gotoStmt->currentNumLocalVariables = _localVariables.size();
        }
    }

    return returnsOnAllCodePaths;
}

bool gulc::CodeTransformer::processCompoundStmtHandleTempValues(gulc::CompoundStmt* compoundStmt) {
    Stmt* stmt = compoundStmt;
    return processStmt(stmt);
}

bool gulc::CodeTransformer::processContinueStmt(gulc::ContinueStmt* continueStmt) {
    if (continueStmt->hasContinueLabel()) {
        destructLocalVariablesDeclaredAfterLoop(_currentLoop, continueStmt->preContinueDeferred);
    } else {
        LabeledStmt* continueLabel = _currentFunction->labeledStmts[continueStmt->continueLabel().name()];
        // If the labeled statement isn't a loop that will be handled in the `CodeVerifier` since that isn't allowed.
        destructLocalVariablesDeclaredAfterLoop(continueLabel->labeledStmt, continueStmt->preContinueDeferred);
    }

    // NOTE: This is an iffy scenario. We need a way to say "all code after this is unreachable" but not that there is
    // a return
    return false;
}

bool gulc::CodeTransformer::processDoCatchStmt(gulc::DoCatchStmt* doCatchStmt) {
    // For `do { ... } catch { ... } finally { ... }` ALL blocks must return for the `DoCatchStmt` to return on all code
    // paths. If a block is missing then we can ignore it.
    // TODO: Can we ignore it?? What if we return in `finally`, doesn't that mean we return in all code paths? Since
    //       `finally` ALWAYS runs?
    bool returnsOnAllCodePaths = true;

    if (!processCompoundStmtHandleTempValues(doCatchStmt->body())) {
        returnsOnAllCodePaths = false;
    }

    for (CatchStmt* catchStmt : doCatchStmt->catchStatements()) {
        if (!processCatchStmt(catchStmt)) {
            returnsOnAllCodePaths = false;
        }
    }

    if (doCatchStmt->hasFinallyStatement()) {
        // TODO: Is this correct? If there is a `finally` block then we can detect `returnsOnAllCodePaths` based off of
        //       it since it will always run?
        returnsOnAllCodePaths = true;

        if (!processCompoundStmtHandleTempValues(doCatchStmt->finallyStatement())) {
            returnsOnAllCodePaths = false;
        }
    }

    return returnsOnAllCodePaths;
}

bool gulc::CodeTransformer::processDoWhileStmt(gulc::DoWhileStmt* doWhileStmt) {
    Stmt* oldLoop = _currentLoop;
    _currentLoop = doWhileStmt;

    doWhileStmt->currentNumLocalVariables = _localVariables.size();
    processCompoundStmtHandleTempValues(doWhileStmt->body());
    processExpr(doWhileStmt->condition);

    _currentLoop = oldLoop;

    // TODO: Is this true? My logic is that the `condition` could be false therefore we don't know if it returns on all
    //       code paths.
    return false;
}

bool gulc::CodeTransformer::processForStmt(gulc::ForStmt* forStmt) {
    Stmt* oldLoop = _currentLoop;
    _currentLoop = forStmt;
    std::size_t oldLocalVariableCount = _localVariables.size();

    if (forStmt->init != nullptr) {
        processExpr(forStmt->init);
    }

    if (forStmt->condition != nullptr) {
        // `condition` runs ever loop and has the potential to create temporary values every loop. We need to make sure
        // that in that (very inefficient) case we're properly calling the destructor on those temporary values.
        std::size_t oldTemporaryValuesSize = _temporaryValues.size();
        _temporaryValues.emplace_back(std::vector<VariableDeclExpr*>());

        processExpr(forStmt->condition);

        // Add the last index to `Stmt::temporaryValues` so we don't lose the pointers and leak memory.
        auto checkLastTemporaryValueList = _temporaryValues.end() - 1;

        if (!checkLastTemporaryValueList->empty()) {
            forStmt->condition->temporaryValues = *checkLastTemporaryValueList;
        }

        _temporaryValues.resize(oldTemporaryValuesSize);
    }

    if (forStmt->iteration != nullptr) {
        // Ditto to above about `condition`, through this happens at the end of `body` while `condition` happens at the
        // beginning.
        std::size_t oldTemporaryValuesSize = _temporaryValues.size();
        _temporaryValues.emplace_back(std::vector<VariableDeclExpr*>());

        processExpr(forStmt->iteration);

        // Add the last index to `Stmt::temporaryValues` so we don't lose the pointers and leak memory.
        auto checkLastTemporaryValueList = _temporaryValues.end() - 1;

        if (!checkLastTemporaryValueList->empty()) {
            forStmt->condition->temporaryValues = *checkLastTemporaryValueList;
        }

        _temporaryValues.resize(oldTemporaryValuesSize);
    }

    forStmt->currentNumLocalVariables = _localVariables.size();

    processCompoundStmtHandleTempValues(forStmt->body());

    // Delete the old local variables
    for (std::int64_t i = static_cast<std::int64_t>(_localVariables.size()) - 1;
            i >= static_cast<std::int64_t>(oldLocalVariableCount); --i) {
        auto destructLocalVariableExpr = destructLocalVariable(_localVariables[i]);

        if (destructLocalVariableExpr != nullptr) {
            forStmt->postLoopCleanup.push_back(destructLocalVariableExpr);
        }
    }

    _localVariables.resize(oldLocalVariableCount);
    _currentLoop = oldLoop;

    // We also perform this operation after `for` loops to remove the number of variables declared in the preloop
    for (GotoStmt* gotoStmt : _validateGotoVariables) {
        // Only lower the number of local variables, never raise it
        // This is to make it so we find the common parent between the `goto` and the labeled statement
        if (_localVariables.size() < gotoStmt->currentNumLocalVariables) {
            gotoStmt->currentNumLocalVariables = _localVariables.size();
        }
    }

    // TODO: Is this true? My logic is that the `condition` could be false therefore we don't know if it returns on all
    //       code paths.
    return false;
}

bool gulc::CodeTransformer::processGotoStmt(gulc::GotoStmt* gotoStmt) {
    LabeledStmt* gotoLabel = _currentFunction->labeledStmts[gotoStmt->label().name()];

    for (std::int64_t i = static_cast<int64_t>(_localVariables.size()) - 1;
            i >= static_cast<int64_t>(gotoLabel->currentNumLocalVariables); --i) {
        // Add the destructor call to the end of the compound statement...
        auto destructLocalVariableExpr = destructLocalVariable(_localVariables[i]);

        if (destructLocalVariableExpr != nullptr) {
            gotoStmt->preGotoDeferred.push_back(destructLocalVariableExpr);
        }
    }

    gotoStmt->currentNumLocalVariables = _localVariables.size();
    _validateGotoVariables.push_back(gotoStmt);

    return false;
}

bool gulc::CodeTransformer::processIfStmt(gulc::IfStmt* ifStmt) {
    processExpr(ifStmt->condition);
    bool trueBodyReturnsOnAllCodePaths = processCompoundStmtHandleTempValues(ifStmt->trueBody());
    bool falseBodyReturnsOnAllCodePaths = false;

    if (ifStmt->hasFalseBody()) {
        // NOTE: `falseBody` can only be `IfStmt` or `CompoundStmt`
        if (llvm::isa<IfStmt>(ifStmt->falseBody())) {
            falseBodyReturnsOnAllCodePaths =
                    processIfStmtHandleTempValues(llvm::dyn_cast<IfStmt>(ifStmt->falseBody()));
        } else if (llvm::isa<CompoundStmt>(ifStmt->falseBody())) {
            falseBodyReturnsOnAllCodePaths =
                    processCompoundStmtHandleTempValues(llvm::dyn_cast<CompoundStmt>(ifStmt->falseBody()));
        }
    }

    // NOTE: Both sides must return on all code paths for us to return on all code paths.
    //       If there is an `if` statement without an `else` then the `if` CANNOT return on all code paths. The `if`
    //       might be `false`
    return trueBodyReturnsOnAllCodePaths && falseBodyReturnsOnAllCodePaths;
}

bool gulc::CodeTransformer::processIfStmtHandleTempValues(gulc::IfStmt* ifStmt) {
    Stmt* stmt = ifStmt;
    return processStmt(stmt);
}

bool gulc::CodeTransformer::processLabeledStmt(gulc::LabeledStmt* labeledStmt) {
    // Validate any `goto` statements that reference this labeled statement.
    for (GotoStmt* gotoStmt : _validateGotoVariables) {
        if (gotoStmt->label().name() == labeledStmt->label().name()) {
            if (_localVariables.size() > gotoStmt->currentNumLocalVariables) {
                printError("cannot jump from this goto to the referenced label, jump skips variable declarations!",
                           gotoStmt->startPosition(), gotoStmt->endPosition());
                return false;
            }
        }
    }

    return processStmt(labeledStmt->labeledStmt);
}

bool gulc::CodeTransformer::processReturnStmt(gulc::ReturnStmt* returnStmt) {
    if (returnStmt->returnValue != nullptr) {
        processExpr(returnStmt->returnValue);
    }

    // Add the destructor calls to the pre return expression list
    for (VariableDeclExpr* localVariable : gulc::reverse(_localVariables)) {
        auto destructLocalVariableExpr = destructLocalVariable(localVariable);

        if (destructLocalVariableExpr != nullptr) {
            returnStmt->preReturnDeferred.push_back(destructLocalVariableExpr);
        }
    }

    // We only destruct parameters on return statements
    if (_currentParameters != nullptr) {
        for (std::int64_t i = static_cast<int64_t>(_currentParameters->size()) - 1; i >= 0; --i) {
            auto destructParameterExpr = destructParameter(i, (*_currentParameters)[i]);

            if (destructParameterExpr != nullptr) {
                returnStmt->preReturnDeferred.push_back(destructParameterExpr);
            }
        }
    }

    // If the current function is a destructor we have to clean up the members automatically
    if (llvm::isa<DestructorDecl>(_currentFunction)) {
        auto currentStruct = llvm::dyn_cast<StructDecl>(_currentFunction->container);

        // TODO: We also need to support properties...
        for (Decl* checkDecl : gulc::reverse(currentStruct->ownedMembers())) {
            if (llvm::isa<VariableDecl>(checkDecl)) {
                auto destructMember = llvm::dyn_cast<VariableDecl>(checkDecl);

                if (llvm::isa<StructType>(destructMember->type)) {
                    auto structType = llvm::dyn_cast<StructType>(destructMember->type);
                    auto foundDestructor = structType->decl()->destructor;

                    if (foundDestructor == nullptr) {
                        printError("[INTERNAL] struct `" + structType->decl()->identifier().name() + "` "
                                   "is missing a destructor!",
                                   destructMember->startPosition(), destructMember->endPosition());
                    }

                    auto destructorReferenceExpr = new DestructorReferenceExpr(
                            foundDestructor->startPosition(),
                            foundDestructor->endPosition(),
                            foundDestructor
                    );
                    auto selfRef = new CurrentSelfExpr(destructMember->startPosition(), destructMember->endPosition());
                    selfRef->valueType = new ReferenceType(
                            Type::Qualifier::Unassigned,
                            new StructType(
                                    Type::Qualifier::Mut,
                                    structType->decl(),
                                    destructMember->startPosition(),
                                    destructMember->endPosition()
                                )
                            );
                    selfRef->valueType->setIsLValue(true);
                    auto memberVariableRefExpr = new MemberVariableRefExpr(
                            destructMember->startPosition(),
                            destructMember->endPosition(),
                            selfRef,
                            llvm::dyn_cast<StructType>(structType->deepCopy()),
                            destructMember
                        );
                    memberVariableRefExpr->valueType = destructMember->type->deepCopy();
                    // A local variable reference is an lvalue.
                    memberVariableRefExpr->valueType->setIsLValue(true);

                    // TODO: We need to account for extern
//        if (foundDestructor != nullptr) {
//            currentFileAst->addImportExtern(foundDestructor);
//        }

                    returnStmt->preReturnDeferred.push_back(
                            new DestructorCallExpr(destructorReferenceExpr, memberVariableRefExpr, {}, {}));
                }
            }
        }
    }

    return true;
}

bool gulc::CodeTransformer::processSwitchStmt(gulc::SwitchStmt* switchStmt) {
    // NOTE: If there is a single case in the switch then we start with the idea that it COULD return on all code paths
    //       We then loop to check if it does. If a single case doesn't return then we revert to `false`
    //       Obviously if there are no cases then the switch doesn't return.
    bool returnsOnAllCodePaths = !switchStmt->cases.empty();

    processExpr(switchStmt->condition);

    for (CaseStmt* caseStmt : switchStmt->cases) {
        if (!processCaseStmtHandleTempValues(caseStmt)) {
            returnsOnAllCodePaths = false;
        }
    }

    return returnsOnAllCodePaths;
}

bool gulc::CodeTransformer::processWhileStmt(gulc::WhileStmt* whileStmt) {
    Stmt* oldLoop = _currentLoop;
    _currentLoop = whileStmt;

    whileStmt->currentNumLocalVariables = _localVariables.size();
    processExpr(whileStmt->condition);
    processCompoundStmtHandleTempValues(whileStmt->body());

    _currentLoop = oldLoop;

    // TODO: Is this true? My logic is that the `condition` could be false therefore we don't know if it returns on all
    //       code paths.
    return false;
}

void gulc::CodeTransformer::destructLocalVariablesDeclaredAfterLoop(gulc::Stmt* loop, std::vector<Expr*>& addToList) {
    unsigned int numLocalVariables = 0;

    if (llvm::isa<ForStmt>(loop)) {
        numLocalVariables = llvm::dyn_cast<ForStmt>(loop)->currentNumLocalVariables;
    } else if (llvm::isa<DoWhileStmt>(loop)) {
        numLocalVariables = llvm::dyn_cast<DoWhileStmt>(loop)->currentNumLocalVariables;
    } else if (llvm::isa<WhileStmt>(loop)) {
        numLocalVariables = llvm::dyn_cast<WhileStmt>(loop)->currentNumLocalVariables;
    }

    // If we've created new local variables then we have to destruct them
    for (std::int64_t i = static_cast<int64_t>(_localVariables.size()) - 1;
            i >= static_cast<int64_t>(numLocalVariables); --i) {
        // Add the destructor call to the end of the compound statement...
        auto destructLocalVariableExpr = destructLocalVariable(_localVariables[i]);

        if (destructLocalVariableExpr != nullptr) {
            addToList.push_back(destructLocalVariableExpr);
        }
    }
}

gulc::DestructorCallExpr* gulc::CodeTransformer::destructLocalVariable(gulc::VariableDeclExpr* localVariable) {
    if (llvm::isa<StructType>(localVariable->type)) {
        auto structType = llvm::dyn_cast<StructType>(localVariable->type);
        auto foundDestructor = structType->decl()->destructor;

        if (foundDestructor == nullptr) {
            printError("[INTERNAL] struct `" + structType->decl()->identifier().name() + "` is missing a destructor!",
                       localVariable->startPosition(), localVariable->endPosition());
        }

        auto destructorReferenceExpr = new DestructorReferenceExpr(
                foundDestructor->startPosition(),
                foundDestructor->endPosition(),
                foundDestructor
            );
        auto localVariableRefExpr = new LocalVariableRefExpr(
                {}, {},
                localVariable->identifier().name()
            );
        localVariableRefExpr->valueType = localVariable->type->deepCopy();
        // A local variable reference is an lvalue.
        localVariableRefExpr->valueType->setIsLValue(true);

        // TODO: We need to account for extern
//        if (foundDestructor != nullptr) {
//            currentFileAst->addImportExtern(foundDestructor);
//        }

        return new DestructorCallExpr(destructorReferenceExpr, localVariableRefExpr, {}, {});
    } else {
        return nullptr;
    }
}

gulc::DestructorCallExpr* gulc::CodeTransformer::destructParameter(std::size_t paramIndex,
                                                                   gulc::ParameterDecl* parameterDecl) {
    if (llvm::isa<StructType>(parameterDecl->type)) {
        auto structType = llvm::dyn_cast<StructType>(parameterDecl->type);
        auto foundDestructor = structType->decl()->destructor;

        if (foundDestructor == nullptr) {
            printError("[INTERNAL] struct `" + structType->decl()->identifier().name() + "` is missing a destructor!",
                       parameterDecl->startPosition(), parameterDecl->endPosition());
        }

        auto destructorReferenceExpr = new DestructorReferenceExpr(
                foundDestructor->startPosition(),
                foundDestructor->endPosition(),
                foundDestructor
        );
        auto parameterRefExpr = new ParameterRefExpr(
                {}, {},
                paramIndex, parameterDecl->identifier().name()
        );
        parameterRefExpr->valueType = parameterDecl->type->deepCopy();
        // A local variable reference is an lvalue.
        parameterRefExpr->valueType->setIsLValue(true);

        // TODO: We need to account for extern
//        if (foundDestructor != nullptr) {
//            currentFileAst->addImportExtern(foundDestructor);
//        }

        return new DestructorCallExpr(destructorReferenceExpr, parameterRefExpr, {}, {});
    } else {
        return nullptr;
    }
}

void gulc::CodeTransformer::processExpr(gulc::Expr*& expr) {
    switch (expr->getExprKind()) {
        case Expr::Kind::ArrayLiteral:
            processArrayLiteralExpr(llvm::dyn_cast<ArrayLiteralExpr>(expr));
            break;
        case Expr::Kind::As:
            processAsExpr(llvm::dyn_cast<AsExpr>(expr));
            break;
        case Expr::Kind::AssignmentOperator:
            processAssignmentOperatorExpr(llvm::dyn_cast<AssignmentOperatorExpr>(expr));
            break;
        case Expr::Kind::BoolLiteral:
            // Bool literal can't contain anything and cannot have temporaries. There's nothing we need to do here
            break;
        case Expr::Kind::CallOperatorReference:
            // Call operator references don't contain anything but a reference to a `CallOperatorDecl`, nothing to do
            break;
        case Expr::Kind::CheckExtendsType:
            // Check extends requires a type on both sides, nothing to do
            break;
        case Expr::Kind::ConstructorCall:
            processConstructorCallExpr(llvm::dyn_cast<ConstructorCallExpr>(expr));
            break;
        case Expr::Kind::ConstructorReference:
            // Constructor reference just holds a reference to a `ConstructorDecl`, nothing to do
            break;
        case Expr::Kind::CurrentSelf:
            // Current self cannot have any temporary values and doesn't need destructed, nothing to do
            break;
        case Expr::Kind::EnumConstRef:
            // Enum const ref only references a decl
            // TODO: Do we need to handle temporary values? I think we only need that if we copy...
            break;
        case Expr::Kind::FunctionCall:
            processFunctionCallExpr(expr);
            break;
        case Expr::Kind::FunctionReference:
            // Function reference just holds a reference to a `FunctionDecl`, nothing to do
            break;
        case Expr::Kind::Has:
            // Has takes a type on the left side and a signature on the right, nothing to do
            break;
        case Expr::Kind::Identifier:
            printError("[INTERNAL] `IdentifierExpr` found in `CodeTransformer::processExpr`, "
                       "these should all be removed by this point!",
                       expr->startPosition(), expr->endPosition());
            break;
        case Expr::Kind::ImplicitCast:
            processImplicitCastExpr(llvm::dyn_cast<ImplicitCastExpr>(expr));
            break;
        case Expr::Kind::ImplicitDeref:
            processImplicitDerefExpr(llvm::dyn_cast<ImplicitDerefExpr>(expr));
            break;
        case Expr::Kind::InfixOperator:
            processInfixOperatorExpr(llvm::dyn_cast<InfixOperatorExpr>(expr));
            break;
        case Expr::Kind::Is:
            processIsExpr(llvm::dyn_cast<IsExpr>(expr));
            break;
        case Expr::Kind::LabeledArgument:
            processLabeledArgumentExpr(llvm::dyn_cast<LabeledArgumentExpr>(expr));
            break;
        case Expr::Kind::LocalVariableRef:
            // Local variable ref doesn't hold any temporaries or do anything that needs processed here.
            break;
        case Expr::Kind::LValueToRValue:
            processLValueToRValueExpr(llvm::dyn_cast<LValueToRValueExpr>(expr));
            break;
        case Expr::Kind::MemberAccessCall:
            printError("[INTERNAL] `MemberAccessCallExpr` found in `CodeTransformer::processExpr`, "
                       "these should all be removed by this point!",
                       expr->startPosition(), expr->endPosition());
            break;
        case Expr::Kind::MemberFunctionCall:
            processMemberFunctionCallExpr(expr);
            break;
        case Expr::Kind::MemberPostfixOperatorCall:
            processMemberPostfixOperatorCallExpr(expr);
            break;
        case Expr::Kind::MemberPrefixOperatorCall:
            processMemberPrefixOperatorCallExpr(expr);
            break;
        case Expr::Kind::MemberPropertyRef:
            processMemberPropertyRefExpr(llvm::dyn_cast<MemberPropertyRefExpr>(expr));
            break;
        case Expr::Kind::MemberSubscriptCall:
            processMemberSubscriptCallExpr(llvm::dyn_cast<MemberSubscriptCallExpr>(expr));
            break;
        case Expr::Kind::MemberVariableRef:
            processMemberVariableRefExpr(llvm::dyn_cast<MemberVariableRefExpr>(expr));
        case Expr::Kind::ParameterRef:
            // Parameter ref just holds the index to a parameter, it doesn't do anything that needs processed here.
            // If we `copy` a parameter it will be contained in another `Expr` that will be processed. The index itself
            // doesn't need processed.
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
        case Expr::Kind::PropertyGetCall:
            processPropertyGetCallExpr(expr);
            break;
        case Expr::Kind::PropertySetCall:
            processPropertySetCallExpr(llvm::dyn_cast<PropertySetCallExpr>(expr));
            break;
        case Expr::Kind::PropertyRef:
            // Property ref is a reference to a global `PropertyDecl`, nothing we need to do here.
            break;
        case Expr::Kind::Ref:
            processRefExpr(llvm::dyn_cast<RefExpr>(expr));
            break;
        case Expr::Kind::SubscriptCall:
            printError("[INTERNAL] `SubscriptCallExpr` found in `CodeTransformer::processExpr`, "
                       "this should be impossible. Subscript operators cannot be global!",
                       expr->startPosition(), expr->endPosition());
            break;
        case Expr::Kind::SubscriptRef:
            // A reference to a subscript decl doesn't need processed here
            break;
        case Expr::Kind::TemplateConstRef:
            printError("[INTERNAL] `TemplateConstRefExpr` found in `CodeTransformer::processExpr`, "
                       "these should all by removed at this point!",
                       expr->startPosition(), expr->endPosition());
            break;
        case Expr::Kind::Ternary:
            processTernaryExpr(llvm::dyn_cast<TernaryExpr>(expr));
            break;
        case Expr::Kind::Try:
            processTryExpr(llvm::dyn_cast<TryExpr>(expr));
            break;
        case Expr::Kind::ValueLiteral:
            // TODO: Should we process these if they have a suffix?
            break;
        case Expr::Kind::VariableDecl:
            processVariableDeclExpr(llvm::dyn_cast<VariableDeclExpr>(expr));
            break;
        case Expr::Kind::VariableRef:
            // Variable references don't need processed as they don't create temporary values
            break;
        case Expr::Kind::VTableFunctionReference:
            break;

        default:
            printError("[INTERNAL] unsupported expression found in `CodeTransformer::processExpr`!",
                       expr->startPosition(), expr->endPosition());
            break;
    }
}

void gulc::CodeTransformer::processArrayLiteralExpr(gulc::ArrayLiteralExpr* arrayLiteralExpr) {
    for (Expr* index : arrayLiteralExpr->indexes) {
        processExpr(index);
    }
}

void gulc::CodeTransformer::processAsExpr(gulc::AsExpr* asExpr) {
    processExpr(asExpr->expr);
}

void gulc::CodeTransformer::processAssignmentOperatorExpr(gulc::AssignmentOperatorExpr* assignmentOperatorExpr) {
    processExpr(assignmentOperatorExpr->leftValue);
    processExpr(assignmentOperatorExpr->rightValue);
}

void gulc::CodeTransformer::processConstructorCallExpr(gulc::ConstructorCallExpr* constructorCallExpr) {
    // If no `objectRef` is provided then we create a temporary value and use that as the object reference to be
    // constructed. In the future we should replace
    // `uninitializedVar = TypeInit()` with `TypeInit::init(self: ref mut uninitializedVar)`
    if (constructorCallExpr->objectRef == nullptr) {
        constructorCallExpr->objectRef = createTemporaryValue(
                constructorCallExpr->valueType,
                constructorCallExpr->startPosition(),
                constructorCallExpr->endPosition()
            );
    }

    for (LabeledArgumentExpr* labeledArgumentExpr : constructorCallExpr->arguments) {
        processExpr(labeledArgumentExpr->argument);
    }
}

void gulc::CodeTransformer::processFunctionCallExpr(gulc::Expr*& expr) {
    auto functionCallExpr = llvm::dyn_cast<FunctionCallExpr>(expr);

    processExpr(functionCallExpr->functionReference);

    for (LabeledArgumentExpr* labeledArgumentExpr : functionCallExpr->arguments) {
        processExpr(labeledArgumentExpr->argument);
    }

    // If the function doesn't return void then we have to store the value as a temporary value. The reason we do this
    // is to make the result able to be passed into a destructor when needed.
    if (!(llvm::isa<BuiltInType>(functionCallExpr->valueType) &&
            llvm::dyn_cast<BuiltInType>(functionCallExpr->valueType)->sizeInBytes() == 0)) {
        auto tempValueLocalVarRef = createTemporaryValue(
                functionCallExpr->valueType,
                functionCallExpr->startPosition(),
                functionCallExpr->endPosition()
            );
        auto saveFunctionResult = new AssignmentOperatorExpr(
                tempValueLocalVarRef, functionCallExpr,
                functionCallExpr->startPosition(), functionCallExpr->endPosition()
            );
        saveFunctionResult->valueType = functionCallExpr->valueType->deepCopy();
        saveFunctionResult->valueType->setIsLValue(true);

        // Replace the function call with a wrapped save assignment for the function call
        expr = saveFunctionResult;
    }
}

void gulc::CodeTransformer::processImplicitCastExpr(gulc::ImplicitCastExpr* implicitCastExpr) {
    processExpr(implicitCastExpr->expr);
}

void gulc::CodeTransformer::processImplicitDerefExpr(gulc::ImplicitDerefExpr* implicitDerefExpr) {
    processExpr(implicitDerefExpr->nestedExpr);
}

void gulc::CodeTransformer::processInfixOperatorExpr(gulc::InfixOperatorExpr* infixOperatorExpr) {
    processExpr(infixOperatorExpr->leftValue);
    processExpr(infixOperatorExpr->rightValue);
}

void gulc::CodeTransformer::processIsExpr(gulc::IsExpr* isExpr) {
    processExpr(isExpr->expr);
}

void gulc::CodeTransformer::processLabeledArgumentExpr(gulc::LabeledArgumentExpr* labeledArgumentExpr) {
    processExpr(labeledArgumentExpr->argument);
}

void gulc::CodeTransformer::processLValueToRValueExpr(gulc::LValueToRValueExpr* lValueToRValueExpr) {
    processExpr(lValueToRValueExpr->lvalue);
}

void gulc::CodeTransformer::processMemberFunctionCallExpr(gulc::Expr*& expr) {
    auto memberFunctionCallExpr = llvm::dyn_cast<MemberFunctionCallExpr>(expr);

    processExpr(memberFunctionCallExpr->selfArgument);
    processExpr(memberFunctionCallExpr->functionReference);

    for (LabeledArgumentExpr* labeledArgumentExpr : memberFunctionCallExpr->arguments) {
        processExpr(labeledArgumentExpr->argument);
    }

    // If the function doesn't return void then we have to store the value as a temporary value. The reason we do this
    // is to make the result able to be passed into a destructor when needed.
    if (!(llvm::isa<BuiltInType>(memberFunctionCallExpr->valueType) &&
            llvm::dyn_cast<BuiltInType>(memberFunctionCallExpr->valueType)->sizeInBytes() == 0)) {
        auto tempValueLocalVarRef = createTemporaryValue(
                memberFunctionCallExpr->valueType,
                memberFunctionCallExpr->startPosition(),
                memberFunctionCallExpr->endPosition()
        );
        auto saveFunctionResult = new AssignmentOperatorExpr(
                tempValueLocalVarRef, memberFunctionCallExpr,
                memberFunctionCallExpr->startPosition(), memberFunctionCallExpr->endPosition()
        );
        saveFunctionResult->valueType = memberFunctionCallExpr->valueType->deepCopy();
        saveFunctionResult->valueType->setIsLValue(true);

        // Replace the function call with a wrapped save assignment for the function call
        expr = saveFunctionResult;
    }
}

void gulc::CodeTransformer::processMemberPostfixOperatorCallExpr(Expr*& expr) {
    auto memberPostfixOperatorCallExpr = llvm::dyn_cast<MemberPostfixOperatorCallExpr>(expr);

    processExpr(memberPostfixOperatorCallExpr->nestedExpr);

    // If the function doesn't return void then we have to store the value as a temporary value. The reason we do this
    // is to make the result able to be passed into a destructor when needed.
    if (!(llvm::isa<BuiltInType>(memberPostfixOperatorCallExpr->valueType) &&
          llvm::dyn_cast<BuiltInType>(memberPostfixOperatorCallExpr->valueType)->sizeInBytes() == 0)) {
        auto tempValueLocalVarRef = createTemporaryValue(
                memberPostfixOperatorCallExpr->valueType,
                memberPostfixOperatorCallExpr->startPosition(),
                memberPostfixOperatorCallExpr->endPosition()
        );
        auto savePostfixOperatorResult = new AssignmentOperatorExpr(
                tempValueLocalVarRef, memberPostfixOperatorCallExpr,
                memberPostfixOperatorCallExpr->startPosition(), memberPostfixOperatorCallExpr->endPosition()
        );
        savePostfixOperatorResult->valueType = memberPostfixOperatorCallExpr->valueType->deepCopy();
        savePostfixOperatorResult->valueType->setIsLValue(true);

        // Replace the function call with a wrapped save assignment for the function call
        expr = savePostfixOperatorResult;
    }
}

void gulc::CodeTransformer::processMemberPrefixOperatorCallExpr(Expr*& expr) {
    auto memberPrefixOperatorCallExpr = llvm::dyn_cast<MemberPrefixOperatorCallExpr>(expr);

    processExpr(memberPrefixOperatorCallExpr->nestedExpr);

    // If the function doesn't return void then we have to store the value as a temporary value. The reason we do this
    // is to make the result able to be passed into a destructor when needed.
    if (!(llvm::isa<BuiltInType>(memberPrefixOperatorCallExpr->valueType) &&
          llvm::dyn_cast<BuiltInType>(memberPrefixOperatorCallExpr->valueType)->sizeInBytes() == 0)) {
        auto tempValueLocalVarRef = createTemporaryValue(
                memberPrefixOperatorCallExpr->valueType,
                memberPrefixOperatorCallExpr->startPosition(),
                memberPrefixOperatorCallExpr->endPosition()
        );
        auto savePrefixOperatorResult = new AssignmentOperatorExpr(
                tempValueLocalVarRef, memberPrefixOperatorCallExpr,
                memberPrefixOperatorCallExpr->startPosition(), memberPrefixOperatorCallExpr->endPosition()
        );
        savePrefixOperatorResult->valueType = memberPrefixOperatorCallExpr->valueType->deepCopy();
        savePrefixOperatorResult->valueType->setIsLValue(true);

        // Replace the function call with a wrapped save assignment for the function call
        expr = savePrefixOperatorResult;
    }
}

void gulc::CodeTransformer::processMemberPropertyRefExpr(gulc::MemberPropertyRefExpr* memberPropertyRefExpr) {
    processExpr(memberPropertyRefExpr->object);

    // TODO: We need to make the result a temporary value to be destructed later if `returnType` isn't `void`.
}

void gulc::CodeTransformer::processMemberSubscriptCallExpr(gulc::MemberSubscriptCallExpr* memberSubscriptCallExpr) {
    processExpr(memberSubscriptCallExpr->selfArgument);

    for (LabeledArgumentExpr* labeledArgumentExpr : memberSubscriptCallExpr->arguments) {
        processExpr(labeledArgumentExpr->argument);
    }

    // TODO: We need to make the result a temporary value to be destructed later if `returnType` isn't `void`.
}

void gulc::CodeTransformer::processMemberVariableRefExpr(gulc::MemberVariableRefExpr* memberVariableRefExpr) {
    processExpr(memberVariableRefExpr->object);
}

void gulc::CodeTransformer::processParenExpr(gulc::ParenExpr* parenExpr) {
    processExpr(parenExpr->nestedExpr);
}

void gulc::CodeTransformer::processPostfixOperatorExpr(gulc::PostfixOperatorExpr* postfixOperatorExpr) {
    processExpr(postfixOperatorExpr->nestedExpr);
}

void gulc::CodeTransformer::processPrefixOperatorExpr(gulc::PrefixOperatorExpr* prefixOperatorExpr) {
    processExpr(prefixOperatorExpr->nestedExpr);
}

void gulc::CodeTransformer::processPropertyGetCallExpr(gulc::Expr*& expr) {
    auto propertyGetCallExpr = llvm::dyn_cast<PropertyGetCallExpr>(expr);

    switch (propertyGetCallExpr->propertyReference->getExprKind()) {
        case Expr::Kind::MemberPropertyRef: {
            auto memberPropertyRef = llvm::dyn_cast<MemberPropertyRefExpr>(propertyGetCallExpr->propertyReference);

            processExpr(memberPropertyRef->object);
            break;
        }
        case Expr::Kind::PropertyRef:
            // I don't think there is anything we need to do for global properties.
            break;
        default:
            printError("[INTERNAL] unknown property reference!",
                       propertyGetCallExpr->startPosition(), propertyGetCallExpr->endPosition());
            return;
    }

    // We need to be able to call the destructor on the result of `prop::get`, to support that we store it in a
    // temporary value. This also gives us the added benefit of making the result an `lvalue` which is another thing we
    // want.
    auto tempValueLocalVarRef = createTemporaryValue(
            propertyGetCallExpr->valueType,
            propertyGetCallExpr->startPosition(),
            propertyGetCallExpr->endPosition()
    );
    auto saveGetResult = new AssignmentOperatorExpr(
            tempValueLocalVarRef, propertyGetCallExpr,
            propertyGetCallExpr->startPosition(), propertyGetCallExpr->endPosition()
    );
    saveGetResult->valueType = propertyGetCallExpr->valueType->deepCopy();
    saveGetResult->valueType->setIsLValue(true);

    // Replace the get call with a wrapped save assignment for the get
    expr = saveGetResult;
}

void gulc::CodeTransformer::processPropertySetCallExpr(gulc::PropertySetCallExpr* propertySetCallExpr) {
    switch (propertySetCallExpr->propertyReference->getExprKind()) {
        case Expr::Kind::MemberPropertyRef: {
            auto memberPropertyRef = llvm::dyn_cast<MemberPropertyRefExpr>(propertySetCallExpr->propertyReference);

            processExpr(memberPropertyRef->object);
            break;
        }
        case Expr::Kind::PropertyRef:
            // I don't think there is anything we need to do for global properties.
            break;
        default:
            printError("[INTERNAL] unknown property reference!",
                       propertySetCallExpr->startPosition(), propertySetCallExpr->endPosition());
            return;
    }

    processExpr(propertySetCallExpr->value);
}

void gulc::CodeTransformer::processRefExpr(gulc::RefExpr* refExpr) {
    processExpr(refExpr->nestedExpr);
}

void gulc::CodeTransformer::processTernaryExpr(gulc::TernaryExpr* ternaryExpr) {
    processExpr(ternaryExpr->condition);
    processExpr(ternaryExpr->trueExpr);
    processExpr(ternaryExpr->falseExpr);
}

void gulc::CodeTransformer::processTryExpr(gulc::TryExpr* tryExpr) {
    processExpr(tryExpr->nestedExpr);
}

void gulc::CodeTransformer::processVariableDeclExpr(gulc::VariableDeclExpr* variableDeclExpr) {
    if (variableDeclExpr->initialValue != nullptr) {
        processExpr(variableDeclExpr->initialValue);
    }

    // The local variable list has already been validated so we just add to the list.
    _localVariables.push_back(variableDeclExpr);
}

gulc::TemporaryValueRefExpr* gulc::CodeTransformer::createTemporaryValue(gulc::Type* type,
                                                                         gulc::TextPosition startPosition,
                                                                         gulc::TextPosition endPosition) {
    auto tempValueLocalVarName = std::string("_tmpValVar.") + std::to_string(_temporaryValueVarNumber);
    auto tempValueLocalVar = new VariableDeclExpr(
            Identifier(
                    startPosition,
                    endPosition,
                    tempValueLocalVarName
            ),
            type,
            nullptr,
            false,
            startPosition,
            endPosition
    );

    ++_temporaryValueVarNumber;

    // The list of temporary values is a vector of a vector so we grab the last vector and add to that list.
    (_temporaryValues.end() - 1)->push_back(tempValueLocalVar);

    auto tempValueLocalVarRef = new TemporaryValueRefExpr(
            startPosition,
            endPosition,
            tempValueLocalVarName
        );
    tempValueLocalVarRef->valueType = type->deepCopy();
    tempValueLocalVarRef->valueType->setIsLValue(true);

    return tempValueLocalVarRef;
}
