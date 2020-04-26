#include <ast/conts/EnsuresCont.hpp>
#include <ast/conts/RequiresCont.hpp>
#include <ast/conts/WhereCont.hpp>
#include <ast/types/DimensionType.hpp>
#include <ast/types/PointerType.hpp>
#include <ast/types/ReferenceType.hpp>
#include <ast/types/TemplatedType.hpp>
#include <ast/types/TemplateTypenameRefType.hpp>
#include "TemplateInstHelper.hpp"

void gulc::TemplateInstHelper::instantiateTemplateStructInstDecl(gulc::TemplateStructDecl* parentTemplateStruct,
                                                                 gulc::TemplateStructInstDecl* templateStructInstDecl,
                                                                 bool processBodyStmts) {
    this->processBodyStmts = processBodyStmts;

    // We instantiate the arguments here just in case any of them are default values that reference other template
    // parameters
    for (Expr* templateArgument : templateStructInstDecl->templateArguments()) {
        instantiateExpr(templateArgument);
    }

    templateParameters = &parentTemplateStruct->templateParameters();
    templateArguments = &templateStructInstDecl->templateArguments();

    for (Attr* attribute : templateStructInstDecl->attributes()) {
        instantiateAttr(attribute);
    }

    for (Cont* contract : templateStructInstDecl->contracts()) {
        instantiateCont(contract);
    }

    for (Type*& inheritedType : templateStructInstDecl->inheritedTypes()) {
        instantiateType(inheritedType);
    }

    for (Decl* ownedMember : templateStructInstDecl->ownedMembers()) {
        instantiateDecl(ownedMember);
    }

    for (ConstructorDecl* constructor : templateStructInstDecl->constructors()) {
        instantiateDecl(constructor);
    }

    if (templateStructInstDecl->destructor != nullptr) {
        instantiateDecl(templateStructInstDecl->destructor);
    }
}

void gulc::TemplateInstHelper::instantiateTemplateTraitInstDecl(gulc::TemplateTraitDecl* parentTemplateTrait,
                                                                gulc::TemplateTraitInstDecl* templateTraitInstDecl,
                                                                bool processBodyStmts) {
    this->processBodyStmts = processBodyStmts;

    // We instantiate the arguments here just in case any of them are default values that reference other template
    // parameters
    for (Expr* templateArgument : templateTraitInstDecl->templateArguments()) {
        instantiateExpr(templateArgument);
    }

    templateParameters = &parentTemplateTrait->templateParameters();
    templateArguments = &templateTraitInstDecl->templateArguments();

    for (Attr* attribute : templateTraitInstDecl->attributes()) {
        instantiateAttr(attribute);
    }

    for (Cont* contract : templateTraitInstDecl->contracts()) {
        instantiateCont(contract);
    }

    for (Type*& inheritedType : templateTraitInstDecl->inheritedTypes()) {
        instantiateType(inheritedType);
    }

    for (Decl* ownedMember : templateTraitInstDecl->ownedMembers()) {
        instantiateDecl(ownedMember);
    }
}

void gulc::TemplateInstHelper::instantiateAttr(gulc::Attr* attr) const {
    // TODO: There currently isn't anything to do here...
}

void gulc::TemplateInstHelper::instantiateCont(gulc::Cont* cont) const {
    if (llvm::isa<EnsuresCont>(cont)) {
        auto ensuresCont = llvm::dyn_cast<EnsuresCont>(cont);

        instantiateExpr(ensuresCont->condition);
    } else if (llvm::isa<RequiresCont>(cont)) {
        auto requiresCont = llvm::dyn_cast<RequiresCont>(cont);

        instantiateExpr(requiresCont->condition);
    } else if (llvm::isa<ThrowsCont>(cont)) {
        // TODO: Is there anything to do here? Exceptions are special types but can they be templated?
    } else if (llvm::isa<WhereCont>(cont)) {
        auto whereCont = llvm::dyn_cast<WhereCont>(cont);

        instantiateExpr(whereCont->condition);
    }
}

void gulc::TemplateInstHelper::instantiateType(gulc::Type*& type) const {
    if (llvm::isa<DimensionType>(type)) {
        auto dimensionType = llvm::dyn_cast<DimensionType>(type);

        instantiateType(dimensionType->nestedType);
    } else if (llvm::isa<PointerType>(type)) {
        auto pointerType = llvm::dyn_cast<PointerType>(type);

        instantiateType(pointerType->nestedType);
    } else if (llvm::isa<ReferenceType>(type)) {
        auto referenceType = llvm::dyn_cast<ReferenceType>(type);

        instantiateType(referenceType->nestedType);
    } else if (llvm::isa<TemplatedType>(type)) {
        auto templatedType = llvm::dyn_cast<TemplatedType>(type);

        for (Expr*& templateArgument : templatedType->templateArguments()) {
            instantiateExpr(templateArgument);
        }
    } else if (llvm::isa<TemplateTypenameRefType>(type)) {
        auto templateTypenameRefType = llvm::dyn_cast<TemplateTypenameRefType>(type);

        for (std::size_t i = 0; i < templateParameters->size(); ++i) {
            if (templateTypenameRefType->refTemplateParameter() == (*templateParameters)[i]) {
                if (!llvm::isa<TypeExpr>((*templateArguments)[i])) {
                    std::cerr << "INTERNAL ERROR: Expected `TypeExpr` in `TemplateInstHelper::instantiateType`!" << std::endl;
                    std::exit(1);
                }

                // When we find the matching parameter we replace it with a deep copy of the type argument for that
                // index
                auto typeExpr = llvm::dyn_cast<TypeExpr>((*templateArguments)[i]);
                Type* newType = typeExpr->type->deepCopy();
                delete type;
                type = newType;
            }
        }

        // NOTE: If the template parameter wasn't found that is ok. The parameter could be from a nested template that
        //       currently hasn't been instantiated itself...
    }
}

void gulc::TemplateInstHelper::instantiateDecl(gulc::Decl* decl) const {
    for (Attr* attribute : decl->attributes()) {
        instantiateAttr(attribute);
    }

    switch (decl->getDeclKind()) {
        case Decl::Kind::Constructor:
            instantiateConstructorDecl(llvm::dyn_cast<ConstructorDecl>(decl));
            break;
        case Decl::Kind::Destructor:
            instantiateDestructorDecl(llvm::dyn_cast<DestructorDecl>(decl));
            break;
        case Decl::Kind::Function:
            instantiateFunctionDecl(llvm::dyn_cast<FunctionDecl>(decl));
            break;
        case Decl::Kind::Parameter:
            instantiateParameterDecl(llvm::dyn_cast<ParameterDecl>(decl));
            break;
        case Decl::Kind::Struct:
            instantiateStructDecl(llvm::dyn_cast<StructDecl>(decl));
            break;
        case Decl::Kind::TemplateFunction:
            instantiateTemplateFunctionDecl(llvm::dyn_cast<TemplateFunctionDecl>(decl));
            break;
        case Decl::Kind::TemplateParameter:
            instantiateTemplateParameterDecl(llvm::dyn_cast<TemplateParameterDecl>(decl));
            break;
        case Decl::Kind::TemplateStruct:
            instantiateTemplateStructDecl(llvm::dyn_cast<TemplateStructDecl>(decl));
            break;
        case Decl::Kind::TemplateStructInst:
            instantiateTemplateStructInstDecl(llvm::dyn_cast<TemplateStructInstDecl>(decl));
            break;
        case Decl::Kind::Variable:
            instantiateVariableDecl(llvm::dyn_cast<VariableDecl>(decl));
            break;
        default:
            break;
    }
}

void gulc::TemplateInstHelper::instantiateStmt(gulc::Stmt* stmt) const {
    switch (stmt->getStmtKind()) {
        case Stmt::Kind::Case:
            instantiateCaseStmt(llvm::dyn_cast<CaseStmt>(stmt));
            break;
        case Stmt::Kind::Catch:
            instantiateCatchStmt(llvm::dyn_cast<CatchStmt>(stmt));
            break;
        case Stmt::Kind::Compound:
            instantiateCompoundStmt(llvm::dyn_cast<CompoundStmt>(stmt));
            break;
        case Stmt::Kind::Do:
            instantiateDoStmt(llvm::dyn_cast<DoStmt>(stmt));
            break;
        case Stmt::Kind::For:
            instantiateForStmt(llvm::dyn_cast<ForStmt>(stmt));
            break;
        case Stmt::Kind::If:
            instantiateIfStmt(llvm::dyn_cast<IfStmt>(stmt));
            break;
        case Stmt::Kind::Labeled:
            instantiateLabeledStmt(llvm::dyn_cast<LabeledStmt>(stmt));
            break;
        case Stmt::Kind::Return:
            instantiateReturnStmt(llvm::dyn_cast<ReturnStmt>(stmt));
            break;
        case Stmt::Kind::Switch:
            instantiateSwitchStmt(llvm::dyn_cast<SwitchStmt>(stmt));
            break;
        case Stmt::Kind::Try:
            instantiateTryStmt(llvm::dyn_cast<TryStmt>(stmt));
            break;
        case Stmt::Kind::While:
            instantiateWhileStmt((llvm::dyn_cast<WhileStmt>(stmt)));
            break;
        case Stmt::Kind::Expr:
            instantiateExpr(llvm::dyn_cast<Expr>(stmt));
            break;
        default:
            break;
    }
}

void gulc::TemplateInstHelper::instantiateExpr(gulc::Expr* expr) const {
    switch (expr->getExprKind()) {
        case Expr::Kind::ArrayLiteral:
            instantiateArrayLiteralExpr(llvm::dyn_cast<ArrayLiteralExpr>(expr));
            break;
        case Expr::Kind::As:
            instantiateAsExpr(llvm::dyn_cast<AsExpr>(expr));
            break;
        case Expr::Kind::AssignmentOperator:
            instantiateAssignmentOperatorExpr(llvm::dyn_cast<AssignmentOperatorExpr>(expr));
            break;
        case Expr::Kind::FunctionCall:
            instantiateFunctionCallExpr(llvm::dyn_cast<FunctionCallExpr>(expr));
            break;
        case Expr::Kind::Has:
            instantiateHasExpr(llvm::dyn_cast<HasExpr>(expr));
            break;
        case Expr::Kind::Identifier:
            instantiateIdentifierExpr(llvm::dyn_cast<IdentifierExpr>(expr));
            break;
        case Expr::Kind::IndexerCall:
            instantiateIndexerCallExpr(llvm::dyn_cast<IndexerCallExpr>(expr));
            break;
        case Expr::Kind::InfixOperator:
            instantiateInfixOperatorExpr(llvm::dyn_cast<InfixOperatorExpr>(expr));
            break;
        case Expr::Kind::Is:
            instantiateIsExpr(llvm::dyn_cast<IsExpr>(expr));
            break;
        case Expr::Kind::LabeledArgument:
            instantiateLabeledArgumentExpr(llvm::dyn_cast<LabeledArgumentExpr>(expr));
            break;
        case Expr::Kind::MemberAccessCall:
            instantiateMemberAccessCallExpr(llvm::dyn_cast<MemberAccessCallExpr>(expr));
            break;
        case Expr::Kind::Paren:
            instantiateParenExpr(llvm::dyn_cast<ParenExpr>(expr));
            break;
        case Expr::Kind::PostfixOperator:
            instantiatePostfixOperatorExpr(llvm::dyn_cast<PostfixOperatorExpr>(expr));
            break;
        case Expr::Kind::PrefixOperator:
            instantiatePrefixOperatorExpr(llvm::dyn_cast<PrefixOperatorExpr>(expr));
            break;
        case Expr::Kind::Ternary:
            instantiateTernaryExpr(llvm::dyn_cast<TernaryExpr>(expr));
            break;
        case Expr::Kind::Type:
            instantiateTypeExpr(llvm::dyn_cast<TypeExpr>(expr));
            break;
        case Expr::Kind::VariableDecl:
            instantiateVariableDeclExpr(llvm::dyn_cast<VariableDeclExpr>(expr));
            break;
        default:
            break;
    }
}

void gulc::TemplateInstHelper::instantiateConstructorDecl(gulc::ConstructorDecl* constructorDecl) const {
    instantiateFunctionDecl(constructorDecl);
}

void gulc::TemplateInstHelper::instantiateDestructorDecl(gulc::DestructorDecl* destructorDecl) const {
    instantiateFunctionDecl(destructorDecl);
}

void gulc::TemplateInstHelper::instantiateFunctionDecl(gulc::FunctionDecl* functionDecl) const {
    for (ParameterDecl* parameter : functionDecl->parameters()) {
        // We call `instantiateDecl` so the attributes are processed
        instantiateDecl(parameter);
    }

    if (functionDecl->returnType != nullptr) {
        instantiateType(functionDecl->returnType);
    }

    for (Cont* contract : functionDecl->contracts()) {
        instantiateCont(contract);
    }

    if (processBodyStmts) {
        instantiateStmt(functionDecl->body());
    }
}

void gulc::TemplateInstHelper::instantiateParameterDecl(gulc::ParameterDecl* parameterDecl) const {
    instantiateType(parameterDecl->type);

    if (parameterDecl->defaultValue != nullptr) {
        instantiateExpr(parameterDecl->defaultValue);
    }
}

void gulc::TemplateInstHelper::instantiateStructDecl(gulc::StructDecl* structDecl) const {
    for (Cont* contract : structDecl->contracts()) {
        instantiateCont(contract);
    }

    for (Type*& inheritedType : structDecl->inheritedTypes()) {
        instantiateType(inheritedType);
    }

    for (Decl* ownedMember : structDecl->ownedMembers()) {
        instantiateDecl(ownedMember);
    }

    for (ConstructorDecl* constructor : structDecl->constructors()) {
        instantiateDecl(constructor);
    }

    if (structDecl->destructor) {
        instantiateDecl(structDecl->destructor);
    }
}

void gulc::TemplateInstHelper::instantiateTemplateFunctionDecl(gulc::TemplateFunctionDecl* templateFunctionDecl) const {
    for (TemplateParameterDecl* templateParameter : templateFunctionDecl->templateParameters()) {
        instantiateTemplateParameterDecl(templateParameter);
    }

    instantiateFunctionDecl(templateFunctionDecl);
}

void gulc::TemplateInstHelper::instantiateTemplateParameterDecl(
        gulc::TemplateParameterDecl* templateParameterDecl) const {
    if (templateParameterDecl->templateParameterKind() == TemplateParameterDecl::TemplateParameterKind::Const) {
        instantiateType(templateParameterDecl->constType);

        // TODO: Handle default value
    } else {
        // TODO: Can `typename`s have default values??
    }
}

void gulc::TemplateInstHelper::instantiateTemplateStructDecl(gulc::TemplateStructDecl* templateStructDecl) const {
    for (TemplateParameterDecl* templateParameter : templateStructDecl->templateParameters()) {
        instantiateTemplateParameterDecl(templateParameter);
    }

    instantiateStructDecl(templateStructDecl);
}

void gulc::TemplateInstHelper::instantiateTemplateStructInstDecl(
        gulc::TemplateStructInstDecl* templateStructInstDecl) const {
    for (Expr*& templateArgument : templateStructInstDecl->templateArguments()) {
        instantiateExpr(templateArgument);
    }

    instantiateStructDecl(templateStructInstDecl);
}

void gulc::TemplateInstHelper::instantiateVariableDecl(gulc::VariableDecl* variableDecl) const {
    instantiateType(variableDecl->type);

    if (variableDecl->initialValue != nullptr) {
        instantiateExpr(variableDecl->initialValue);
    }
}

void gulc::TemplateInstHelper::instantiateCaseStmt(gulc::CaseStmt* caseStmt) const {
    instantiateExpr(caseStmt->condition);
    instantiateStmt(caseStmt->trueStmt);
}

void gulc::TemplateInstHelper::instantiateCatchStmt(gulc::CatchStmt* catchStmt) const {
    if (catchStmt->hasExceptionType()) {
        instantiateType(catchStmt->exceptionType);
    }

    instantiateStmt(catchStmt->body());
}

void gulc::TemplateInstHelper::instantiateCompoundStmt(gulc::CompoundStmt* compoundStmt) const {
    for (Stmt* statement : compoundStmt->statements) {
        instantiateStmt(statement);
    }
}

void gulc::TemplateInstHelper::instantiateDoStmt(gulc::DoStmt* doStmt) const {
    instantiateStmt(doStmt->body());
    instantiateExpr(doStmt->condition);
}

void gulc::TemplateInstHelper::instantiateForStmt(gulc::ForStmt* forStmt) const {
    if (forStmt->init != nullptr) {
        instantiateExpr(forStmt->init);
    }

    if (forStmt->condition != nullptr) {
        instantiateExpr(forStmt->condition);
    }

    if (forStmt->iteration != nullptr) {
        instantiateExpr(forStmt->iteration);
    }

    instantiateStmt(forStmt->body());
}

void gulc::TemplateInstHelper::instantiateIfStmt(gulc::IfStmt* ifStmt) const {
    instantiateExpr(ifStmt->condition);
    instantiateStmt(ifStmt->trueBody());

    if (ifStmt->hasFalseBody()) {
        instantiateStmt(ifStmt->falseBody());
    }
}

void gulc::TemplateInstHelper::instantiateLabeledStmt(gulc::LabeledStmt* labeledStmt) const {
    instantiateStmt(labeledStmt->labeledStmt);
}

void gulc::TemplateInstHelper::instantiateReturnStmt(gulc::ReturnStmt* returnStmt) const {
    if (returnStmt->returnValue != nullptr) {
        instantiateExpr(returnStmt->returnValue);
    }
}

void gulc::TemplateInstHelper::instantiateSwitchStmt(gulc::SwitchStmt* switchStmt) const {
    instantiateExpr(switchStmt->condition);

    for (Stmt* statement : switchStmt->statements) {
        instantiateStmt(statement);
    }
}

void gulc::TemplateInstHelper::instantiateTryStmt(gulc::TryStmt* tryStmt) const {
    instantiateStmt(tryStmt->body());

    for (CatchStmt* catchStmt : tryStmt->catchStatements()) {
        instantiateStmt(catchStmt);
    }

    if (tryStmt->hasFinallyStatement()) {
        instantiateStmt(tryStmt->finallyStatement());
    }
}

void gulc::TemplateInstHelper::instantiateWhileStmt(gulc::WhileStmt* whileStmt) const {
    instantiateExpr(whileStmt->condition);
    instantiateStmt(whileStmt->body());
}

void gulc::TemplateInstHelper::instantiateArrayLiteralExpr(gulc::ArrayLiteralExpr* arrayLiteralExpr) const {
    for (Expr* index : arrayLiteralExpr->indexes) {
        instantiateExpr(index);
    }
}

void gulc::TemplateInstHelper::instantiateAsExpr(gulc::AsExpr* asExpr) const {
    instantiateExpr(asExpr->expr);
    instantiateType(asExpr->asType);
}

void gulc::TemplateInstHelper::instantiateAssignmentOperatorExpr(
        gulc::AssignmentOperatorExpr* assignmentOperatorExpr) const {
    instantiateExpr(assignmentOperatorExpr->leftValue);
    instantiateExpr(assignmentOperatorExpr->rightValue);
}

void gulc::TemplateInstHelper::instantiateFunctionCallExpr(gulc::FunctionCallExpr* functionCallExpr) const {
    instantiateExpr(functionCallExpr->functionReference);

    for (Expr* argument : functionCallExpr->arguments) {
        instantiateExpr(argument);
    }
}

void gulc::TemplateInstHelper::instantiateHasExpr(gulc::HasExpr* hasExpr) const {
    instantiateExpr(hasExpr->expr);
    instantiateType(hasExpr->trait);
}

void gulc::TemplateInstHelper::instantiateIdentifierExpr(gulc::IdentifierExpr* identifierExpr) const {
    for (Expr* templateArgument : identifierExpr->templateArguments()) {
        instantiateExpr(templateArgument);
    }
}

void gulc::TemplateInstHelper::instantiateIndexerCallExpr(gulc::IndexerCallExpr* indexerCallExpr) const {
    instantiateExpr(indexerCallExpr->indexerReference);

    for (Expr* argument : indexerCallExpr->arguments) {
        instantiateExpr(argument);
    }
}

void gulc::TemplateInstHelper::instantiateInfixOperatorExpr(gulc::InfixOperatorExpr* infixOperatorExpr) const {
    instantiateExpr(infixOperatorExpr->leftValue);
    instantiateExpr(infixOperatorExpr->rightValue);
}

void gulc::TemplateInstHelper::instantiateIsExpr(gulc::IsExpr* isExpr) const {
    instantiateExpr(isExpr->expr);
    instantiateType(isExpr->isType);
}

void gulc::TemplateInstHelper::instantiateLabeledArgumentExpr(gulc::LabeledArgumentExpr* labeledArgumentExpr) const {
    instantiateExpr(labeledArgumentExpr->argument);
}

void gulc::TemplateInstHelper::instantiateMemberAccessCallExpr(gulc::MemberAccessCallExpr* memberAccessCallExpr) const {
    instantiateExpr(memberAccessCallExpr->objectRef);
    instantiateExpr(memberAccessCallExpr->member);
}

void gulc::TemplateInstHelper::instantiateParenExpr(gulc::ParenExpr* parenExpr) const {
    instantiateExpr(parenExpr->nestedExpr);
}

void gulc::TemplateInstHelper::instantiatePostfixOperatorExpr(gulc::PostfixOperatorExpr* postfixOperatorExpr) const {
    instantiateExpr(postfixOperatorExpr->nestedExpr);
}

void gulc::TemplateInstHelper::instantiatePrefixOperatorExpr(gulc::PrefixOperatorExpr* prefixOperatorExpr) const {
    instantiateExpr(prefixOperatorExpr->nestedExpr);
}

void gulc::TemplateInstHelper::instantiateTernaryExpr(gulc::TernaryExpr* ternaryExpr) const {
    instantiateExpr(ternaryExpr->condition);
    instantiateExpr(ternaryExpr->trueExpr);
    instantiateExpr(ternaryExpr->falseExpr);
}

void gulc::TemplateInstHelper::instantiateTypeExpr(gulc::TypeExpr* typeExpr) const {
    instantiateType(typeExpr->type);
}

void gulc::TemplateInstHelper::instantiateVariableDeclExpr(gulc::VariableDeclExpr* variableDeclExpr) const {
    if (variableDeclExpr->type != nullptr) {
        instantiateType(variableDeclExpr->type);
    }

    if (variableDeclExpr->initialValue != nullptr) {
        instantiateExpr(variableDeclExpr->initialValue);
    }
}
