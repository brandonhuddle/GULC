#include <ast/conts/EnsuresCont.hpp>
#include <ast/conts/RequiresCont.hpp>
#include <ast/conts/WhereCont.hpp>
#include <ast/types/DimensionType.hpp>
#include <ast/types/PointerType.hpp>
#include <ast/types/ReferenceType.hpp>
#include <ast/types/TemplatedType.hpp>
#include <ast/types/TemplateTypenameRefType.hpp>
#include "TemplateInstHelper.hpp"
#include <ast/decls/TypeAliasDecl.hpp>
#include <ast/types/FlatArrayType.hpp>
#include <ast/types/UnresolvedNestedType.hpp>
#include <ast/types/TemplateStructType.hpp>
#include <ast/types/TemplateTraitType.hpp>
#include <ast/exprs/TemplateConstRefExpr.hpp>
#include <ast/types/DependentType.hpp>
#include <ast/types/StructType.hpp>
#include <ast/types/TraitType.hpp>

void gulc::TemplateInstHelper::instantiateTemplateStructInstDecl(gulc::TemplateStructDecl* parentTemplateStruct,
                                                                 gulc::TemplateStructInstDecl* templateStructInstDecl,
                                                                 bool processBodyStmts) {
    _processBodyStmts = processBodyStmts;

    _templateParameters = &parentTemplateStruct->templateParameters();
    _templateArguments = &templateStructInstDecl->templateArguments();

    // We instantiate the parameters here just in case any of them are default values that reference other template
    // parameters
    for (Expr* templateArgument : templateStructInstDecl->templateArguments()) {
        instantiateExpr(templateArgument);
    }

    // We need to account for whether the new instantiation is contained within a template or not
    if (templateStructInstDecl->containerTemplateType == nullptr) {
        _currentContainerTemplateType = nullptr;
        templateStructInstDecl->containedInTemplate = false;
    } else {
        std::vector<Expr*> copiedTemplateArguments;
        copiedTemplateArguments.reserve(templateStructInstDecl->templateArguments().size());

        for (Expr* templateArgument : templateStructInstDecl->templateArguments()) {
            copiedTemplateArguments.push_back(templateArgument->deepCopy());
        }

        _currentContainerTemplateType = new DependentType(Type::Qualifier::Unassigned,
                                                          _currentContainerTemplateType,
                                                          new TemplateStructType(Type::Qualifier::Unassigned,
                                                                                 copiedTemplateArguments,
                                                                                 templateStructInstDecl->parentTemplateStruct(),
                                                                                 {}, {}));
    }

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
    _processBodyStmts = processBodyStmts;

    _templateParameters = &parentTemplateTrait->templateParameters();
    _templateArguments = &templateTraitInstDecl->templateArguments();

    // We instantiate the parameters here just in case any of them are default values that reference other template
    // parameters
    for (Expr* templateArgument : templateTraitInstDecl->templateArguments()) {
        instantiateExpr(templateArgument);
    }

    // We need to account for whether the new instantiation is contained within a template or not
    if (templateTraitInstDecl->containerTemplateType == nullptr) {
        _currentContainerTemplateType = nullptr;
        templateTraitInstDecl->containedInTemplate = false;
    } else {
        std::vector<Expr*> copiedTemplateArguments;
        copiedTemplateArguments.reserve(templateTraitInstDecl->templateArguments().size());

        for (Expr* templateArgument : templateTraitInstDecl->templateArguments()) {
            copiedTemplateArguments.push_back(templateArgument->deepCopy());
        }

        _currentContainerTemplateType = new DependentType(Type::Qualifier::Unassigned,
                                                          _currentContainerTemplateType,
                                                          new TemplateTraitType(Type::Qualifier::Unassigned,
                                                                                copiedTemplateArguments,
                                                                                templateTraitInstDecl->parentTemplateTrait(),
                                                                                {}, {}));
    }

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

void gulc::TemplateInstHelper::instantiateTemplateFunctionInstDecl(gulc::TemplateFunctionDecl* parentTemplateFunction,
                                                                   gulc::TemplateFunctionInstDecl* templateFunctionInstDecl) {
    _processBodyStmts = true;

    _templateParameters = &parentTemplateFunction->templateParameters();
    _templateArguments = &templateFunctionInstDecl->templateArguments();

    // We instantiate the parameters here just in case any of them are default values that reference other template
    // parameters
    for (Expr* templateArgument : templateFunctionInstDecl->templateArguments()) {
        instantiateExpr(templateArgument);
    }

    // TODO: Is there anything we need to do differently if this is contained within a template? I don't think it is
    //       possible to reach this point within a template?

    for (Attr* attribute : templateFunctionInstDecl->attributes()) {
        instantiateAttr(attribute);
    }

    for (Cont* contract : templateFunctionInstDecl->contracts()) {
        instantiateCont(contract);
    }

    for (ParameterDecl* parameter : templateFunctionInstDecl->parameters()) {
        instantiateParameterDecl(parameter);
    }

    if (templateFunctionInstDecl->returnType != nullptr) {
        instantiateType(templateFunctionInstDecl->returnType);
    }

    instantiateStmt(templateFunctionInstDecl->body());
}

void gulc::TemplateInstHelper::instantiateType(gulc::Type*& type,
                                               std::vector<TemplateParameterDecl*>* templateParameters,
                                               std::vector<Expr*>* templateArguments) {
    _templateParameters = templateParameters;
    _templateArguments = templateArguments;

    // We instantiate the parameters here just in case any of them are default values that reference other template
    // parameters
    for (Expr* templateArgument : *templateArguments) {
        instantiateExpr(templateArgument);
    }

    instantiateType(type);
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

        for (std::size_t i = 0; i < _templateParameters->size(); ++i) {
            if (templateTypenameRefType->refTemplateParameter() == (*_templateParameters)[i]) {
                if (!llvm::isa<TypeExpr>((*_templateArguments)[i])) {
                    std::cerr << "INTERNAL ERROR: Expected `TypeExpr` in `TemplateInstHelper::instantiateType`!" << std::endl;
                    std::exit(1);
                }

                // When we find the matching parameter we replace it with a deep copy of the type argument for that
                // index
                auto typeExpr = llvm::dyn_cast<TypeExpr>((*_templateArguments)[i]);
                Type* newType = typeExpr->type->deepCopy();
                delete type;
                type = newType;
                break;
            }
        }

        // NOTE: If the template parameter wasn't found that is ok. The parameter could be from a nested template that
        //       currently hasn't been instantiated itself...
    }
}

void gulc::TemplateInstHelper::instantiateDecl(gulc::Decl* decl) {
    decl->containedInTemplate = _currentContainerTemplateType != nullptr;

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
            instantiateStructDecl(llvm::dyn_cast<StructDecl>(decl), true);
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
            instantiateTraitDecl(llvm::dyn_cast<TraitDecl>(decl), true);
            break;
        case Decl::Kind::TypeAlias:
            instantiateTypeAliasDecl(llvm::dyn_cast<TypeAliasDecl>(decl));
            break;
        case Decl::Kind::Variable:
            instantiateVariableDecl(llvm::dyn_cast<VariableDecl>(decl));
            break;
        default:
            std::cerr << "[INTERNAL ERROR] unhandled `Decl` found in `TemplateInstHelper`!" << std::endl;
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
        case Stmt::Kind::DoCatch:
            instantiateDoCatchStmt(llvm::dyn_cast<DoCatchStmt>(stmt));
            break;
        case Stmt::Kind::DoWhile:
            instantiateDoWhileStmt(llvm::dyn_cast<DoWhileStmt>(stmt));
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
        case Stmt::Kind::While:
            instantiateWhileStmt((llvm::dyn_cast<WhileStmt>(stmt)));
            break;
        case Stmt::Kind::Expr:
            instantiateExpr(llvm::dyn_cast<Expr>(stmt));
            break;
        default:
            std::cerr << "[INTERNAL ERROR] unhandled `Stmt` found in `TemplateInstHelper`!" << std::endl;
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
        case Expr::Kind::CheckExtendsType:
            instantiateCheckExtendsTypeExpr(llvm::dyn_cast<CheckExtendsTypeExpr>(expr));
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
            std::cerr << "[INTERNAL ERROR] unhandled `Expr` found in `TemplateInstHelper`!" << std::endl;
            std::exit(1);
            break;
    }
}

void gulc::TemplateInstHelper::instantiateCallOperatorDecl(gulc::CallOperatorDecl* callOperatorDecl) {
    instantiateFunctionDecl(callOperatorDecl);
}

void gulc::TemplateInstHelper::instantiateConstructorDecl(gulc::ConstructorDecl* constructorDecl) {
    instantiateFunctionDecl(constructorDecl);
}

void gulc::TemplateInstHelper::instantiateDestructorDecl(gulc::DestructorDecl* destructorDecl) {
    instantiateFunctionDecl(destructorDecl);
}

void gulc::TemplateInstHelper::instantiateEnumDecl(gulc::EnumDecl* enumDecl) const {
    if (enumDecl->constType != nullptr) {
        instantiateType(enumDecl->constType);
    }

    enumDecl->containerTemplateType = _currentContainerTemplateType != nullptr ?
                                      _currentContainerTemplateType->deepCopy() : nullptr;

    for (EnumConstDecl* enumConst : enumDecl->enumConsts()) {
        if (enumConst->constValue != nullptr) {
            instantiateExpr(enumConst->constValue);
        }
    }
}

void gulc::TemplateInstHelper::instantiateFunctionDecl(gulc::FunctionDecl* functionDecl) {
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

    if (_processBodyStmts) {
        instantiateStmt(functionDecl->body());
    }
}

void gulc::TemplateInstHelper::instantiateOperatorDecl(gulc::OperatorDecl* operatorDecl) {
    instantiateFunctionDecl(operatorDecl);
}

void gulc::TemplateInstHelper::instantiateParameterDecl(gulc::ParameterDecl* parameterDecl) const {
    instantiateType(parameterDecl->type);

    if (parameterDecl->defaultValue != nullptr) {
        instantiateExpr(parameterDecl->defaultValue);
    }
}

void gulc::TemplateInstHelper::instantiatePropertyDecl(gulc::PropertyDecl* propertyDecl) {
    instantiateType(propertyDecl->type);

    for (PropertyGetDecl* getter : propertyDecl->getters()) {
        instantiateDecl(getter);
    }

    if (propertyDecl->hasSetter()) {
        instantiateDecl(propertyDecl->setter());
    }
}

void gulc::TemplateInstHelper::instantiatePropertyGetDecl(gulc::PropertyGetDecl* propertyGetDecl) {
    instantiateFunctionDecl(propertyGetDecl);
}

void gulc::TemplateInstHelper::instantiatePropertySetDecl(gulc::PropertySetDecl* propertySetDecl) {
    instantiateFunctionDecl(propertySetDecl);
}

void gulc::TemplateInstHelper::instantiateStructDecl(gulc::StructDecl* structDecl, bool setTemplateContainer) {
    structDecl->containerTemplateType = _currentContainerTemplateType != nullptr ?
                                        _currentContainerTemplateType->deepCopy() : nullptr;

    Type* oldContainerTemplateType = _currentContainerTemplateType;

    if (setTemplateContainer && _currentContainerTemplateType != nullptr) {
        _currentContainerTemplateType = new DependentType(Type::Qualifier::Unassigned,
                                                          _currentContainerTemplateType,
                                                          new StructType(Type::Qualifier::Unassigned,
                                                                         structDecl, {}, {}));
    }

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

    _currentContainerTemplateType = oldContainerTemplateType;
}

void gulc::TemplateInstHelper::instantiateSubscriptOperatorDecl(gulc::SubscriptOperatorDecl* subscriptOperatorDecl) {
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

void gulc::TemplateInstHelper::instantiateSubscriptOperatorGetDecl(gulc::SubscriptOperatorGetDecl* subscriptOperatorGetDecl) {
    instantiateFunctionDecl(subscriptOperatorGetDecl);
}

void gulc::TemplateInstHelper::instantiateSubscriptOperatorSetDecl(gulc::SubscriptOperatorSetDecl* subscriptOperatorSetDecl) {
    instantiateFunctionDecl(subscriptOperatorSetDecl);
}

void gulc::TemplateInstHelper::instantiateTemplateFunctionDecl(gulc::TemplateFunctionDecl* templateFunctionDecl) {
    for (TemplateParameterDecl* templateParameter : templateFunctionDecl->templateParameters()) {
        instantiateTemplateParameterDecl(templateParameter);
    }

    instantiateFunctionDecl(templateFunctionDecl);
}

void gulc::TemplateInstHelper::instantiateTemplateParameterDecl(gulc::TemplateParameterDecl* templateParameterDecl) const {
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

void gulc::TemplateInstHelper::instantiateTemplateStructDecl(gulc::TemplateStructDecl* templateStructDecl) {
    for (TemplateParameterDecl* templateParameter : templateStructDecl->templateParameters()) {
        instantiateTemplateParameterDecl(templateParameter);
    }

    Type* oldContainerTemplateType = _currentContainerTemplateType;

    // Create the list of template parameters from the template parameters
    std::vector<Expr*> containerTemplateArguments;
    containerTemplateArguments.reserve(templateStructDecl->templateParameters().size());

    for (TemplateParameterDecl* templateParameter : templateStructDecl->templateParameters()) {
        if (templateParameter->templateParameterKind() == TemplateParameterDecl::TemplateParameterKind::Typename) {
            containerTemplateArguments.push_back(new TypeExpr(new TemplateTypenameRefType(Type::Qualifier::Unassigned,
                                                                                          templateParameter, {}, {})));
        } else {
            containerTemplateArguments.push_back(new TemplateConstRefExpr(templateParameter));
        }
    }

    Type* containerTemplateType = new TemplateStructType(Type::Qualifier::Unassigned,
                                                         containerTemplateArguments,
                                                         templateStructDecl, {}, {});

    if (_currentContainerTemplateType != nullptr) {
        _currentContainerTemplateType = new DependentType(Type::Qualifier::Unassigned,
                                                          _currentContainerTemplateType,
                                                          containerTemplateType);
    } else {
        _currentContainerTemplateType = containerTemplateType;
    }

    instantiateStructDecl(templateStructDecl, false);

    // We set `containerTemplateType` AFTER `instantiateStructDecl` since `instantiateStructDecl` will set it to
    // `_currentContainerTemplateType` which we do NOT want.
    templateStructDecl->containerTemplateType = oldContainerTemplateType != nullptr ?
                                                oldContainerTemplateType->deepCopy() : nullptr;

    _currentContainerTemplateType = oldContainerTemplateType;
}

void gulc::TemplateInstHelper::instantiateTemplateStructInstDecl(gulc::TemplateStructInstDecl* templateStructInstDecl) {
    for (Expr*& templateArgument : templateStructInstDecl->templateArguments()) {
        instantiateExpr(templateArgument);
    }

    Type* oldContainerTemplateType = _currentContainerTemplateType;

    // For template instantiations we only set `_currentContainerTemplateType` if it isn't null
    // The reason for this is since this is an instantiation it is no longer needed to keep track of the container
    // template type
    if (_currentContainerTemplateType != nullptr) {
        std::vector<Expr*> copiedTemplateArguments;
        copiedTemplateArguments.reserve(templateStructInstDecl->templateArguments().size());

        for (Expr* templateArgument : templateStructInstDecl->templateArguments()) {
            copiedTemplateArguments.push_back(templateArgument->deepCopy());
        }

        _currentContainerTemplateType = new DependentType(Type::Qualifier::Unassigned,
                                                          _currentContainerTemplateType,
                                                          new TemplateStructType(Type::Qualifier::Unassigned,
                                                                                 copiedTemplateArguments,
                                                                                 templateStructInstDecl->parentTemplateStruct(),
                                                                                 {}, {}));
    }

    instantiateStructDecl(templateStructInstDecl, false);

    // We set `containerTemplateType` AFTER `instantiateStructDecl` since `instantiateStructDecl` will set it to
    // `_currentContainerTemplateType` which we do NOT want.
    templateStructInstDecl->containerTemplateType = oldContainerTemplateType != nullptr ?
                                                    oldContainerTemplateType->deepCopy() : nullptr;

    _currentContainerTemplateType = oldContainerTemplateType;
}

void gulc::TemplateInstHelper::instantiateTemplateTraitDecl(gulc::TemplateTraitDecl* templateTraitDecl) {
    for (TemplateParameterDecl* templateParameter : templateTraitDecl->templateParameters()) {
        instantiateTemplateParameterDecl(templateParameter);
    }

    Type* oldContainerTemplateType = _currentContainerTemplateType;

    // Create the list of template parameters from the template parameters
    std::vector<Expr*> containerTemplateArguments;
    containerTemplateArguments.reserve(templateTraitDecl->templateParameters().size());

    for (TemplateParameterDecl* templateParameter : templateTraitDecl->templateParameters()) {
        if (templateParameter->templateParameterKind() == TemplateParameterDecl::TemplateParameterKind::Typename) {
            containerTemplateArguments.push_back(new TypeExpr(new TemplateTypenameRefType(Type::Qualifier::Unassigned,
                                                                                          templateParameter, {}, {})));
        } else {
            containerTemplateArguments.push_back(new TemplateConstRefExpr(templateParameter));
        }
    }

    Type* containerTemplateType = new TemplateTraitType(Type::Qualifier::Unassigned,
                                                        containerTemplateArguments,
                                                        templateTraitDecl, {}, {});

    if (_currentContainerTemplateType != nullptr) {
        _currentContainerTemplateType = new DependentType(Type::Qualifier::Unassigned,
                                                          _currentContainerTemplateType,
                                                          containerTemplateType);
    } else {
        _currentContainerTemplateType = containerTemplateType;
    }

    instantiateTraitDecl(templateTraitDecl, false);

    // We set `containerTemplateType` AFTER `instantiateTraitDecl` since `instantiateTraitDecl` will set it to
    // `_currentContainerTemplateType` which we do NOT want.
    templateTraitDecl->containerTemplateType = oldContainerTemplateType != nullptr ?
                                               oldContainerTemplateType->deepCopy() : nullptr;

    _currentContainerTemplateType = oldContainerTemplateType;
}

void gulc::TemplateInstHelper::instantiateTemplateTraitInstDecl(gulc::TemplateTraitInstDecl* templateTraitInstDecl) {
    for (Expr*& templateArgument : templateTraitInstDecl->templateArguments()) {
        instantiateExpr(templateArgument);
    }

    Type* oldContainerTemplateType = _currentContainerTemplateType;

    // For template instantiations we only set `_currentContainerTemplateType` if it isn't null
    // The reason for this is since this is an instantiation it is no longer needed to keep track of the container
    // template type
    if (_currentContainerTemplateType != nullptr) {
        std::vector<Expr*> copiedTemplateArguments;
        copiedTemplateArguments.reserve(templateTraitInstDecl->templateArguments().size());

        for (Expr* templateArgument : templateTraitInstDecl->templateArguments()) {
            copiedTemplateArguments.push_back(templateArgument->deepCopy());
        }

        _currentContainerTemplateType = new DependentType(Type::Qualifier::Unassigned,
                                                          _currentContainerTemplateType,
                                                          new TemplateTraitType(Type::Qualifier::Unassigned,
                                                                                copiedTemplateArguments,
                                                                                templateTraitInstDecl->parentTemplateTrait(),
                                                                                {}, {}));
    }

    instantiateTraitDecl(templateTraitInstDecl, false);

    // We set `containerTemplateType` AFTER `instantiateTraitDecl` since `instantiateTraitDecl` will set it to
    // `_currentContainerTemplateType` which we do NOT want.
    templateTraitInstDecl->containerTemplateType = oldContainerTemplateType != nullptr ?
                                                   oldContainerTemplateType->deepCopy() : nullptr;

    _currentContainerTemplateType = oldContainerTemplateType;
}

void gulc::TemplateInstHelper::instantiateTraitDecl(gulc::TraitDecl* traitDecl, bool setTemplateContainer) {
    traitDecl->containerTemplateType = _currentContainerTemplateType != nullptr ?
                                       _currentContainerTemplateType->deepCopy() : nullptr;

    Type* oldContainerTemplateType = _currentContainerTemplateType;

    if (setTemplateContainer && _currentContainerTemplateType != nullptr) {
        _currentContainerTemplateType = new DependentType(Type::Qualifier::Unassigned,
                                                          _currentContainerTemplateType,
                                                          new TraitType(Type::Qualifier::Unassigned,
                                                                        traitDecl, {}, {}));
    }

    for (Cont* contract : traitDecl->contracts()) {
        instantiateCont(contract);
    }

    for (Type*& inheritedType : traitDecl->inheritedTypes()) {
        instantiateType(inheritedType);
    }

    for (Decl* ownedMember : traitDecl->ownedMembers()) {
        instantiateDecl(ownedMember);
    }

    _currentContainerTemplateType = oldContainerTemplateType;
}

void gulc::TemplateInstHelper::instantiateTypeAliasDecl(gulc::TypeAliasDecl* typeAliasDecl) const {
    typeAliasDecl->containerTemplateType = _currentContainerTemplateType != nullptr ?
                                           _currentContainerTemplateType->deepCopy() : nullptr;

    for (TemplateParameterDecl* templateParameter : typeAliasDecl->templateParameters()) {
        instantiateTemplateParameterDecl(templateParameter);
    }

    instantiateType(typeAliasDecl->typeValue);
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

void gulc::TemplateInstHelper::instantiateDoCatchStmt(gulc::DoCatchStmt* doCatchStmt) const {
    instantiateStmt(doCatchStmt->body());

    for (CatchStmt* catchStmt : doCatchStmt->catchStatements()) {
        instantiateStmt(catchStmt);
    }

    if (doCatchStmt->hasFinallyStatement()) {
        instantiateStmt(doCatchStmt->finallyStatement());
    }
}

void gulc::TemplateInstHelper::instantiateDoWhileStmt(gulc::DoWhileStmt* doWhileStmt) const {
    instantiateStmt(doWhileStmt->body());
    instantiateExpr(doWhileStmt->condition);
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

void gulc::TemplateInstHelper::instantiateAssignmentOperatorExpr(gulc::AssignmentOperatorExpr* assignmentOperatorExpr) const {
    instantiateExpr(assignmentOperatorExpr->leftValue);
    instantiateExpr(assignmentOperatorExpr->rightValue);
}

void gulc::TemplateInstHelper::instantiateCheckExtendsTypeExpr(gulc::CheckExtendsTypeExpr* checkExtendsTypeExpr) const {
    instantiateType(checkExtendsTypeExpr->checkType);
    instantiateType(checkExtendsTypeExpr->extendsType);
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

void gulc::TemplateInstHelper::instantiateSubscriptCallExpr(gulc::SubscriptCallExpr* subscriptCallExpr) const {
    instantiateExpr(subscriptCallExpr->subscriptReference);

    for (Expr* argument : subscriptCallExpr->arguments) {
        instantiateExpr(argument);
    }
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
