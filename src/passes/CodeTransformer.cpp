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
#include "CodeTransformer.hpp"

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
    // TODO: Once we support `func` within `enum` declarations we will need this.
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

    // Prototypes don't have bodies
    if (!functionDecl->isPrototype()) {
        processCompoundStmtHandleTempValues(functionDecl->body());
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

void gulc::CodeTransformer::processStmt(gulc::Stmt*& stmt) {
    std::size_t oldTemporaryValuesSize = _temporaryValues.size();
    _temporaryValues.push_back(std::vector<VariableDeclExpr*>());

    switch (stmt->getStmtKind()) {
        case Stmt::Kind::Break:
            // NOTE: There isn't anything in `break` that we need to process here.
            break;
        case Stmt::Kind::Case:
            processCaseStmt(llvm::dyn_cast<CaseStmt>(stmt));
            break;
        case Stmt::Kind::Catch:
            // TODO: I don't think this should be included in the general area either...
            processCatchStmt(llvm::dyn_cast<CatchStmt>(stmt));
            break;
        case Stmt::Kind::Compound:
            processCompoundStmt(llvm::dyn_cast<CompoundStmt>(stmt));
            break;
        case Stmt::Kind::Continue:
            // NOTE: There isn't anything in `continue` that we need to process here.
            break;
        case Stmt::Kind::DoCatch:
            processDoCatchStmt(llvm::dyn_cast<DoCatchStmt>(stmt));
            break;
        case Stmt::Kind::DoWhile:
            processDoWhileStmt(llvm::dyn_cast<DoWhileStmt>(stmt));
            break;
        case Stmt::Kind::For:
            processForStmt(llvm::dyn_cast<ForStmt>(stmt));
            break;
        case Stmt::Kind::Goto:
            // NOTE: There isn't anything in `goto` that we need to process here.
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
}

void gulc::CodeTransformer::processCaseStmt(gulc::CaseStmt* caseStmt) {
    if (!caseStmt->isDefault()) {
        processExpr(caseStmt->condition);
    }

    processStmt(caseStmt->trueStmt);
}

void gulc::CodeTransformer::processCatchStmt(gulc::CatchStmt* catchStmt) {
    processCompoundStmtHandleTempValues(catchStmt->body());
}

void gulc::CodeTransformer::processCompoundStmt(gulc::CompoundStmt* compoundStmt) {
    for (Stmt* stmt : compoundStmt->statements) {
        processStmt(stmt);
    }
}

void gulc::CodeTransformer::processCompoundStmtHandleTempValues(gulc::CompoundStmt* compoundStmt) {
    Stmt* stmt = compoundStmt;
    processStmt(stmt);
}

void gulc::CodeTransformer::processDoCatchStmt(gulc::DoCatchStmt* doCatchStmt) {
    processCompoundStmtHandleTempValues(doCatchStmt->body());

    for (CatchStmt* catchStmt : doCatchStmt->catchStatements()) {
        processCatchStmt(catchStmt);
    }

    if (doCatchStmt->hasFinallyStatement()) {
        processCompoundStmtHandleTempValues(doCatchStmt->finallyStatement());
    }
}

void gulc::CodeTransformer::processDoWhileStmt(gulc::DoWhileStmt* doWhileStmt) {
    processCompoundStmtHandleTempValues(doWhileStmt->body());
    processExpr(doWhileStmt->condition);
}

void gulc::CodeTransformer::processForStmt(gulc::ForStmt* forStmt) {
    if (forStmt->init != nullptr) {
        processExpr(forStmt->init);
    }

    if (forStmt->condition != nullptr) {
        processExpr(forStmt->condition);
    }

    if (forStmt->iteration != nullptr) {
        processExpr(forStmt->iteration);
    }

    processCompoundStmtHandleTempValues(forStmt->body());
}

void gulc::CodeTransformer::processIfStmt(gulc::IfStmt* ifStmt) {
    processExpr(ifStmt->condition);
    processCompoundStmtHandleTempValues(ifStmt->trueBody());

    if (ifStmt->hasFalseBody()) {
        // NOTE: `falseBody` can only be `IfStmt` or `CompoundStmt`
        if (llvm::isa<IfStmt>(ifStmt->falseBody())) {
            processIfStmtHandleTempValues(llvm::dyn_cast<IfStmt>(ifStmt->falseBody()));
        } else if (llvm::isa<CompoundStmt>(ifStmt->falseBody())) {
            processCompoundStmtHandleTempValues(llvm::dyn_cast<CompoundStmt>(ifStmt->falseBody()));
        }
    }
}

void gulc::CodeTransformer::processIfStmtHandleTempValues(gulc::IfStmt* ifStmt) {
    Stmt* stmt = ifStmt;
    processStmt(stmt);
}

void gulc::CodeTransformer::processLabeledStmt(gulc::LabeledStmt* labeledStmt) {
    processStmt(labeledStmt->labeledStmt);
}

void gulc::CodeTransformer::processReturnStmt(gulc::ReturnStmt* returnStmt) {
    if (returnStmt->returnValue != nullptr) {
        processExpr(returnStmt->returnValue);
    }
}

void gulc::CodeTransformer::processSwitchStmt(gulc::SwitchStmt* switchStmt) {
    processExpr(switchStmt->condition);

    for (Stmt* stmt : switchStmt->statements) {
        processStmt(stmt);
    }
}

void gulc::CodeTransformer::processWhileStmt(gulc::WhileStmt* whileStmt) {
    processExpr(whileStmt->condition);
    processCompoundStmtHandleTempValues(whileStmt->body());
}

// TODO: So I think the way we need to do temporaries is:
//        1. An `Expr` that can potentially create a new destructable object should add that to a list of temporaries
//        2. In `processStmt` after the statement is processed we should steal the list of temporaries (so we can
//           delete them)
//        3. Use this new list of temporaries to destruct the temporaries. Also keep a list of local variables to
//           destruct them as well.
// TODO: I think `CodeTransformer` should do the following:
//        1. Insert temporary values
//        2. Destruct local variables
//        3. Handle `DeferStmt`
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
        case Expr::Kind::PropertyRef:
            // Property ref is a reference to a global `PropertyDecl`, nothing we need to do here.
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
}

gulc::LocalVariableRefExpr* gulc::CodeTransformer::createTemporaryValue(gulc::Type* type,
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

    auto tempValueLocalVarRef = new LocalVariableRefExpr(
            startPosition,
            endPosition,
            tempValueLocalVarName
        );
    tempValueLocalVarRef->valueType = type->deepCopy();
    tempValueLocalVarRef->valueType->setIsLValue(true);

    return tempValueLocalVarRef;
}
