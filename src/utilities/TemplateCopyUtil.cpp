#include <ast/conts/EnsuresCont.hpp>
#include <ast/conts/RequiresCont.hpp>
#include <ast/conts/WhereCont.hpp>
#include <ast/types/DimensionType.hpp>
#include <ast/types/PointerType.hpp>
#include <ast/types/ReferenceType.hpp>
#include <ast/types/TemplatedType.hpp>
#include <ast/types/TemplateTypenameRefType.hpp>
#include <ast/types/AliasType.hpp>
#include <ast/types/FlatArrayType.hpp>
#include <ast/types/UnresolvedNestedType.hpp>
#include <ast/types/TemplateStructType.hpp>
#include <ast/types/TemplateTraitType.hpp>
#include <ast/types/DependentType.hpp>
#include "TemplateCopyUtil.hpp"

void gulc::TemplateCopyUtil::instantiateTemplateStructCopy(
        std::vector<TemplateParameterDecl*> const* oldTemplateParameters,
        TemplateStructDecl* templateStruct) {
    this->oldTemplateParameters = oldTemplateParameters;
    this->newTemplateParameters = &templateStruct->templateParameters();

    for (Attr* attribute : templateStruct->attributes()) {
        instantiateAttr(attribute);
    }

    for (Cont* contract : templateStruct->contracts()) {
        instantiateCont(contract);
    }

    for (Type*& inheritedType : templateStruct->inheritedTypes()) {
        instantiateType(inheritedType);
    }

    for (Decl* ownedMember : templateStruct->ownedMembers()) {
        instantiateDecl(ownedMember);
    }

    for (ConstructorDecl* constructor : templateStruct->constructors()) {
        instantiateDecl(constructor);
    }

    if (templateStruct->destructor != nullptr) {
        instantiateDecl(templateStruct->destructor);
    }
}

void gulc::TemplateCopyUtil::instantiateTemplateTraitCopy(
        std::vector<TemplateParameterDecl*> const* oldTemplateParameters,
        TemplateTraitDecl* templateTrait) {
    this->oldTemplateParameters = oldTemplateParameters;
    this->newTemplateParameters = &templateTrait->templateParameters();

    for (Attr* attribute : templateTrait->attributes()) {
        instantiateAttr(attribute);
    }

    for (Cont* contract : templateTrait->contracts()) {
        instantiateCont(contract);
    }

    for (Type*& inheritedType : templateTrait->inheritedTypes()) {
        instantiateType(inheritedType);
    }

    for (Decl* ownedMember : templateTrait->ownedMembers()) {
        instantiateDecl(ownedMember);
    }
}

void gulc::TemplateCopyUtil::instantiateTemplateFunctionCopy(
        std::vector<TemplateParameterDecl*> const* oldTemplateParameters,
        gulc::TemplateFunctionDecl* templateFunction) {
    this->oldTemplateParameters = oldTemplateParameters;
    this->newTemplateParameters = &templateFunction->templateParameters();

    for (Attr* attribute : templateFunction->attributes()) {
        instantiateAttr(attribute);
    }

    for (Cont* contract : templateFunction->contracts()) {
        instantiateCont(contract);
    }

    for (ParameterDecl* parameter : templateFunction->parameters()) {
        instantiateParameterDecl(parameter);
    }

    if (templateFunction->returnType != nullptr) {
        instantiateType(templateFunction->returnType);
    }

    // NOTE: Template functions CANNOT be `extern` so we don't check if it is.
    instantiateStmt(templateFunction->body());
}

void gulc::TemplateCopyUtil::instantiateAttr(gulc::Attr* attr) const {
    // TODO: There currently isn't anything to do here...
}

void gulc::TemplateCopyUtil::instantiateCont(gulc::Cont* cont) const {
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

void gulc::TemplateCopyUtil::instantiateType(gulc::Type*& type) const {
    if (llvm::isa<DependentType>(type)) {
        auto dependentType = llvm::dyn_cast<DependentType>(type);

        instantiateType(dependentType->container);
        instantiateType(dependentType->dependent);
    } else if (llvm::isa<DimensionType>(type)) {
        auto dimensionType = llvm::dyn_cast<DimensionType>(type);

        instantiateType(dimensionType->nestedType);
    } else if (llvm::isa<FlatArrayType>(type)) {
        auto flatArrayType = llvm::dyn_cast<FlatArrayType>(type);

        instantiateType(flatArrayType->indexType);
        instantiateExpr(flatArrayType->length);
    } else if (llvm::isa<UnresolvedNestedType>(type)) {
        auto nestedType = llvm::dyn_cast<UnresolvedNestedType>(type);

        instantiateType(nestedType->container);

        for (Expr*& templateArgument : nestedType->templateArguments()) {
            instantiateExpr(templateArgument);
        }
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
    } else if (llvm::isa<TemplateStructType>(type)) {
        auto templateStructType = llvm::dyn_cast<TemplateStructType>(type);

        for (Expr*& templateArgument : templateStructType->templateArguments()) {
            instantiateExpr(templateArgument);
        }
    } else if (llvm::isa<TemplateTraitType>(type)) {
        auto templateTraitType = llvm::dyn_cast<TemplateTraitType>(type);

        for (Expr*& templateArgument : templateTraitType->templateArguments()) {
            instantiateExpr(templateArgument);
        }
    } else if (llvm::isa<TemplateTypenameRefType>(type)) {
        auto templateTypenameRefType = llvm::dyn_cast<TemplateTypenameRefType>(type);

        for (std::size_t i = 0; i < oldTemplateParameters->size(); ++i) {
            if (templateTypenameRefType->refTemplateParameter() == (*oldTemplateParameters)[i]) {
                // Replace the template parameter ref with the new template parameter ref
                auto newTemplateTypenameRefType = new TemplateTypenameRefType(type->qualifier(),
                                                                              (*newTemplateParameters)[i],
                                                                              type->startPosition(),
                                                                              type->endPosition());
                delete type;
                type = newTemplateTypenameRefType;
                break;
            }
        }

        // NOTE: If the template parameter wasn't found that is ok. The parameter could be from a nested template that
        //       currently hasn't been instantiated itself...
    }
}

void gulc::TemplateCopyUtil::instantiateDecl(gulc::Decl* decl) const {
    for (Attr* attribute : decl->attributes()) {
        instantiateAttr(attribute);
    }

    switch (decl->getDeclKind()) {
        case Decl::Kind::CallOperator:
            instantiateCallOperatorDecl(llvm::dyn_cast<CallOperatorDecl>(decl));
            break;
        case Decl::Kind::Constructor:
            instantiateConstructorDecl(llvm::dyn_cast<ConstructorDecl>(decl));
            break;
        case Decl::Kind::Destructor:
            instantiateDestructorDecl(llvm::dyn_cast<DestructorDecl>(decl));
            break;
        case Decl::Kind::Enum:
            instantiateEnumDecl(llvm::dyn_cast<EnumDecl>(decl));
            break;
        case Decl::Kind::Function:
            instantiateFunctionDecl(llvm::dyn_cast<FunctionDecl>(decl));
            break;
        case Decl::Kind::Operator:
            instantiateOperatorDecl(llvm::dyn_cast<OperatorDecl>(decl));
            break;
        case Decl::Kind::Parameter:
            instantiateParameterDecl(llvm::dyn_cast<ParameterDecl>(decl));
            break;
        case Decl::Kind::Property:
            instantiatePropertyDecl(llvm::dyn_cast<PropertyDecl>(decl));
            break;
        case Decl::Kind::PropertyGet:
            instantiatePropertyGetDecl(llvm::dyn_cast<PropertyGetDecl>(decl));
            break;
        case Decl::Kind::PropertySet:
            instantiatePropertySetDecl(llvm::dyn_cast<PropertySetDecl>(decl));
            break;
        case Decl::Kind::Struct:
            instantiateStructDecl(llvm::dyn_cast<StructDecl>(decl));
            break;
        case Decl::Kind::SubscriptOperator:
            instantiateSubscriptOperatorDecl(llvm::dyn_cast<SubscriptOperatorDecl>(decl));
            break;
        case Decl::Kind::SubscriptOperatorGet:
            instantiateSubscriptOperatorGetDecl(llvm::dyn_cast<SubscriptOperatorGetDecl>(decl));
            break;
        case Decl::Kind::SubscriptOperatorSet:
            instantiateSubscriptOperatorSetDecl(llvm::dyn_cast<SubscriptOperatorSetDecl>(decl));
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
        case Decl::Kind::TemplateTrait:
            instantiateTemplateTraitDecl(llvm::dyn_cast<TemplateTraitDecl>(decl));
            break;
        case Decl::Kind::TemplateTraitInst:
            instantiateTemplateTraitInstDecl(llvm::dyn_cast<TemplateTraitInstDecl>(decl));
            break;
        case Decl::Kind::Trait:
            instantiateTraitDecl(llvm::dyn_cast<TraitDecl>(decl));
            break;
        case Decl::Kind::TypeAlias:
            instantiateTypeAliasDecl(llvm::dyn_cast<TypeAliasDecl>(decl));
            break;
        case Decl::Kind::Variable:
            instantiateVariableDecl(llvm::dyn_cast<VariableDecl>(decl));
            break;
        default:
            std::cerr << "[INTERNAL ERROR] unhandled `Decl` found in `TemplateCopyUtil`!" << std::endl;
            break;
    }
}

void gulc::TemplateCopyUtil::instantiateStmt(gulc::Stmt* stmt) const {
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
            std::cerr << "[INTERNAL ERROR] unhandled `Stmt` found in `TemplateCopyUtil`!" << std::endl;
            break;
    }
}

void gulc::TemplateCopyUtil::instantiateExpr(gulc::Expr* expr) const {
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
        case Expr::Kind::SubscriptCall:
            instantiateSubscriptCallExpr(llvm::dyn_cast<SubscriptCallExpr>(expr));
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
            std::cerr << "[INTERNAL ERROR] unhandled `Expr` found in `TemplateCopyUtil`!" << std::endl;
            break;
    }
}

void gulc::TemplateCopyUtil::instantiateCallOperatorDecl(gulc::CallOperatorDecl* callOperatorDecl) const {
    instantiateFunctionDecl(callOperatorDecl);
}

void gulc::TemplateCopyUtil::instantiateConstructorDecl(gulc::ConstructorDecl* constructorDecl) const {
    instantiateFunctionDecl(constructorDecl);
}

void gulc::TemplateCopyUtil::instantiateDestructorDecl(gulc::DestructorDecl* destructorDecl) const {
    instantiateFunctionDecl(destructorDecl);
}

void gulc::TemplateCopyUtil::instantiateEnumDecl(gulc::EnumDecl* enumDecl) const {
    if (enumDecl->constType != nullptr) {
        instantiateType(enumDecl->constType);
    }

    for (EnumConstDecl* enumConst : enumDecl->enumConsts()) {
        if (enumConst->constValue != nullptr) {
            instantiateExpr(enumConst->constValue);
        }
    }
}

void gulc::TemplateCopyUtil::instantiateFunctionDecl(gulc::FunctionDecl* functionDecl) const {
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

    instantiateStmt(functionDecl->body());
}

void gulc::TemplateCopyUtil::instantiateOperatorDecl(gulc::OperatorDecl* operatorDecl) const {
    instantiateFunctionDecl(operatorDecl);
}

void gulc::TemplateCopyUtil::instantiateParameterDecl(gulc::ParameterDecl* parameterDecl) const {
    instantiateType(parameterDecl->type);

    if (parameterDecl->defaultValue != nullptr) {
        instantiateExpr(parameterDecl->defaultValue);
    }
}

void gulc::TemplateCopyUtil::instantiatePropertyDecl(gulc::PropertyDecl* propertyDecl) const {
    instantiateType(propertyDecl->type);

    for (PropertyGetDecl* getter : propertyDecl->getters()) {
        instantiateDecl(getter);
    }

    if (propertyDecl->hasSetter()) {
        instantiateDecl(propertyDecl->setter());
    }
}

void gulc::TemplateCopyUtil::instantiatePropertyGetDecl(gulc::PropertyGetDecl* propertyGetDecl) const {
    instantiateFunctionDecl(propertyGetDecl);
}

void gulc::TemplateCopyUtil::instantiatePropertySetDecl(gulc::PropertySetDecl* propertySetDecl) const {
    instantiateFunctionDecl(propertySetDecl);
}

void gulc::TemplateCopyUtil::instantiateStructDecl(gulc::StructDecl* structDecl) const {
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

void gulc::TemplateCopyUtil::instantiateSubscriptOperatorDecl(gulc::SubscriptOperatorDecl* subscriptOperatorDecl) const {
    for (ParameterDecl* parameter : subscriptOperatorDecl->parameters()) {
        instantiateParameterDecl(parameter);
    }

    for (SubscriptOperatorGetDecl* getter : subscriptOperatorDecl->getters()) {
        instantiateDecl(getter);
    }

    if (subscriptOperatorDecl->hasSetter()) {
        instantiateDecl(subscriptOperatorDecl->setter());
    }
}

void gulc::TemplateCopyUtil::instantiateSubscriptOperatorGetDecl(gulc::SubscriptOperatorGetDecl* subscriptOperatorGetDecl) const {
    instantiateFunctionDecl(subscriptOperatorGetDecl);
}

void gulc::TemplateCopyUtil::instantiateSubscriptOperatorSetDecl(gulc::SubscriptOperatorSetDecl* subscriptOperatorSetDecl) const {
    instantiateFunctionDecl(subscriptOperatorSetDecl);
}

void gulc::TemplateCopyUtil::instantiateTemplateFunctionDecl(gulc::TemplateFunctionDecl* templateFunctionDecl) const {
    for (TemplateParameterDecl* templateParameter : templateFunctionDecl->templateParameters()) {
        instantiateTemplateParameterDecl(templateParameter);
    }

    instantiateFunctionDecl(templateFunctionDecl);
}

void gulc::TemplateCopyUtil::instantiateTemplateParameterDecl(gulc::TemplateParameterDecl* templateParameterDecl) const {
    if (templateParameterDecl->templateParameterKind() == TemplateParameterDecl::TemplateParameterKind::Const) {
        instantiateType(templateParameterDecl->constType);

        // TODO: Handle default value
    } else {
        for (Type*& inheritedType : templateParameterDecl->inheritedTypes) {
            instantiateType(inheritedType);
        }

        // TODO: Can `typename`s have default values??
    }
}

void gulc::TemplateCopyUtil::instantiateTemplateStructDecl(gulc::TemplateStructDecl* templateStructDecl) const {
    for (TemplateParameterDecl* templateParameter : templateStructDecl->templateParameters()) {
        instantiateTemplateParameterDecl(templateParameter);
    }

    instantiateStructDecl(templateStructDecl);
}

void gulc::TemplateCopyUtil::instantiateTemplateStructInstDecl(gulc::TemplateStructInstDecl* templateStructInstDecl) const {
    for (Expr*& templateArgument : templateStructInstDecl->templateArguments()) {
        instantiateExpr(templateArgument);
    }

    instantiateStructDecl(templateStructInstDecl);
}

void gulc::TemplateCopyUtil::instantiateTemplateTraitDecl(gulc::TemplateTraitDecl* templateTraitDecl) const {
    for (TemplateParameterDecl* templateParameter : templateTraitDecl->templateParameters()) {
        instantiateTemplateParameterDecl(templateParameter);
    }

    instantiateTraitDecl(templateTraitDecl);
}

void gulc::TemplateCopyUtil::instantiateTemplateTraitInstDecl(gulc::TemplateTraitInstDecl* templateTraitInstDecl) const {
    for (Expr*& templateArgument : templateTraitInstDecl->templateArguments()) {
        instantiateExpr(templateArgument);
    }

    instantiateTraitDecl(templateTraitInstDecl);
}

void gulc::TemplateCopyUtil::instantiateTraitDecl(gulc::TraitDecl* traitDecl) const {
    for (Cont* contract : traitDecl->contracts()) {
        instantiateCont(contract);
    }

    for (Type*& inheritedType : traitDecl->inheritedTypes()) {
        instantiateType(inheritedType);
    }

    for (Decl* ownedMember : traitDecl->ownedMembers()) {
        instantiateDecl(ownedMember);
    }
}

void gulc::TemplateCopyUtil::instantiateTypeAliasDecl(gulc::TypeAliasDecl* typeAliasDecl) const {
    for (TemplateParameterDecl* templateParameter : typeAliasDecl->templateParameters()) {
        instantiateTemplateParameterDecl(templateParameter);
    }

    instantiateType(typeAliasDecl->typeValue);
}

void gulc::TemplateCopyUtil::instantiateVariableDecl(gulc::VariableDecl* variableDecl) const {
    instantiateType(variableDecl->type);

    if (variableDecl->initialValue != nullptr) {
        instantiateExpr(variableDecl->initialValue);
    }
}

void gulc::TemplateCopyUtil::instantiateCaseStmt(gulc::CaseStmt* caseStmt) const {
    instantiateExpr(caseStmt->condition);
    instantiateStmt(caseStmt->trueStmt);
}

void gulc::TemplateCopyUtil::instantiateCatchStmt(gulc::CatchStmt* catchStmt) const {
    if (catchStmt->hasExceptionType()) {
        instantiateType(catchStmt->exceptionType);
    }

    instantiateStmt(catchStmt->body());
}

void gulc::TemplateCopyUtil::instantiateCompoundStmt(gulc::CompoundStmt* compoundStmt) const {
    for (Stmt* statement : compoundStmt->statements) {
        instantiateStmt(statement);
    }
}

void gulc::TemplateCopyUtil::instantiateDoStmt(gulc::DoStmt* doStmt) const {
    instantiateStmt(doStmt->body());
    instantiateExpr(doStmt->condition);
}

void gulc::TemplateCopyUtil::instantiateForStmt(gulc::ForStmt* forStmt) const {
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

void gulc::TemplateCopyUtil::instantiateIfStmt(gulc::IfStmt* ifStmt) const {
    instantiateExpr(ifStmt->condition);
    instantiateStmt(ifStmt->trueBody());

    if (ifStmt->hasFalseBody()) {
        instantiateStmt(ifStmt->falseBody());
    }
}

void gulc::TemplateCopyUtil::instantiateLabeledStmt(gulc::LabeledStmt* labeledStmt) const {
    instantiateStmt(labeledStmt->labeledStmt);
}

void gulc::TemplateCopyUtil::instantiateReturnStmt(gulc::ReturnStmt* returnStmt) const {
    if (returnStmt->returnValue != nullptr) {
        instantiateExpr(returnStmt->returnValue);
    }
}

void gulc::TemplateCopyUtil::instantiateSwitchStmt(gulc::SwitchStmt* switchStmt) const {
    instantiateExpr(switchStmt->condition);

    for (Stmt* statement : switchStmt->statements) {
        instantiateStmt(statement);
    }
}

void gulc::TemplateCopyUtil::instantiateTryStmt(gulc::TryStmt* tryStmt) const {
    instantiateStmt(tryStmt->body());

    for (CatchStmt* catchStmt : tryStmt->catchStatements()) {
        instantiateStmt(catchStmt);
    }

    if (tryStmt->hasFinallyStatement()) {
        instantiateStmt(tryStmt->finallyStatement());
    }
}

void gulc::TemplateCopyUtil::instantiateWhileStmt(gulc::WhileStmt* whileStmt) const {
    instantiateExpr(whileStmt->condition);
    instantiateStmt(whileStmt->body());
}

void gulc::TemplateCopyUtil::instantiateArrayLiteralExpr(gulc::ArrayLiteralExpr* arrayLiteralExpr) const {
    for (Expr* index : arrayLiteralExpr->indexes) {
        instantiateExpr(index);
    }
}

void gulc::TemplateCopyUtil::instantiateAsExpr(gulc::AsExpr* asExpr) const {
    instantiateExpr(asExpr->expr);
    instantiateType(asExpr->asType);
}

void gulc::TemplateCopyUtil::instantiateAssignmentOperatorExpr(
        gulc::AssignmentOperatorExpr* assignmentOperatorExpr) const {
    instantiateExpr(assignmentOperatorExpr->leftValue);
    instantiateExpr(assignmentOperatorExpr->rightValue);
}

void gulc::TemplateCopyUtil::instantiateFunctionCallExpr(gulc::FunctionCallExpr* functionCallExpr) const {
    instantiateExpr(functionCallExpr->functionReference);

    for (Expr* argument : functionCallExpr->arguments) {
        instantiateExpr(argument);
    }
}

void gulc::TemplateCopyUtil::instantiateHasExpr(gulc::HasExpr* hasExpr) const {
    instantiateExpr(hasExpr->expr);
    instantiateType(hasExpr->trait);
}

void gulc::TemplateCopyUtil::instantiateIdentifierExpr(gulc::IdentifierExpr* identifierExpr) const {
    for (Expr* templateArgument : identifierExpr->templateArguments()) {
        instantiateExpr(templateArgument);
    }
}

void gulc::TemplateCopyUtil::instantiateInfixOperatorExpr(gulc::InfixOperatorExpr* infixOperatorExpr) const {
    instantiateExpr(infixOperatorExpr->leftValue);
    instantiateExpr(infixOperatorExpr->rightValue);
}

void gulc::TemplateCopyUtil::instantiateIsExpr(gulc::IsExpr* isExpr) const {
    instantiateExpr(isExpr->expr);
    instantiateType(isExpr->isType);
}

void gulc::TemplateCopyUtil::instantiateLabeledArgumentExpr(gulc::LabeledArgumentExpr* labeledArgumentExpr) const {
    instantiateExpr(labeledArgumentExpr->argument);
}

void gulc::TemplateCopyUtil::instantiateMemberAccessCallExpr(gulc::MemberAccessCallExpr* memberAccessCallExpr) const {
    instantiateExpr(memberAccessCallExpr->objectRef);
    instantiateExpr(memberAccessCallExpr->member);
}

void gulc::TemplateCopyUtil::instantiateParenExpr(gulc::ParenExpr* parenExpr) const {
    instantiateExpr(parenExpr->nestedExpr);
}

void gulc::TemplateCopyUtil::instantiatePostfixOperatorExpr(gulc::PostfixOperatorExpr* postfixOperatorExpr) const {
    instantiateExpr(postfixOperatorExpr->nestedExpr);
}

void gulc::TemplateCopyUtil::instantiatePrefixOperatorExpr(gulc::PrefixOperatorExpr* prefixOperatorExpr) const {
    instantiateExpr(prefixOperatorExpr->nestedExpr);
}

void gulc::TemplateCopyUtil::instantiateSubscriptCallExpr(gulc::SubscriptCallExpr* subscriptCallExpr) const {
    instantiateExpr(subscriptCallExpr->subscriptReference);

    for (Expr* argument : subscriptCallExpr->arguments) {
        instantiateExpr(argument);
    }
}

void gulc::TemplateCopyUtil::instantiateTernaryExpr(gulc::TernaryExpr* ternaryExpr) const {
    instantiateExpr(ternaryExpr->condition);
    instantiateExpr(ternaryExpr->trueExpr);
    instantiateExpr(ternaryExpr->falseExpr);
}

void gulc::TemplateCopyUtil::instantiateTypeExpr(gulc::TypeExpr* typeExpr) const {
    instantiateType(typeExpr->type);
}

void gulc::TemplateCopyUtil::instantiateVariableDeclExpr(gulc::VariableDeclExpr* variableDeclExpr) const {
    if (variableDeclExpr->type != nullptr) {
        instantiateType(variableDeclExpr->type);
    }

    if (variableDeclExpr->initialValue != nullptr) {
        instantiateExpr(variableDeclExpr->initialValue);
    }
}
