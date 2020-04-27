#include <utilities/TypeHelper.hpp>
#include <ast/types/DimensionType.hpp>
#include <ast/types/PointerType.hpp>
#include <ast/types/ReferenceType.hpp>
#include <ast/types/TemplatedType.hpp>
#include <ast/exprs/IdentifierExpr.hpp>
#include <ast/types/UnresolvedType.hpp>
#include <ast/types/FlatArrayType.hpp>
#include "BasicTypeResolver.hpp"

void gulc::BasicTypeResolver::processFiles(std::vector<ASTFile>& files) {
    for (ASTFile& file : files) {
        _currentFile = &file;

        for (Decl* decl : file.declarations) {
            processDecl(decl);
        }
    }
}

void gulc::BasicTypeResolver::printError(std::string const& message, gulc::TextPosition startPosition,
                                         gulc::TextPosition endPosition) const {
    std::cerr << "gulc type resolver error[" << _filePaths[_currentFile->sourceFileID] << ", "
                                         "{" << startPosition.line << ", " << startPosition.column << " "
                                         "to " << endPosition.line << ", " << endPosition.column << "}]: "
              << message << std::endl;
    std::exit(1);
}

void gulc::BasicTypeResolver::printWarning(std::string const& message, gulc::TextPosition startPosition,
                                           gulc::TextPosition endPosition) const {
    std::cerr << "gulc type resolver warning[" << _filePaths[_currentFile->sourceFileID] << ", "
                                           "{" << startPosition.line << ", " << startPosition.column << " "
                                           "to " << endPosition.line << ", " << endPosition.column << "}]: "
              << message << std::endl;
}

bool gulc::BasicTypeResolver::resolveType(gulc::Type*& type) const {
    bool result = TypeHelper::resolveType(type, _currentFile, _templateParameters, _containingDecls);

    if (result) {
        processType(type);
    }

    return result;
}

void gulc::BasicTypeResolver::processType(gulc::Type* type) const {
    if (llvm::isa<DimensionType>(type)) {
        auto dimensionType = llvm::dyn_cast<DimensionType>(type);

        processType(dimensionType->nestedType);
    } else if (llvm::isa<FlatArrayType>(type)) {
        auto flatArrayType = llvm::dyn_cast<FlatArrayType>(type);

        processExpr(flatArrayType->length);
        processType(flatArrayType->indexType);
    } else if (llvm::isa<PointerType>(type)) {
        auto pointerType = llvm::dyn_cast<PointerType>(type);

        processType(pointerType->nestedType);
    } else if (llvm::isa<ReferenceType>(type)) {
        auto referenceType = llvm::dyn_cast<ReferenceType>(type);

        processType(referenceType->nestedType);
    } else if (llvm::isa<TemplatedType>(type)) {
        auto templatedType = llvm::dyn_cast<TemplatedType>(type);

        for (Expr*& argument : templatedType->templateArguments()) {
            processTemplateArgumentExpr(argument);
        }
    }
}

void gulc::BasicTypeResolver::processDecl(gulc::Decl* decl, bool isGlobal) {
    switch (decl->getDeclKind()) {
        case Decl::Kind::Import:
            // We skip imports, they're no longer useful here...
            break;

        case Decl::Kind::CallOperator:
        case Decl::Kind::Constructor:
        case Decl::Kind::Destructor:
        case Decl::Kind::Function:
        case Decl::Kind::Operator:
            processFunctionDecl(llvm::dyn_cast<FunctionDecl>(decl));
            break;
        case Decl::Kind::Namespace:
            processNamespaceDecl(llvm::dyn_cast<NamespaceDecl>(decl));
            break;
        case Decl::Kind::Property:
            processPropertyDecl(llvm::dyn_cast<PropertyDecl>(decl));
            break;
        case Decl::Kind::PropertyGet:
            processPropertyGetDecl(llvm::dyn_cast<PropertyGetDecl>(decl));
            break;
        case Decl::Kind::PropertySet:
            processPropertySetDecl(llvm::dyn_cast<PropertySetDecl>(decl));
            break;
        case Decl::Kind::Struct:
            processStructDecl(llvm::dyn_cast<StructDecl>(decl));
            break;
        case Decl::Kind::SubscriptOperator:
            processSubscriptOperatorDecl(llvm::dyn_cast<SubscriptOperatorDecl>(decl));
            break;
        case Decl::Kind::SubscriptOperatorGet:
            processSubscriptOperatorGetDecl(llvm::dyn_cast<SubscriptOperatorGetDecl>(decl));
            break;
        case Decl::Kind::SubscriptOperatorSet:
            processSubscriptOperatorSetDecl(llvm::dyn_cast<SubscriptOperatorSetDecl>(decl));
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
            processTypeAliasDecl(llvm::dyn_cast<TypeAliasDecl>(decl));
            break;
        case Decl::Kind::Variable:
            processVariableDecl(llvm::dyn_cast<VariableDecl>(decl), isGlobal);
            break;

        default:
            printError("INTERNAL ERROR - unhandled Decl type found in `BasicTypeResolver`!",
                       decl->startPosition(), decl->endPosition());
            break;
    }
}

void gulc::BasicTypeResolver::processFunctionDecl(gulc::FunctionDecl* functionDecl) {
    for (ParameterDecl* parameter : functionDecl->parameters()) {
        processParameterDecl(parameter);
    }

    // Return type might be null for `void`
    if (functionDecl->returnType != nullptr) {
        if (!resolveType(functionDecl->returnType)) {
            printError("function return type `" + functionDecl->returnType->toString() + "` was not found!",
                       functionDecl->startPosition(), functionDecl->endPosition());
        }
    }

    processStmt(functionDecl->body());
}

void gulc::BasicTypeResolver::processNamespaceDecl(gulc::NamespaceDecl* namespaceDecl) {
    _containingDecls.push_back(namespaceDecl);

    for (Decl* nestedDecl : namespaceDecl->nestedDecls()) {
        processDecl(nestedDecl);
    }

    _containingDecls.pop_back();
}

void gulc::BasicTypeResolver::processParameterDecl(gulc::ParameterDecl* parameterDecl) const {
    if (!resolveType(parameterDecl->type)) {
        printError("function parameter type `" + parameterDecl->type->toString() + "` was not found!",
                   parameterDecl->startPosition(), parameterDecl->endPosition());
    }

    if (parameterDecl->defaultValue != nullptr) {
        processExpr(parameterDecl->defaultValue);
    }
}

void gulc::BasicTypeResolver::processPropertyDecl(gulc::PropertyDecl* propertyDecl) {
    if (!resolveType(propertyDecl->type)) {
        printError("property type `" + propertyDecl->type->toString() + "` was not found!",
                   propertyDecl->startPosition(), propertyDecl->endPosition());
    }

    for (PropertyGetDecl* getter : propertyDecl->getters()) {
        // We call the generic `processDecl` so that our attributes are processed proeprly
        processDecl(getter);
    }

    if (propertyDecl->hasSetter()) {
        processDecl(propertyDecl->setter());
    }
}

void gulc::BasicTypeResolver::processPropertyGetDecl(gulc::PropertyGetDecl* propertyGetDecl) {
    processFunctionDecl(propertyGetDecl);
}

void gulc::BasicTypeResolver::processPropertySetDecl(gulc::PropertySetDecl* propertySetDecl) {
    processFunctionDecl(propertySetDecl);
}

void gulc::BasicTypeResolver::processStructDecl(gulc::StructDecl* structDecl) {
    for (Type*& inheritedType : structDecl->inheritedTypes()) {
        if (!resolveType(inheritedType)) {
            printError("struct inherited type `" + inheritedType->toString() + "` was not found!",
                       structDecl->startPosition(), structDecl->endPosition());
        }
    }

    _containingDecls.push_back(structDecl);

    for (Decl* member : structDecl->ownedMembers()) {
        processDecl(member, false);
    }

    // Make sure we process the constructors and destructor since they are NOT in the `ownedMembers` list...
    for (ConstructorDecl* constructor : structDecl->constructors()) {
        processDecl(constructor);
    }

    if (structDecl->destructor != nullptr) {
        processDecl(structDecl->destructor);
    }

    _containingDecls.pop_back();
}

void gulc::BasicTypeResolver::processSubscriptOperatorDecl(gulc::SubscriptOperatorDecl* subscriptOperatorDecl) {
    for (ParameterDecl* parameter : subscriptOperatorDecl->parameters()) {
        processParameterDecl(parameter);
    }

    // Return type might be null for `void`
    if (subscriptOperatorDecl->type != nullptr) {
        if (!resolveType(subscriptOperatorDecl->type)) {
            printError("subscript type `" + subscriptOperatorDecl->type->toString() + "` was not found!",
                       subscriptOperatorDecl->startPosition(), subscriptOperatorDecl->endPosition());
        }
    } else {
        printError("subscripts MUST have a type!",
                   subscriptOperatorDecl->startPosition(), subscriptOperatorDecl->endPosition());
    }

    for (SubscriptOperatorGetDecl* getter : subscriptOperatorDecl->getters()) {
        processDecl(getter);
    }

    if (subscriptOperatorDecl->hasSetter()) {
        processDecl(subscriptOperatorDecl->setter());
    }
}

void gulc::BasicTypeResolver::processSubscriptOperatorGetDecl(gulc::SubscriptOperatorGetDecl* subscriptOperatorGetDecl) {
    processFunctionDecl(subscriptOperatorGetDecl);
}

void gulc::BasicTypeResolver::processSubscriptOperatorSetDecl(gulc::SubscriptOperatorSetDecl* subscriptOperatorSetDecl) {
    processFunctionDecl(subscriptOperatorSetDecl);
}

void gulc::BasicTypeResolver::processTemplateFunctionDecl(gulc::TemplateFunctionDecl* templateFunctionDecl) {
    for (TemplateParameterDecl* templateParameter : templateFunctionDecl->templateParameters()) {
        processTemplateParameterDecl(templateParameter);
    }

    _templateParameters.push_back(&templateFunctionDecl->templateParameters());

    processFunctionDecl(templateFunctionDecl);

    _templateParameters.pop_back();
}

void gulc::BasicTypeResolver::processTemplateParameterDecl(gulc::TemplateParameterDecl* templateParameterDecl) const {
    // If the template parameter is a const then we have to process its underlying type
    if (templateParameterDecl->templateParameterKind() == TemplateParameterDecl::TemplateParameterKind::Const) {
        if (!resolveType(templateParameterDecl->constType)) {
            printError("const template parameter type `" + templateParameterDecl->constType->toString() + "` was not found!",
                       templateParameterDecl->startPosition(), templateParameterDecl->endPosition());
        }
    }
}

void gulc::BasicTypeResolver::processTemplateStructDecl(gulc::TemplateStructDecl* templateStructDecl) {
    for (TemplateParameterDecl* templateParameter : templateStructDecl->templateParameters()) {
        processTemplateParameterDecl(templateParameter);
    }

    _templateParameters.push_back(&templateStructDecl->templateParameters());

    processStructDecl(templateStructDecl);

    _templateParameters.pop_back();
}

void gulc::BasicTypeResolver::processTemplateTraitDecl(gulc::TemplateTraitDecl* templateTraitDecl) {
    for (TemplateParameterDecl* templateParameter : templateTraitDecl->templateParameters()) {
        processTemplateParameterDecl(templateParameter);
    }

    _templateParameters.push_back(&templateTraitDecl->templateParameters());

    processTraitDecl(templateTraitDecl);

    _templateParameters.pop_back();
}

void gulc::BasicTypeResolver::processTraitDecl(gulc::TraitDecl* traitDecl) {
    for (Type*& inheritedType : traitDecl->inheritedTypes()) {
        resolveType(inheritedType);
    }

    _containingDecls.push_back(traitDecl);

    for (Decl* member : traitDecl->ownedMembers()) {
        processDecl(member, false);

        if (llvm::isa<VariableDecl>(member)) {
            // Trait members can only be `const` or `static`
            auto variableMember = llvm::dyn_cast<VariableDecl>(member);

            if (!variableMember->isConstExpr() && !variableMember->isStatic()) {
                printError("traits can only contain `const` or `static` variables!",
                           variableMember->startPosition(), variableMember->endPosition());
            }
        }
    }

    _containingDecls.pop_back();
}

void gulc::BasicTypeResolver::processTypeAliasDecl(gulc::TypeAliasDecl* typeAliasDecl) {
    // TODO: Detect circular references with the potential for `typealias prefix ^<T> = ^T;` or something.
    for (TemplateParameterDecl* templateParameter : typeAliasDecl->templateParameters()) {
        processTemplateParameterDecl(templateParameter);
    }

    if (typeAliasDecl->hasTemplateParameters()) {
        _templateParameters.push_back(&typeAliasDecl->templateParameters());
    }

    resolveType(typeAliasDecl->typeValue);

    if (typeAliasDecl->hasTemplateParameters()) {
        _templateParameters.pop_back();
    }
}

void gulc::BasicTypeResolver::processVariableDecl(gulc::VariableDecl* variableDecl, bool isGlobal) const {
    if (isGlobal) {
        if (!variableDecl->isConstExpr() && !variableDecl->isStatic()) {
            printError("global variables must be marked `const` or `static`!",
                       variableDecl->startPosition(), variableDecl->endPosition());
        }
    }

    if (variableDecl->type == nullptr) {
        printError("variables outside of function bodies and similar MUST have a type specified!",
                   variableDecl->startPosition(), variableDecl->endPosition());
    } else {
        if (!resolveType(variableDecl->type)) {
            printError("variable type `" + variableDecl->type->toString() + "` was not found!",
                       variableDecl->type->startPosition(), variableDecl->type->endPosition());
        }
    }

    if (variableDecl->initialValue != nullptr) {
        processExpr(variableDecl->initialValue);
    }
}

void gulc::BasicTypeResolver::processStmt(gulc::Stmt* stmt) const {
    switch (stmt->getStmtKind()) {
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
        case Stmt::Kind::Expr:
            processExpr(llvm::dyn_cast<Expr>(stmt));
            break;

        default:
            printError("INTERNAL ERROR - unhandled Stmt type found in `BasicTypeResolver`!",
                       stmt->startPosition(), stmt->endPosition());
            break;
    }
}

void gulc::BasicTypeResolver::processCaseStmt(gulc::CaseStmt* caseStmt) const {
    if (!caseStmt->isDefault()) {
        processExpr(caseStmt->condition);
    }

    processStmt(caseStmt->trueStmt);
}

void gulc::BasicTypeResolver::processCatchStmt(gulc::CatchStmt* catchStmt) const {
    if (catchStmt->hasExceptionType()) {
        resolveType(catchStmt->exceptionType);
    }

    processStmt(catchStmt->body());
}

void gulc::BasicTypeResolver::processCompoundStmt(gulc::CompoundStmt* compoundStmt) const {
    for (Stmt* statement : compoundStmt->statements) {
        processStmt(statement);
    }
}

void gulc::BasicTypeResolver::processDoStmt(gulc::DoStmt* doStmt) const {
    processStmt(doStmt->body());
    processExpr(doStmt->condition);
}

void gulc::BasicTypeResolver::processForStmt(gulc::ForStmt* forStmt) const {
    if (forStmt->init != nullptr) {
        processExpr(forStmt->init);
    }

    if (forStmt->condition != nullptr) {
        processExpr(forStmt->condition);
    }

    if (forStmt->iteration != nullptr) {
        processExpr(forStmt->iteration);
    }

    processStmt(forStmt->body());
}

void gulc::BasicTypeResolver::processIfStmt(gulc::IfStmt* ifStmt) const {
    processExpr(ifStmt->condition);
    processStmt(ifStmt->trueBody());

    if (ifStmt->hasFalseBody()) {
        processStmt(ifStmt->falseBody());
    }
}

void gulc::BasicTypeResolver::processLabeledStmt(gulc::LabeledStmt* labeledStmt) const {
    processStmt(labeledStmt);
}

void gulc::BasicTypeResolver::processReturnStmt(gulc::ReturnStmt* returnStmt) const {
    if (returnStmt->returnValue != nullptr) {
        processExpr(returnStmt->returnValue);
    }
}

void gulc::BasicTypeResolver::processSwitchStmt(gulc::SwitchStmt* switchStmt) const {
    processExpr(switchStmt->condition);

    for (Stmt* statement : switchStmt->statements) {
        processStmt(statement);
    }
}

void gulc::BasicTypeResolver::processTryStmt(gulc::TryStmt* tryStmt) const {
    processStmt(tryStmt->body());

    for (CatchStmt* catchStmt : tryStmt->catchStatements()) {
        processCatchStmt(catchStmt);
    }

    if (tryStmt->hasFinallyStatement()) {
        processStmt(tryStmt->finallyStatement());
    }
}

void gulc::BasicTypeResolver::processWhileStmt(gulc::WhileStmt* whileStmt) const {
    processExpr(whileStmt->condition);
    processStmt(whileStmt->body());
}

void gulc::BasicTypeResolver::processTemplateArgumentExpr(gulc::Expr*& expr) const {
    // TODO: Support more than just the `IdentifierExpr` (i.e. we need to support saying `std.math.vec2`
    if (llvm::isa<IdentifierExpr>(expr)) {
        // TODO: What happens if something is shadowing a type? Do we disallow `const` variables from being able to
        //       shadow Decls? This could prevent that issue from ever arising...
        auto identifierExpr = llvm::dyn_cast<IdentifierExpr>(expr);

        Type* tmpType = new UnresolvedType(Type::Qualifier::Unassigned, {},
                                           identifierExpr->identifier(),
                                           identifierExpr->templateArguments());

        if (resolveType(tmpType)) {
            // We clear them as they should now be used by `tmpType` OR no be deleted...
            identifierExpr->templateArguments().clear();

            expr = new TypeExpr(tmpType);

            // Delete the no longer needed identifier expression
            delete identifierExpr;
        } else {
            // TODO: When the type isn't found we should delete `tmpType` and look for a potential `const var`
            //       (but only if there aren't template arguments...)
            printError("type `" + identifierExpr->identifier().name() + "` was not found!",
                       identifierExpr->startPosition(), identifierExpr->endPosition());
        }
    } else {
        processExpr(expr);
    }
}

void gulc::BasicTypeResolver::processExpr(gulc::Expr* expr) const {
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
        case Expr::Kind::FunctionCall:
            processFunctionCallExpr(llvm::dyn_cast<FunctionCallExpr>(expr));
            break;
        case Expr::Kind::Has:
            processHasExpr(llvm::dyn_cast<HasExpr>(expr));
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
        case Expr::Kind::LabeledArgument:
            processLabeledArgumentExpr(llvm::dyn_cast<LabeledArgumentExpr>(expr));
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
        case Expr::Kind::Type:
            processTypeExpr(llvm::dyn_cast<TypeExpr>(expr));
            break;
        case Expr::Kind::ValueLiteral:
            processValueLiteralExpr(llvm::dyn_cast<ValueLiteralExpr>(expr));
            break;
        case Expr::Kind::VariableDecl:
            processVariableDeclExpr(llvm::dyn_cast<VariableDeclExpr>(expr));
            break;

        default:
            printError("INTERNAL ERROR - unhandled Expr type found in `BasicTypeResolver`!",
                       expr->startPosition(), expr->endPosition());
            break;
    }
}

void gulc::BasicTypeResolver::processArrayLiteralExpr(gulc::ArrayLiteralExpr* arrayLiteralExpr) const {
    for (Expr* index : arrayLiteralExpr->indexes) {
        processExpr(index);
    }
}

void gulc::BasicTypeResolver::processAsExpr(gulc::AsExpr* asExpr) const {
    processExpr(asExpr->expr);
    resolveType(asExpr->asType);
}

void gulc::BasicTypeResolver::processAssignmentOperatorExpr(gulc::AssignmentOperatorExpr* assignmentOperatorExpr) const {
    processExpr(assignmentOperatorExpr->leftValue);
    processExpr(assignmentOperatorExpr->rightValue);
}

void gulc::BasicTypeResolver::processFunctionCallExpr(gulc::FunctionCallExpr* functionCallExpr) const {
    processExpr(functionCallExpr->functionReference);

    for (Expr* argument : functionCallExpr->arguments) {
        processExpr(argument);
    }
}

void gulc::BasicTypeResolver::processHasExpr(gulc::HasExpr* hasExpr) const {
    processExpr(hasExpr->expr);
    resolveType(hasExpr->trait);
}

void gulc::BasicTypeResolver::processIdentifierExpr(gulc::IdentifierExpr* identifierExpr) const {
    for (Expr* templateArgument : identifierExpr->templateArguments()) {
        processExpr(templateArgument);
    }
}

void gulc::BasicTypeResolver::processIndexerCallExpr(gulc::IndexerCallExpr* indexerCallExpr) const {
    processExpr(indexerCallExpr->indexerReference);

    for (Expr* argument : indexerCallExpr->arguments) {
        processExpr(argument);
    }
}

void gulc::BasicTypeResolver::processInfixOperatorExpr(gulc::InfixOperatorExpr* infixOperatorExpr) const {
    processExpr(infixOperatorExpr->leftValue);
    processExpr(infixOperatorExpr->rightValue);
}

void gulc::BasicTypeResolver::processIsExpr(gulc::IsExpr* isExpr) const {
    processExpr(isExpr->expr);
    resolveType(isExpr->isType);
}

void gulc::BasicTypeResolver::processLabeledArgumentExpr(gulc::LabeledArgumentExpr* labeledArgumentExpr) const {
    processExpr(labeledArgumentExpr->argument);
}

void gulc::BasicTypeResolver::processMemberAccessCallExpr(gulc::MemberAccessCallExpr* memberAccessCallExpr) const {
    processExpr(memberAccessCallExpr->objectRef);
}

void gulc::BasicTypeResolver::processParenExpr(gulc::ParenExpr* parenExpr) const {
    processExpr(parenExpr->nestedExpr);
}

void gulc::BasicTypeResolver::processPostfixOperatorExpr(gulc::PostfixOperatorExpr* postfixOperatorExpr) const {
    processExpr(postfixOperatorExpr->nestedExpr);
}

void gulc::BasicTypeResolver::processPrefixOperatorExpr(gulc::PrefixOperatorExpr* prefixOperatorExpr) const {
    processExpr(prefixOperatorExpr->nestedExpr);
}

void gulc::BasicTypeResolver::processTernaryExpr(gulc::TernaryExpr* ternaryExpr) const {
    processExpr(ternaryExpr->condition);
    processExpr(ternaryExpr->trueExpr);
    processExpr(ternaryExpr->falseExpr);
}

void gulc::BasicTypeResolver::processTypeExpr(gulc::TypeExpr* typeExpr) const {
    resolveType(typeExpr->type);
}

void gulc::BasicTypeResolver::processValueLiteralExpr(gulc::ValueLiteralExpr* valueLiteralExpr) const {
    // TODO: Is there anything we can do here? I don't think there is anything we can do until `DeclInstantiator`...
}

void gulc::BasicTypeResolver::processVariableDeclExpr(gulc::VariableDeclExpr* variableDeclExpr) const {
    if (variableDeclExpr->type != nullptr) {
        resolveType(variableDeclExpr->type);
    }

    if (variableDeclExpr->initialValue != nullptr) {
        processExpr(variableDeclExpr->initialValue);
    }
}
