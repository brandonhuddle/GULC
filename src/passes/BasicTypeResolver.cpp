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
#include <utilities/TypeHelper.hpp>
#include <ast/types/DimensionType.hpp>
#include <ast/types/PointerType.hpp>
#include <ast/types/ReferenceType.hpp>
#include <ast/types/TemplatedType.hpp>
#include <ast/exprs/IdentifierExpr.hpp>
#include <ast/types/UnresolvedType.hpp>
#include <ast/types/FlatArrayType.hpp>
#include <ast/types/BuiltInType.hpp>
#include <ast/types/TraitType.hpp>
#include <ast/types/UnresolvedNestedType.hpp>
#include <ast/conts/WhereCont.hpp>
#include <ast/types/DependentType.hpp>
#include <make_reverse_iterator.hpp>
#include <ast/types/SelfType.hpp>
#include <ast/types/EnumType.hpp>
#include <ast/types/StructType.hpp>
#include "BasicTypeResolver.hpp"

void gulc::BasicTypeResolver::processFiles(std::vector<ASTFile>& files) {
    for (ASTFile& file : files) {
        _currentFile = &file;

        for (Decl* decl : file.declarations) {
            processDecl(decl);

            if (llvm::isa<ExtensionDecl>(decl)) {
                _currentFile->scopeExtensions.push_back(llvm::dyn_cast<ExtensionDecl>(decl));
            }
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

bool gulc::BasicTypeResolver::resolveType(gulc::Type*& type) {
    bool isAmbiguous = false;
    bool result = TypeHelper::resolveType(type, _currentFile, _namespacePrototypes, _templateParameters,
                                          _containingDecls, &isAmbiguous);

    if (result) {
        if (isAmbiguous) {
            printError("type `" + type->toString() + "` is ambiguous!",
                       type->startPosition(), type->endPosition());
        }

        processType(type);
    }

    return result;
}

void gulc::BasicTypeResolver::processType(gulc::Type*& type) {
    if (llvm::isa<DependentType>(type)) {
        auto dependentType = llvm::dyn_cast<DependentType>(type);

        processType(dependentType->container);
        processType(dependentType->dependent);
    } else if (llvm::isa<DimensionType>(type)) {
        auto dimensionType = llvm::dyn_cast<DimensionType>(type);

        processType(dimensionType->nestedType);
    } else if (llvm::isa<FlatArrayType>(type)) {
        auto flatArrayType = llvm::dyn_cast<FlatArrayType>(type);

        processExpr(flatArrayType->length);
        processType(flatArrayType->indexType);
    } else if (llvm::isa<UnresolvedNestedType>(type)) {
        auto nestedType = llvm::dyn_cast<UnresolvedNestedType>(type);

        processType(nestedType->container);

        for (Expr*& argument : nestedType->templateArguments()) {
            processTemplateArgumentExpr(argument);
        }
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
    } else if (llvm::isa<SelfType>(type)) {
        if (!_containingDecls.empty()) {
            // Replace `Self` with the actual type if possible.
            switch (_containingDecls[_containingDecls.size() - 1]->getDeclKind()) {
                case Decl::Kind::Enum: {
                    auto enumDecl = llvm::dyn_cast<EnumDecl>(_containingDecls[_containingDecls.size() - 1]);
                    auto enumType = new EnumType(Type::Qualifier::Unassigned, enumDecl,
                                                 type->startPosition(), type->endPosition());
                    delete type;
                    type = enumType;
                    break;
                }
                case Decl::Kind::Struct: {
                    auto structDecl = llvm::dyn_cast<StructDecl>(_containingDecls[_containingDecls.size() - 1]);
                    auto structType = new StructType(Type::Qualifier::Unassigned, structDecl,
                                                     type->startPosition(), type->endPosition());
                    delete type;
                    type = structType;
                    break;
                }
                case Decl::Kind::Trait: {
                    auto traitDecl = llvm::dyn_cast<TraitDecl>(_containingDecls[_containingDecls.size() - 1]);
                    auto traitType = new TraitType(Type::Qualifier::Unassigned, traitDecl,
                                                   type->startPosition(), type->endPosition());
                    delete type;
                    type = traitType;
                    break;
                }
                default:
                    break;
            }
        }
    }
}

void gulc::BasicTypeResolver::processContracts(std::vector<Cont*>& contracts) {
    for (Cont* contract : contracts) {
        switch (contract->getContKind()) {
            case Cont::Kind::Where: {
                auto whereCont = llvm::dyn_cast<WhereCont>(contract);

                processExpr(whereCont->condition);
                break;
            }
            case Cont::Kind::Requires:
                printError("`requires` not yet supported!",
                           contract->startPosition(), contract->endPosition());
                break;
            case Cont::Kind::Ensures:
                printError("`ensures` not yet supported!",
                           contract->startPosition(), contract->endPosition());
                break;
            case Cont::Kind::Throws:
                printError("`throws` not yet supported!",
                           contract->startPosition(), contract->endPosition());
                break;
            default:
                printError("unknown contract!",
                           contract->startPosition(), contract->endPosition());
                break;
        }
    }
}

void gulc::BasicTypeResolver::processDecl(gulc::Decl* decl, bool isGlobal) {
    switch (decl->getDeclKind()) {
        case Decl::Kind::Import:
            // We skip imports, they're no longer useful here...
            break;

        case Decl::Kind::Enum:
            processEnumDecl(llvm::dyn_cast<EnumDecl>(decl));
            break;
        case Decl::Kind::Extension:
            processExtensionDecl(llvm::dyn_cast<ExtensionDecl>(decl));
            break;
        case Decl::Kind::CallOperator:
        case Decl::Kind::Constructor:
        case Decl::Kind::Destructor:
        case Decl::Kind::Function:
        case Decl::Kind::Operator:
        case Decl::Kind::TypeSuffix:
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

void gulc::BasicTypeResolver::processEnumDecl(gulc::EnumDecl* enumDecl) {
    if (enumDecl->constType != nullptr) {
        if (!resolveType(enumDecl->constType)) {
            printError("enum base type `" + enumDecl->constType->toString() + "` was not found!",
                       enumDecl->startPosition(), enumDecl->endPosition());
        }
    } else {
        enumDecl->constType = gulc::BuiltInType::get(Type::Qualifier::Unassigned, "i32", {}, {});
    }

    for (EnumConstDecl* enumConst : enumDecl->enumConsts()) {
        // TODO: If the `constValue` is null we will need to handle setting the default values
        if (enumConst->constValue != nullptr) {
            processExpr(enumConst->constValue);
        }
    }
}

void gulc::BasicTypeResolver::processExtensionDecl(gulc::ExtensionDecl* extensionDecl) {
    processContracts(extensionDecl->contracts());

    if (!resolveType(extensionDecl->typeToExtend)) {
        printError("extension type `" + extensionDecl->typeToExtend->toString() + "` was not found!",
                   extensionDecl->typeToExtend->startPosition(), extensionDecl->typeToExtend->endPosition());
    }

    for (Type*& inheritedType : extensionDecl->inheritedTypes()) {
        if (!resolveType(inheritedType)) {
            printError("extension inherited type `" + inheritedType->toString() + "` was not found!",
                       inheritedType->startPosition(), inheritedType->endPosition());
        }
    }

    _containingDecls.push_back(extensionDecl);

    for (Decl* member : extensionDecl->ownedMembers()) {
        processDecl(member, false);
    }

    // Make sure we process the constructors since they are NOT in the `ownedMembers` list...
    for (ConstructorDecl* constructor : extensionDecl->constructors()) {
        processDecl(constructor);
    }

    _containingDecls.pop_back();
}

void gulc::BasicTypeResolver::processFunctionDecl(gulc::FunctionDecl* functionDecl) {
    processContracts(functionDecl->contracts());

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

    // Clear any old values
    _labelIdentifiers.clear();

    processCompoundStmt(functionDecl->body());

    // Check if any labels were used but not found
    for (auto const& checkLabel : _labelIdentifiers) {
        if (!checkLabel.second.status) {
            printError("label `" + checkLabel.first + "` was not found!",
                       checkLabel.second.startPosition, checkLabel.second.endPosition);
        }
    }
}

void gulc::BasicTypeResolver::processNamespaceDecl(gulc::NamespaceDecl* namespaceDecl) {
    _containingDecls.push_back(namespaceDecl);

    for (Decl* nestedDecl : namespaceDecl->nestedDecls()) {
        processDecl(nestedDecl);

        if (llvm::isa<ExtensionDecl>(nestedDecl)) {
            namespaceDecl->scopeExtensions.push_back(llvm::dyn_cast<ExtensionDecl>(nestedDecl));
        }
    }

    _containingDecls.pop_back();
}

void gulc::BasicTypeResolver::processParameterDecl(gulc::ParameterDecl* parameterDecl) {
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
    processContracts(structDecl->contracts());

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

void gulc::BasicTypeResolver::processTemplateParameterDecl(gulc::TemplateParameterDecl* templateParameterDecl) {
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
    processContracts(traitDecl->contracts());

    for (Type*& inheritedType : traitDecl->inheritedTypes()) {
        if (!resolveType(inheritedType)) {
            printError("trait inherited type `" + inheritedType->toString() + "` was not found!",
                       inheritedType->startPosition(), inheritedType->endPosition());
        }
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

    if (!resolveType(typeAliasDecl->typeValue)) {
        printError("alias type `" + typeAliasDecl->typeValue->toString() + "` was not found!",
                   typeAliasDecl->typeValue->startPosition(), typeAliasDecl->typeValue->endPosition());
    }

    if (typeAliasDecl->hasTemplateParameters()) {
        _templateParameters.pop_back();
    }
}

void gulc::BasicTypeResolver::processVariableDecl(gulc::VariableDecl* variableDecl, bool isGlobal) {
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

void gulc::BasicTypeResolver::processStmt(gulc::Stmt* stmt) {
    switch (stmt->getStmtKind()) {
        case Stmt::Kind::Break:
            processBreakStmt(llvm::dyn_cast<BreakStmt>(stmt));
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
        case Stmt::Kind::Continue:
            processContinueStmt(llvm::dyn_cast<ContinueStmt>(stmt));
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
            processGotoStmt(llvm::dyn_cast<GotoStmt>(stmt));
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
        case Stmt::Kind::Expr:
            processExpr(llvm::dyn_cast<Expr>(stmt));
            break;

        default:
            printError("INTERNAL ERROR - unhandled Stmt type found in `BasicTypeResolver`!",
                       stmt->startPosition(), stmt->endPosition());
            break;
    }
}

void gulc::BasicTypeResolver::processBreakStmt(gulc::BreakStmt* breakStmt) {
    if (breakStmt->hasBreakLabel()) {
        if (_labelIdentifiers.find(breakStmt->breakLabel().name()) == _labelIdentifiers.end()) {
            _labelIdentifiers.insert({breakStmt->breakLabel().name(),
                                      LabelStatus(false, breakStmt->breakLabel().startPosition(),
                                                  breakStmt->breakLabel().endPosition())});
        }
    }
}

void gulc::BasicTypeResolver::processCaseStmt(gulc::CaseStmt* caseStmt) {
    if (!caseStmt->isDefault()) {
        processExpr(caseStmt->condition);
    }

    for (Stmt*& statement : caseStmt->body) {
        processStmt(statement);
    }
}

void gulc::BasicTypeResolver::processCatchStmt(gulc::CatchStmt* catchStmt) {
    if (catchStmt->hasExceptionType()) {
        if (!resolveType(catchStmt->exceptionType)) {
            printError("catch type `" + catchStmt->exceptionType->toString() = "` was not found!",
                       catchStmt->exceptionType->startPosition(), catchStmt->exceptionType->endPosition());
        }
    }

    processStmt(catchStmt->body());
}

void gulc::BasicTypeResolver::processCompoundStmt(gulc::CompoundStmt* compoundStmt) {
    for (Stmt* statement : compoundStmt->statements) {
        processStmt(statement);
    }
}

void gulc::BasicTypeResolver::processContinueStmt(gulc::ContinueStmt* continueStmt) {
    if (continueStmt->hasContinueLabel()) {
        if (_labelIdentifiers.find(continueStmt->continueLabel().name()) == _labelIdentifiers.end()) {
            _labelIdentifiers.insert({continueStmt->continueLabel().name(),
                                      LabelStatus(false, continueStmt->continueLabel().startPosition(),
                                                  continueStmt->continueLabel().endPosition())});
        }
    }
}

void gulc::BasicTypeResolver::processDoCatchStmt(gulc::DoCatchStmt* doCatchStmt) {
    processStmt(doCatchStmt->body());

    for (CatchStmt* catchStmt : doCatchStmt->catchStatements()) {
        processCatchStmt(catchStmt);
    }

    if (doCatchStmt->hasFinallyStatement()) {
        processStmt(doCatchStmt->finallyStatement());
    }
}

void gulc::BasicTypeResolver::processDoWhileStmt(gulc::DoWhileStmt* doWhileStmt) {
    processStmt(doWhileStmt->body());
    processExpr(doWhileStmt->condition);
}

void gulc::BasicTypeResolver::processForStmt(gulc::ForStmt* forStmt) {
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

void gulc::BasicTypeResolver::processGotoStmt(gulc::GotoStmt* gotoStmt) {
    if (_labelIdentifiers.find(gotoStmt->label().name()) == _labelIdentifiers.end()) {
        _labelIdentifiers.insert({gotoStmt->label().name(),
                                  LabelStatus(false, gotoStmt->label().startPosition(),
                                              gotoStmt->label().endPosition())});
    }
}

void gulc::BasicTypeResolver::processIfStmt(gulc::IfStmt* ifStmt) {
    processExpr(ifStmt->condition);
    processStmt(ifStmt->trueBody());

    if (ifStmt->hasFalseBody()) {
        processStmt(ifStmt->falseBody());
    }
}

void gulc::BasicTypeResolver::processLabeledStmt(gulc::LabeledStmt* labeledStmt) {
    processStmt(labeledStmt->labeledStmt);

    // Either store the status as already being true or change the status to true
    if (_labelIdentifiers.find(labeledStmt->label().name()) == _labelIdentifiers.end()) {
        _labelIdentifiers.insert({labeledStmt->label().name(),
                                  LabelStatus(true, labeledStmt->label().startPosition(),
                                              labeledStmt->label().endPosition())});
    } else {
        // If the status is already true we output an error saying the label name is used twice
        if (_labelIdentifiers[labeledStmt->label().name()].status) {
            printError("redefinition of label `" + labeledStmt->label().name() + "`!",
                       _labelIdentifiers[labeledStmt->label().name()].startPosition,
                       _labelIdentifiers[labeledStmt->label().name()].endPosition);
        }

        // We change the start and end positions as well for proper redefinition error messages
        _labelIdentifiers[labeledStmt->label().name()] = LabelStatus(true, labeledStmt->label().startPosition(),
                                                                     labeledStmt->label().endPosition());
    }
}

void gulc::BasicTypeResolver::processReturnStmt(gulc::ReturnStmt* returnStmt) {
    if (returnStmt->returnValue != nullptr) {
        processExpr(returnStmt->returnValue);
    }
}

void gulc::BasicTypeResolver::processSwitchStmt(gulc::SwitchStmt* switchStmt) {
    processExpr(switchStmt->condition);

    for (CaseStmt* statement : switchStmt->cases) {
        processCaseStmt(statement);
    }
}

void gulc::BasicTypeResolver::processWhileStmt(gulc::WhileStmt* whileStmt) {
    processExpr(whileStmt->condition);
    processStmt(whileStmt->body());
}

void gulc::BasicTypeResolver::processTemplateArgumentExpr(gulc::Expr*& expr) {
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
            //       (but only if there aren't template parameters...)
            printError("type `" + identifierExpr->identifier().name() + "` was not found!",
                       identifierExpr->startPosition(), identifierExpr->endPosition());
        }
    } else {
        processExpr(expr);
    }
}

void gulc::BasicTypeResolver::processExpr(gulc::Expr* expr) {
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
            // Nothing to do here, no type needs resolving.
            break;
        case Expr::Kind::CheckExtendsType:
            processCheckExtendsTypeExpr(llvm::dyn_cast<CheckExtendsTypeExpr>(expr));
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
        case Expr::Kind::Ref:
            processRefExpr(llvm::dyn_cast<RefExpr>(expr));
            break;
        case Expr::Kind::SubscriptCall:
            processSubscriptCallExpr(llvm::dyn_cast<SubscriptCallExpr>(expr));
            break;
        case Expr::Kind::Ternary:
            processTernaryExpr(llvm::dyn_cast<TernaryExpr>(expr));
            break;
        case Expr::Kind::Try:
            processTryExpr(llvm::dyn_cast<TryExpr>(expr));
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

void gulc::BasicTypeResolver::processArrayLiteralExpr(gulc::ArrayLiteralExpr* arrayLiteralExpr) {
    for (Expr* index : arrayLiteralExpr->indexes) {
        processExpr(index);
    }
}

void gulc::BasicTypeResolver::processAsExpr(gulc::AsExpr* asExpr) {
    processExpr(asExpr->expr);

    if (!resolveType(asExpr->asType)) {
        printError("as type `" + asExpr->asType->toString() + "` was not found!",
                   asExpr->asType->startPosition(), asExpr->asType->endPosition());
    }
}

void gulc::BasicTypeResolver::processAssignmentOperatorExpr(gulc::AssignmentOperatorExpr* assignmentOperatorExpr) {
    processExpr(assignmentOperatorExpr->leftValue);
    processExpr(assignmentOperatorExpr->rightValue);
}

void gulc::BasicTypeResolver::processCheckExtendsTypeExpr(gulc::CheckExtendsTypeExpr* checkExtendsTypeExpr) {
    if (!resolveType(checkExtendsTypeExpr->checkType)) {
        printError("type `" + checkExtendsTypeExpr->checkType->toString() + "` was not found!",
                   checkExtendsTypeExpr->checkType->startPosition(),
                   checkExtendsTypeExpr->checkType->endPosition());
    }

    if (!resolveType(checkExtendsTypeExpr->extendsType)) {
        printError("type `" + checkExtendsTypeExpr->extendsType->toString() + "` was not found!",
                   checkExtendsTypeExpr->extendsType->startPosition(),
                   checkExtendsTypeExpr->extendsType->endPosition());
    }
}

void gulc::BasicTypeResolver::processFunctionCallExpr(gulc::FunctionCallExpr* functionCallExpr) {
    processExpr(functionCallExpr->functionReference);

    for (LabeledArgumentExpr* argument : functionCallExpr->arguments) {
        processLabeledArgumentExpr(argument);
    }
}

void gulc::BasicTypeResolver::processHasExpr(gulc::HasExpr* hasExpr) {
    processExpr(hasExpr->expr);

    if (!resolveType(hasExpr->trait)) {
        printError("has type `" + hasExpr->trait->toString() + "` was not found!",
                   hasExpr->startPosition(), hasExpr->endPosition());
    }
}

void gulc::BasicTypeResolver::processIdentifierExpr(gulc::IdentifierExpr* identifierExpr) {
    for (Expr* templateArgument : identifierExpr->templateArguments()) {
        processExpr(templateArgument);
    }
}

void gulc::BasicTypeResolver::processInfixOperatorExpr(gulc::InfixOperatorExpr* infixOperatorExpr) {
    processExpr(infixOperatorExpr->leftValue);
    processExpr(infixOperatorExpr->rightValue);
}

void gulc::BasicTypeResolver::processIsExpr(gulc::IsExpr* isExpr) {
    processExpr(isExpr->expr);

    if (!resolveType(isExpr->isType)) {
        printError("is type `" + isExpr->isType->toString() + "` was not found!",
                   isExpr->isType->startPosition(), isExpr->isType->endPosition());
    }
}

void gulc::BasicTypeResolver::processLabeledArgumentExpr(gulc::LabeledArgumentExpr* labeledArgumentExpr) {
    processExpr(labeledArgumentExpr->argument);
}

void gulc::BasicTypeResolver::processMemberAccessCallExpr(gulc::MemberAccessCallExpr* memberAccessCallExpr) {
    processExpr(memberAccessCallExpr->objectRef);
}

void gulc::BasicTypeResolver::processParenExpr(gulc::ParenExpr* parenExpr) {
    processExpr(parenExpr->nestedExpr);
}

void gulc::BasicTypeResolver::processPostfixOperatorExpr(gulc::PostfixOperatorExpr* postfixOperatorExpr) {
    processExpr(postfixOperatorExpr->nestedExpr);
}

void gulc::BasicTypeResolver::processPrefixOperatorExpr(gulc::PrefixOperatorExpr* prefixOperatorExpr) {
    processExpr(prefixOperatorExpr->nestedExpr);
}

void gulc::BasicTypeResolver::processRefExpr(gulc::RefExpr* refExpr) {
    processExpr(refExpr->nestedExpr);
}

void gulc::BasicTypeResolver::processSubscriptCallExpr(gulc::SubscriptCallExpr* subscriptCallExpr) {
    processExpr(subscriptCallExpr->subscriptReference);

    for (LabeledArgumentExpr* argument : subscriptCallExpr->arguments) {
        processLabeledArgumentExpr(argument);
    }
}

void gulc::BasicTypeResolver::processTernaryExpr(gulc::TernaryExpr* ternaryExpr) {
    processExpr(ternaryExpr->condition);
    processExpr(ternaryExpr->trueExpr);
    processExpr(ternaryExpr->falseExpr);
}

void gulc::BasicTypeResolver::processTryExpr(gulc::TryExpr* tryExpr) {
    processExpr(tryExpr->nestedExpr);
}

void gulc::BasicTypeResolver::processTypeExpr(gulc::TypeExpr* typeExpr) {
    if (!resolveType(typeExpr->type)) {
        printError("type `" + typeExpr->toString() + "` was not found!",
                   typeExpr->startPosition(), typeExpr->endPosition());
    }
}

void gulc::BasicTypeResolver::processValueLiteralExpr(gulc::ValueLiteralExpr* valueLiteralExpr) const {
    // TODO: Is there anything we can do here? I don't think there is anything we can do until `DeclInstantiator`...
}

void gulc::BasicTypeResolver::processVariableDeclExpr(gulc::VariableDeclExpr* variableDeclExpr) {
    if (variableDeclExpr->type != nullptr) {
        if (!resolveType(variableDeclExpr->type)) {
            printError("local variable type `" + variableDeclExpr->type->toString() + "` was not found!",
                       variableDeclExpr->type->startPosition(), variableDeclExpr->type->endPosition());
        }
    }

    if (variableDeclExpr->initialValue != nullptr) {
        processExpr(variableDeclExpr->initialValue);
    }
}
