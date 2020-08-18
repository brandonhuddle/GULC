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
#include <ast/types/DependentType.hpp>
#include <ast/types/StructType.hpp>
#include <ast/types/TemplateStructType.hpp>
#include "CodeProcessor.hpp"
#include "DeclInstantiator.hpp"
#include <utilities/SignatureComparer.hpp>
#include <ast/exprs/ConstructorCallExpr.hpp>
#include <ast/types/TraitType.hpp>
#include <ast/types/TemplateTraitType.hpp>
#include <ast/exprs/VTableFunctionReferenceExpr.hpp>
#include <ast/exprs/FunctionReferenceExpr.hpp>
#include <ast/exprs/MemberFunctionCallExpr.hpp>
#include <ast/types/ReferenceType.hpp>
#include <ast/types/EnumType.hpp>
#include <ast/types/BuiltInType.hpp>
#include <ast/exprs/LocalVariableRefExpr.hpp>
#include <ast/exprs/ParameterRefExpr.hpp>
#include <ast/exprs/EnumConstRefExpr.hpp>
#include <utilities/TypeHelper.hpp>
#include <ast/exprs/MemberVariableRefExpr.hpp>
#include <ast/exprs/VariableRefExpr.hpp>
#include <ast/exprs/ImplicitCastExpr.hpp>
#include <ast/types/FlatArrayType.hpp>
#include <ast/types/PointerType.hpp>
#include <ast/decls/TypeSuffixDecl.hpp>
#include <utilities/FunctorUtil.hpp>
#include <ast/types/BoolType.hpp>
#include <ast/exprs/LValueToRValueExpr.hpp>
#include <ast/exprs/StructAssignmentOperatorExpr.hpp>
#include <ast/exprs/ImplicitDerefExpr.hpp>

void gulc::CodeProcessor::processFiles(std::vector<ASTFile>& files) {
    for (ASTFile& file : files) {
        _currentFile = &file;

        for (Decl* decl : file.declarations) {
            processDecl(decl);
        }
    }
}

void gulc::CodeProcessor::printError(const std::string& message, gulc::TextPosition startPosition,
                                     gulc::TextPosition endPosition) const {
    std::cerr << "gulc error[" << _filePaths[_currentFile->sourceFileID] << ", "
                            "{" << startPosition.line << ", " << startPosition.column << " "
                            "to " << endPosition.line << ", " << endPosition.column << "}]: "
              << message << std::endl;
    std::exit(1);
}

void gulc::CodeProcessor::printWarning(const std::string& message, gulc::TextPosition startPosition,
                                       gulc::TextPosition endPosition) const {
    std::cout << "gulc warning[" << _filePaths[_currentFile->sourceFileID] << ", "
                              "{" << startPosition.line << ", " << startPosition.column << " "
                              "to " << endPosition.line << ", " << endPosition.column << "}]: "
              << message << std::endl;
}

gulc::CurrentSelfExpr* gulc::CodeProcessor::getCurrentSelfRef(TextPosition startPosition,
                                                              TextPosition endPosition) const {
    bool isMutable = false;

    if (llvm::isa<ConstructorDecl>(_currentFunction)) {
        isMutable = true;
    } else {
        isMutable = _currentFunction->isMutable();
    }

    if (llvm::isa<StructDecl>(_currentContainer)) {
        auto structContainer = llvm::dyn_cast<StructDecl>(_currentContainer);
        auto result = new CurrentSelfExpr(startPosition, endPosition);

        // We set the `result` as either `&mut Self` or `&immut Self` depending on what the current function's
        // mutability is
        if (isMutable) {
            result->valueType = new ReferenceType(Type::Qualifier::Unassigned,
                    new StructType(Type::Qualifier::Mut, structContainer, startPosition, endPosition));
        } else {
            result->valueType = new ReferenceType(Type::Qualifier::Unassigned,
                    new StructType(Type::Qualifier::Immut, structContainer, startPosition, endPosition));
        }

        result->valueType->setIsLValue(true);

        return result;
    } else if (llvm::isa<TraitDecl>(_currentContainer)) {
        auto traitContainer = llvm::dyn_cast<TraitDecl>(_currentContainer);
        auto result = new CurrentSelfExpr(startPosition, endPosition);

        // We set the `result` as either `&mut Self` or `&immut Self` depending on what the current function's
        // mutability is
        if (isMutable) {
            result->valueType = new ReferenceType(Type::Qualifier::Unassigned,
                                                  new TraitType(Type::Qualifier::Mut, traitContainer, startPosition, endPosition));
        } else {
            result->valueType = new ReferenceType(Type::Qualifier::Unassigned,
                                                  new TraitType(Type::Qualifier::Immut, traitContainer, startPosition, endPosition));
        }

        result->valueType->setIsLValue(true);

        return result;
    } else if (llvm::isa<EnumDecl>(_currentContainer)) {
        auto enumContainer = llvm::dyn_cast<EnumDecl>(_currentContainer);
        auto result = new CurrentSelfExpr(startPosition, endPosition);

        // We set the `result` as either `&mut Self` or `&immut Self` depending on what the current function's
        // mutability is
        if (isMutable) {
            result->valueType = new ReferenceType(Type::Qualifier::Unassigned,
                                                  new EnumType(Type::Qualifier::Mut, enumContainer, startPosition, endPosition));
        } else {
            result->valueType = new ReferenceType(Type::Qualifier::Unassigned,
                                                  new EnumType(Type::Qualifier::Immut, enumContainer, startPosition, endPosition));
        }

        result->valueType->setIsLValue(true);

        return result;
    } else if (llvm::isa<ExtensionDecl>(_currentContainer)) {
        printError("`extension` not yet supported!",
                   startPosition, endPosition);
    } else {
        printError("[INTERNAL] attempted to grab reference to `self` outside of a context with `self`!",
                   startPosition, endPosition);
    }
    // TODO:
//    else if (llvm::isa<UnionDecl>(_currentContainer)) {
//
//    }

    return nullptr;
}

//=====================================================================================================================
// DECLARATIONS
//=====================================================================================================================
void gulc::CodeProcessor::processDecl(gulc::Decl* decl) {
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
            printError("[INTERNAL ERROR] unexpected `Decl` found in `CodeProcessor::processDecl`!",
                       decl->startPosition(), decl->endPosition());
            break;
    }
}

void gulc::CodeProcessor::processConstructorDecl(gulc::ConstructorDecl* constructorDecl,
                                                 CompoundStmt* temporaryInitializersCompoundStmt) {
    FunctionDecl* oldFunction = _currentFunction;
    _currentFunction = constructorDecl;
    std::vector<ParameterDecl*>* oldParameters = _currentParameters;
    _currentParameters = &constructorDecl->parameters();

    for (ParameterDecl* parameter : constructorDecl->parameters()) {
        processParameterDecl(parameter);
    }

    auto containerStruct = llvm::dyn_cast<StructDecl>(constructorDecl->container);

    switch (constructorDecl->constructorType()) {
        case ConstructorType::Normal: {
            bool addDefaultInitializers = true;

            if (constructorDecl->baseConstructorCall == nullptr) {
                if (containerStruct->baseStruct != nullptr) {
                    // If there isn't a user specified base constructor call but a base constructor exists then we
                    // create a default call to it so the struct is in a valid state without the user needing to
                    // specify it.
                    if (containerStruct->baseStruct->cachedDefaultConstructor == nullptr) {
                        // NOTE: If there is no default base constructor then the current struct CANNOT define any
                        //       constructors without specifying a valid base constructor. Any of our constructors
                        //       found to not have a base constructor is marked deleted and will error out later.
                        constructorDecl->constructorState = ConstructorDecl::ConstructorState::Deleted;
                    } else {
                        // The `processBaseConstructorCall` expects a `FunctionCallExpr` with an `IdentifierExpr` so
                        // that what we provide instead of trying to manually do what `processBaseConstructorCall`
                        // already does.
                        constructorDecl->baseConstructorCall = new FunctionCallExpr(
                                new IdentifierExpr(
                                        Identifier(
                                                constructorDecl->startPosition(),
                                                constructorDecl->endPosition(),
                                                "base"
                                            ),
                                        {}
                                ),
                                {},
                                constructorDecl->startPosition(),
                                constructorDecl->endPosition()
                        );
                    }

                    addDefaultInitializers = true;
                } else {
                    // There is no base constructor call nor base struct to implicitly create a call to, all we need
                    // for this constructor is to add the default initializers and validate it normally
                    addDefaultInitializers = true;
                }
            } else {
                // Check to see if the base constructor call is calling `self`, for `self` constructor calls we don't
                // have to add default initializers since they will be initialized in the other `self` constructor.
                if (llvm::isa<FunctionCallExpr>(constructorDecl->baseConstructorCall)) {
                    auto checkFuncCallExpr = llvm::dyn_cast<FunctionCallExpr>(constructorDecl->baseConstructorCall);

                    if (llvm::isa<IdentifierExpr>(checkFuncCallExpr->functionReference) &&
                            llvm::dyn_cast<IdentifierExpr>(checkFuncCallExpr->functionReference)->identifier().name() ==
                                "self") {
                        addDefaultInitializers = false;
                    }
                }
            }

            // We only add the default initializers if the constructor doesn't call "self(...)" as a base constructor
            // This is because the "self(...)" will already be populating those fields.
            if (addDefaultInitializers) {
                auto& statements = constructorDecl->body()->statements;
                statements.insert(statements.begin(), temporaryInitializersCompoundStmt->deepCopy());
            }

            if (constructorDecl->baseConstructorCall != nullptr) {
                // We process this at the end so we still have `IdentifierExpr` instead of a resolved expression at
                // the end.
                processBaseConstructorCall(containerStruct, constructorDecl->baseConstructorCall);
            }
            break;
        }
        case ConstructorType::Copy: {
            if (constructorDecl->baseConstructorCall != nullptr) {
                printError("copy constructors cannot have explicit base constructor calls!",
                           constructorDecl->baseConstructorCall->startPosition(),
                           constructorDecl->baseConstructorCall->endPosition());
            }

            if (containerStruct->baseStruct != nullptr) {
                // NOTE: I don't think this will ever happen but I keep it here just in case. My plan is for there to
                //       always at least be the constructor prototype but for it to just be marked deleted
                //       `init copy(...) = delete`
                if (containerStruct->baseStruct->cachedCopyConstructor == nullptr) {
                    constructorDecl->constructorState = ConstructorDecl::ConstructorState::Deleted;
                } else {
                    constructorDecl->baseConstructorCall =
                            createBaseMoveCopyConstructorCall(
                                    containerStruct->baseStruct->cachedCopyConstructor,
                                    constructorDecl->parameters()[0],
                                    constructorDecl->startPosition(),
                                    constructorDecl->endPosition()
                            );
                }
            }
            break;
        }
        case ConstructorType::Move: {
            if (constructorDecl->baseConstructorCall != nullptr) {
                printError("move constructors cannot have explicit base constructor calls!",
                           constructorDecl->baseConstructorCall->startPosition(),
                           constructorDecl->baseConstructorCall->endPosition());
            }

            if (containerStruct->baseStruct != nullptr) {
                // NOTE: I don't think this will ever happen but I keep it here just in case. My plan is for there to
                //       always at least be the constructor prototype but for it to just be marked deleted
                //       `init move(...) = delete`
                if (containerStruct->baseStruct->cachedMoveConstructor == nullptr) {
                    constructorDecl->constructorState = ConstructorDecl::ConstructorState::Deleted;
                } else {
                    constructorDecl->baseConstructorCall =
                            createBaseMoveCopyConstructorCall(
                                    containerStruct->baseStruct->cachedMoveConstructor,
                                    constructorDecl->parameters()[0],
                                    constructorDecl->startPosition(),
                                    constructorDecl->endPosition()
                            );
                }
            }
            break;
        }
    }

    // Prototypes don't have bodies
    if (!constructorDecl->isPrototype()) {
        processCompoundStmt(constructorDecl->body());
    }

    // TODO: This is where we should check the `body()` to validate all members are assigned.
    //  * If the constructor is auto-generated and not all members are assigned then the constructor is marked `Deleted`
    //  * If the constructor is user-created and not all members are assigned then we error out saying such

    _currentParameters = oldParameters;
    _currentFunction = oldFunction;
}

void gulc::CodeProcessor::processEnumDecl(gulc::EnumDecl* enumDecl) {
    // TODO: I think at this point processing for `EnumDecl` should be completed, right?
    //       All instantiations should be done in `DeclInstantiator` and validation finished in `DeclInstValidator`
    //       Enums only work with const values as well so those should be solved before this point too.
    // TODO: Once we support `func` within `enum` declarations we will need this.
    Decl* oldContainer = _currentContainer;
    _currentContainer = enumDecl;


    _currentContainer = oldContainer;
}

void gulc::CodeProcessor::processExtensionDecl(gulc::ExtensionDecl* extensionDecl) {
    Decl* oldContainer = _currentContainer;
    _currentContainer = extensionDecl;

    for (Decl* ownedMember : extensionDecl->ownedMembers()) {
        processDecl(ownedMember);
    }

    for (ConstructorDecl* constructor : extensionDecl->constructors()) {
        processDecl(constructor);
    }

    _currentContainer = oldContainer;
}

void gulc::CodeProcessor::processFunctionDecl(gulc::FunctionDecl* functionDecl) {
    FunctionDecl* oldFunction = _currentFunction;
    _currentFunction = functionDecl;
    std::vector<ParameterDecl*>* oldParameters = _currentParameters;
    _currentParameters = &functionDecl->parameters();

    for (ParameterDecl* parameter : functionDecl->parameters()) {
        processParameterDecl(parameter);
    }

    // Prototypes don't have bodies
    if (!functionDecl->isPrototype()) {
        processCompoundStmt(functionDecl->body());
    }

    _currentParameters = oldParameters;
    _currentFunction = oldFunction;
}

void gulc::CodeProcessor::processNamespaceDecl(gulc::NamespaceDecl* namespaceDecl) {
    Decl* oldContainer = _currentContainer;
    _currentContainer = namespaceDecl;

    for (Decl* nestedDecl : namespaceDecl->nestedDecls()) {
        processDecl(nestedDecl);
    }

    _currentContainer = oldContainer;
}

void gulc::CodeProcessor::processParameterDecl(gulc::ParameterDecl* parameterDecl) {
    // TODO: Parameters should be done by this point, right?
}

void gulc::CodeProcessor::processPropertyDecl(gulc::PropertyDecl* propertyDecl) {
    for (PropertyGetDecl* getter : propertyDecl->getters()) {
        processPropertyGetDecl(getter);
    }

    if (propertyDecl->hasSetter()) {
        processPropertySetDecl(propertyDecl->setter());
    }
}

void gulc::CodeProcessor::processPropertyGetDecl(gulc::PropertyGetDecl *propertyGetDecl) {
    processFunctionDecl(propertyGetDecl);
}

void gulc::CodeProcessor::processPropertySetDecl(gulc::PropertySetDecl *propertySetDecl) {
    processFunctionDecl(propertySetDecl);
}

void gulc::CodeProcessor::processStructDecl(gulc::StructDecl* structDecl) {
    Decl* oldContainer = _currentContainer;
    _currentContainer = structDecl;

    // NOTE: For the `copy`, `move`, and `default` constructors we CANNOT validate if any called constructors are valid
    //       or not yet. We have to wait until `CodeTransformer` to handle that. All we're doing here is creating what
    //       the code could be if the required constructors exist.
    //       We also CANNOT error even if a member if `ref` here. All we are doing here is prepending the body of the
    //       constructor with default member initializers. If no defaults exist we will let the later stages validate
    //       whether or not the member is initialized (as the user might initialize the variables themselves) we will
    //       also let an optimization pass detect the unneeded first assignment and completely elide it.
    auto temporaryInitializersCompoundStmt = createStructMemberInitializersCompoundStmt(structDecl);

    for (ConstructorDecl* constructor : structDecl->constructors()) {
        processConstructorDecl(constructor, temporaryInitializersCompoundStmt);
    }

    delete temporaryInitializersCompoundStmt;

    if (structDecl->destructor != nullptr) {
        processFunctionDecl(structDecl->destructor);
    }

    for (Decl* member : structDecl->ownedMembers()) {
        processDecl(member);
    }

    _currentContainer = oldContainer;
}

void gulc::CodeProcessor::processSubscriptOperatorDecl(gulc::SubscriptOperatorDecl* subscriptOperatorDecl) {
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

void gulc::CodeProcessor::processSubscriptOperatorGetDecl(gulc::SubscriptOperatorGetDecl *subscriptOperatorGetDecl) {
    processFunctionDecl(subscriptOperatorGetDecl);
}

void gulc::CodeProcessor::processSubscriptOperatorSetDecl(gulc::SubscriptOperatorSetDecl *subscriptOperatorSetDecl) {
    processFunctionDecl(subscriptOperatorSetDecl);
}

void gulc::CodeProcessor::processTemplateFunctionDecl(gulc::TemplateFunctionDecl* templateFunctionDecl) {
    // TODO: What would we do here?
}

void gulc::CodeProcessor::processTemplateStructDecl(gulc::TemplateStructDecl* templateStructDecl) {
    Decl* oldContainer = _currentContainer;
    _currentContainer = templateStructDecl;

    // TODO: What would we do here?

    _currentContainer = oldContainer;
}

void gulc::CodeProcessor::processTemplateTraitDecl(gulc::TemplateTraitDecl* templateTraitDecl) {
    Decl* oldContainer = _currentContainer;
    _currentContainer = templateTraitDecl;

    _currentContainer = oldContainer;
}

void gulc::CodeProcessor::processTraitDecl(gulc::TraitDecl* traitDecl) {
    Decl* oldContainer = _currentContainer;
    _currentContainer = traitDecl;

    for (Decl* member : traitDecl->ownedMembers()) {
        processDecl(member);
    }

    _currentContainer = oldContainer;
}

void gulc::CodeProcessor::processVariableDecl(gulc::VariableDecl* variableDecl) {
    // NOTE: For member variables we still need to process the initial value...
    if (variableDecl->hasInitialValue()) {
        processExpr(variableDecl->initialValue);
    }
}

void gulc::CodeProcessor::processBaseConstructorCall(gulc::StructDecl* structDecl,
                                                     gulc::FunctionCallExpr*& functionCallExpr) {
    for (LabeledArgumentExpr* labeledArgumentExpr : functionCallExpr->arguments) {
        processLabeledArgumentExpr(labeledArgumentExpr);
    }

    if (llvm::isa<IdentifierExpr>(functionCallExpr->functionReference)) {
        auto checkIdentifierExpr = llvm::dyn_cast<IdentifierExpr>(functionCallExpr->functionReference);

        ConstructorDecl* foundConstructor = nullptr;
        std::vector<MatchingDecl> foundMatches;

        if (checkIdentifierExpr->identifier().name() == "self") {
            // If there aren't any arguments we'll save ourselves the search and just use the default constructor
            if (functionCallExpr->arguments.empty()) {
                foundConstructor = structDecl->cachedDefaultConstructor;
            }

            if (foundConstructor == nullptr) {
                fillListOfMatchingConstructors(structDecl, functionCallExpr->arguments, foundMatches);
            }
        } else if (checkIdentifierExpr->identifier().name() == "base") {
            if (structDecl->baseStruct == nullptr) {
                printError("struct `" + structDecl->identifier().name() + "` does not have a `base` struct!",
                           checkIdentifierExpr->startPosition(), checkIdentifierExpr->endPosition());
                return;
            }

            // If there aren't any arguments we'll save ourselves the search and just use the default constructor
            if (functionCallExpr->arguments.empty()) {
                foundConstructor = structDecl->baseStruct->cachedDefaultConstructor;
            }

            if (foundConstructor == nullptr) {
                fillListOfMatchingConstructors(structDecl->baseStruct, functionCallExpr->arguments, foundMatches);
            }
        } else {
            printError("unknown base constructor name found, expected `self` or `base`!",
                       checkIdentifierExpr->startPosition(), checkIdentifierExpr->endPosition());
            return;
        }

        if (foundConstructor == nullptr) {
            if (foundMatches.size() > 1) {
                for (MatchingDecl& checkMatch : foundMatches) {
                    if (checkMatch.kind == MatchingDecl::Kind::Match) {
                        if (foundConstructor == nullptr) {
                            foundConstructor = llvm::dyn_cast<ConstructorDecl>(checkMatch.matchingDecl);
                        } else {
                            printError("base constructor call is ambiguous!",
                                       functionCallExpr->startPosition(), functionCallExpr->endPosition());
                            return;
                        }
                    }
                }
            } else if (!foundMatches.empty()) {
                foundConstructor = llvm::dyn_cast<ConstructorDecl>(foundMatches[0].matchingDecl);
            }
        }

        if (foundConstructor == nullptr) {
            printError("base constructor was not found!",
                       functionCallExpr->startPosition(), functionCallExpr->endPosition());
            return;
        }

        auto constructorReferenceExpr = new ConstructorReferenceExpr(
                functionCallExpr->functionReference->startPosition(),
                functionCallExpr->functionReference->endPosition(),
                foundConstructor
            );
        processConstructorReferenceExpr(constructorReferenceExpr);
        auto constructorCallExpr = new ConstructorCallExpr(
                constructorReferenceExpr,
                getCurrentSelfRef(constructorReferenceExpr->startPosition(), constructorReferenceExpr->endPosition()),
                functionCallExpr->arguments,
                functionCallExpr->startPosition(),
                functionCallExpr->endPosition()
            );
        // We steal these...
        functionCallExpr->arguments.clear();

        processConstructorCallExpr(constructorCallExpr);

        // We handle argument casting and conversion no matter what. The below function will handle
        // converting from lvalue to rvalue, casting, and other rules for us.
        handleArgumentCasting(foundConstructor->parameters(), constructorCallExpr->arguments);

        // Delete the old function call and replace it with the new constructor call
        delete functionCallExpr;
        functionCallExpr = constructorCallExpr;
    }
}

gulc::ConstructorCallExpr* gulc::CodeProcessor::createBaseMoveCopyConstructorCall(ConstructorDecl* baseConstructorDecl,
                                                                                  ParameterDecl* otherParameterDecl,
                                                                                  TextPosition startPosition,
                                                                                  TextPosition endPosition) {
    // For `move` and `copy` constructors we have to manually create their calls, we can't use
    // `processBaseConstructorCall` due to ambiguity.
    auto constructorReferenceExpr = new ConstructorReferenceExpr(
            startPosition,
            endPosition,
            baseConstructorDecl
    );
    processConstructorReferenceExpr(constructorReferenceExpr);

    auto labeledOtherArgument = new LabeledArgumentExpr(
            Identifier({}, {}, "_"),
            new IdentifierExpr(otherParameterDecl->identifier(), {})
    );
    processLabeledArgumentExpr(labeledOtherArgument);

    auto constructorCallExpr = new ConstructorCallExpr(
            constructorReferenceExpr,
            getCurrentSelfRef(constructorReferenceExpr->startPosition(),
                              constructorReferenceExpr->endPosition()),
            {
                    labeledOtherArgument
            },
            startPosition,
            endPosition
    );

    processConstructorCallExpr(constructorCallExpr);

    // We handle argument casting and conversion no matter what. The below function will handle
    // converting from lvalue to rvalue, casting, and other rules for us.
    handleArgumentCasting(baseConstructorDecl->parameters(), constructorCallExpr->arguments);

    return constructorCallExpr;
}

gulc::CompoundStmt* gulc::CodeProcessor::createStructMemberInitializersCompoundStmt(gulc::StructDecl* structDecl) {
    std::vector<Stmt*> initializerStatements;

    // TODO: Check `PropertyDecl` as well. `PropertyDecl` allows for default generation which means we will have to
    //       create a `var` behind the scenes which we will also need to initialize here.
    for (Decl* initializeDecl : structDecl->ownedMembers()) {
        if (llvm::isa<VariableDecl>(initializeDecl)) {
            auto initializeVariableDecl = llvm::dyn_cast<VariableDecl>(initializeDecl);
            Expr* initialValue = nullptr;

            switch (initializeVariableDecl->type->getTypeKind()) {
                case Type::Kind::Bool:
                    // false
                    initialValue = new BoolLiteralExpr({}, {}, false);
                    break;
                case Type::Kind::BuiltIn:
                    // 0
                    initialValue = new ValueLiteralExpr(ValueLiteralExpr::LiteralType::Integer, "0", "", {}, {});
                    break;
                case Type::Kind::Enum:
                    // TODO: Should this just be the first value of the enum?
                    printError("[INTERNAL] enums not yet supported!",
                               structDecl->startPosition(), structDecl->endPosition());
                    break;
                case Type::Kind::FlatArray:
                    // TODO: What should this be?
                    printError("[INTERNAL] flat arrays not yet supported!",
                               structDecl->startPosition(), structDecl->endPosition());
                    break;
                case Type::Kind::FunctionPointer:
                    // TODO: What should this be?
                    printError("[INTERNAL] function pointers not yet supported!",
                               structDecl->startPosition(), structDecl->endPosition());
                    break;
                case Type::Kind::Pointer:
                    // null
                    // TODO: Add `NullLiteralExpr`...
                    printError("[INTERNAL] pointers not yet supported!",
                               structDecl->startPosition(), structDecl->endPosition());
                    break;
                case Type::Kind::Reference:
                    // Cannot initialize by default, let the error checking pass handle it. User might assign it
                    // instead.
                    break;
                case Type::Kind::Trait:
                    // TODO: What should this be?
                    printError("[INTERNAL] traits not yet supported!",
                               structDecl->startPosition(), structDecl->endPosition());
                    break;
                default:
                    printError("[INTERNAL] unsupported type found in "
                               "`CodeProcessor::createStructMemberInitializersCompoundStmt`!",
                               structDecl->startPosition(), structDecl->endPosition());
                    break;
            }

            if (initialValue != nullptr) {
                initializerStatements.push_back(
                        new AssignmentOperatorExpr(
                                // NOTE: Without the default `self.` we could accidentally fill parameters.
                                new MemberAccessCallExpr(
                                        false,
                                        new IdentifierExpr(
                                                Identifier({}, {}, "self"),
                                                {}
                                        ),
                                        new IdentifierExpr(
                                                Identifier({}, {}, initializeVariableDecl->identifier().name()),
                                                {}
                                        )
                                ),
                                initialValue,
                                {},
                                {}
                        )
                );
            }
        }
    }

    return new CompoundStmt(
            initializerStatements,
            {},
            {}
        );
}

//=====================================================================================================================
// STATEMENTS
//=====================================================================================================================
void gulc::CodeProcessor::processStmt(gulc::Stmt*& stmt) {
    switch (stmt->getStmtKind()) {
        case Stmt::Kind::Break:
            processBreakStmt(llvm::dyn_cast<BreakStmt>(stmt));
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

        case Stmt::Kind::Expr: {
            auto castedExpr = llvm::dyn_cast<Expr>(stmt);
            processExpr(castedExpr);
            stmt = castedExpr;
            break;
        }

        default:
            printError("[INTERNAL ERROR] unsupported `Stmt` found in `CodeProcessor::processStmt`!",
                       stmt->startPosition(), stmt->endPosition());
            break;
    }
}

void gulc::CodeProcessor::processBreakStmt(gulc::BreakStmt* breakStmt) {
    if (breakStmt->hasBreakLabel()) {
        addUnresolvedLabel(breakStmt->breakLabel().name());
    }
}

void gulc::CodeProcessor::processCaseStmt(gulc::CaseStmt* caseStmt) {
    if (!caseStmt->isDefault()) {
        processExpr(caseStmt->condition);

        caseStmt->condition = convertLValueToRValue(caseStmt->condition);
        caseStmt->condition = dereferenceReference(caseStmt->condition);
    }

    for (Stmt* statement : caseStmt->body) {
        processStmt(statement);
    }
}

void gulc::CodeProcessor::processCatchStmt(gulc::CatchStmt* catchStmt) {
    processCompoundStmt(catchStmt->body());
}

void gulc::CodeProcessor::processCompoundStmt(gulc::CompoundStmt* compoundStmt) {
    std::size_t oldLocalVariableCount = _localVariables.size();

    for (Stmt*& statement : compoundStmt->statements) {
        processStmt(statement);
    }

    _localVariables.resize(oldLocalVariableCount);
}

void gulc::CodeProcessor::processContinueStmt(gulc::ContinueStmt* continueStmt) {
    if (continueStmt->hasContinueLabel()) {
        addUnresolvedLabel(continueStmt->continueLabel().name());
    }
}

void gulc::CodeProcessor::processDoCatchStmt(gulc::DoCatchStmt* doCatchStmt) {
    processCompoundStmt(doCatchStmt->body());

    for (CatchStmt* catchStmt : doCatchStmt->catchStatements()) {
        processCatchStmt(catchStmt);
    }

    if (doCatchStmt->hasFinallyStatement()) {
        processCompoundStmt(doCatchStmt->finallyStatement());
    }
}

void gulc::CodeProcessor::processDoWhileStmt(gulc::DoWhileStmt* doWhileStmt) {
    processCompoundStmt(doWhileStmt->body());
    processExpr(doWhileStmt->condition);

    doWhileStmt->condition = convertLValueToRValue(doWhileStmt->condition);
    doWhileStmt->condition = dereferenceReference(doWhileStmt->condition);
}

void gulc::CodeProcessor::processForStmt(gulc::ForStmt* forStmt) {
    std::size_t oldLocalVariableCount = _localVariables.size();

    if (forStmt->init != nullptr) {
        processExpr(forStmt->init);
    }

    if (forStmt->condition != nullptr) {
        processExpr(forStmt->condition);

        forStmt->condition = convertLValueToRValue(forStmt->condition);
        forStmt->condition = dereferenceReference(forStmt->condition);
    }

    if (forStmt->iteration != nullptr) {
        processExpr(forStmt->iteration);
    }

    processCompoundStmt(forStmt->body());

    _localVariables.resize(oldLocalVariableCount);
}

void gulc::CodeProcessor::processGotoStmt(gulc::GotoStmt* gotoStmt) {
    addUnresolvedLabel(gotoStmt->label().name());
}

void gulc::CodeProcessor::processIfStmt(gulc::IfStmt* ifStmt) {
    processExpr(ifStmt->condition);
    processCompoundStmt(ifStmt->trueBody());

    if (ifStmt->hasFalseBody()) {
        // NOTE: `falseBody` can only be `IfStmt` or `CompoundStmt`
        if (llvm::isa<IfStmt>(ifStmt->falseBody())) {
            processIfStmt(llvm::dyn_cast<IfStmt>(ifStmt->falseBody()));
        } else if (llvm::isa<CompoundStmt>(ifStmt->falseBody())) {
            processCompoundStmt(llvm::dyn_cast<CompoundStmt>(ifStmt->falseBody()));
        }
    }

    ifStmt->condition = convertLValueToRValue(ifStmt->condition);
    ifStmt->condition = dereferenceReference(ifStmt->condition);
}

void gulc::CodeProcessor::processLabeledStmt(gulc::LabeledStmt* labeledStmt) {
    labeledStmt->currentNumLocalVariables = _localVariables.size();

    _currentFunction->labeledStmts.insert({labeledStmt->label().name(), labeledStmt});

    labelResolved(labeledStmt->label().name());

    processStmt(labeledStmt->labeledStmt);
}

void gulc::CodeProcessor::processReturnStmt(gulc::ReturnStmt* returnStmt) {
    if (returnStmt->returnValue != nullptr) {
        processExpr(returnStmt->returnValue);

        // NOTE: We don't implicitly convert from `lvalue` to reference here if the function returns a reference.
        //       To return a reference you MUST specify `ref {value}`, we don't do implicitly referencing like in C++.
        returnStmt->returnValue = convertLValueToRValue(returnStmt->returnValue);
        returnStmt->returnValue = dereferenceReference(returnStmt->returnValue);
    }
}

void gulc::CodeProcessor::processSwitchStmt(gulc::SwitchStmt* switchStmt) {
    processExpr(switchStmt->condition);

    for (CaseStmt*& caseStmt : switchStmt->cases) {
        processCaseStmt(caseStmt);
    }
}

void gulc::CodeProcessor::processWhileStmt(gulc::WhileStmt* whileStmt) {
    processExpr(whileStmt->condition);
    processCompoundStmt(whileStmt->body());
}

void gulc::CodeProcessor::labelResolved(std::string const& labelName) {
    _labelNames.insert_or_assign(labelName, true);
}

void gulc::CodeProcessor::addUnresolvedLabel(std::string const& labelName) {
    if (_labelNames.find(labelName) == _labelNames.end()) {
        _labelNames.insert({ labelName, false });
    }
}

//=====================================================================================================================
// EXPRESSIONS
//=====================================================================================================================
void gulc::CodeProcessor::processExpr(gulc::Expr*& expr) {
    switch (expr->getExprKind()) {
        case Expr::Kind::ArrayLiteral:
            processArrayLiteralExpr(llvm::dyn_cast<ArrayLiteralExpr>(expr));
            break;
        case Expr::Kind::As:
            processAsExpr(llvm::dyn_cast<AsExpr>(expr));
            break;
        case Expr::Kind::AssignmentOperator: {
            auto assignmentOperatorExpr = llvm::dyn_cast<AssignmentOperatorExpr>(expr);
            processAssignmentOperatorExpr(assignmentOperatorExpr);
            expr = assignmentOperatorExpr;
            break;
        }
        case Expr::Kind::BoolLiteral:
            processBoolLiteralExpr(llvm::dyn_cast<BoolLiteralExpr>(expr));
            break;
        case Expr::Kind::CheckExtendsType:
            processCheckExtendsTypeExpr(llvm::dyn_cast<CheckExtendsTypeExpr>(expr));
            break;
        case Expr::Kind::FunctionCall: {
            auto functionCallExpr = llvm::dyn_cast<FunctionCallExpr>(expr);
            processFunctionCallExpr(functionCallExpr);
            expr = functionCallExpr;
            break;
        }
        case Expr::Kind::Has:
            processHasExpr(llvm::dyn_cast<HasExpr>(expr));
            break;
        case Expr::Kind::Identifier:
            processIdentifierExpr(expr);
            break;
        case Expr::Kind::InfixOperator:
            processInfixOperatorExpr(llvm::dyn_cast<InfixOperatorExpr>(expr));
            break;
        case Expr::Kind::Is:
            processIsExpr(llvm::dyn_cast<IsExpr>(expr));
            break;
        case Expr::Kind::LabeledArgument:
            printError("argument label found outside of function call, this is not supported! (how did you even do that?!)",
                       expr->startPosition(), expr->endPosition());
            break;
        case Expr::Kind::MemberAccessCall:
            processMemberAccessCallExpr(expr);
            break;
        case Expr::Kind::Paren:
            processParenExpr(llvm::dyn_cast<ParenExpr>(expr));
            break;
        case Expr::Kind::PostfixOperator: {
            auto postfixOperatorExpr = llvm::dyn_cast<PostfixOperatorExpr>(expr);
            processPostfixOperatorExpr(postfixOperatorExpr);
            expr = postfixOperatorExpr;
            break;
        }
        case Expr::Kind::PrefixOperator: {
            auto prefixOperatorExpr = llvm::dyn_cast<PrefixOperatorExpr>(expr);
            processPrefixOperatorExpr(prefixOperatorExpr);
            expr = prefixOperatorExpr;
            break;
        }
        case Expr::Kind::Ref:
            processRefExpr(llvm::dyn_cast<RefExpr>(expr));
            break;
        case Expr::Kind::SubscriptCall:
            processSubscriptCallExpr(expr);
            break;
        case Expr::Kind::TemplateConstRef:
            processTemplateConstRefExpr(llvm::dyn_cast<TemplateConstRefExpr>(expr));
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
            printError("[INTERNAL ERROR] unsupported `Expr` found in `CodeProcessor::processExpr`!",
                       expr->startPosition(), expr->endPosition());
            break;
    }
}

void gulc::CodeProcessor::processArrayLiteralExpr(gulc::ArrayLiteralExpr* arrayLiteralExpr) {
    // TODO: I really like the syntax Swift uses such as:
    //           dependencies: [
    //               .package(url: "", from: "")
    //           ]
    //       I believe they're using what I'm calling `enum union`s for this, I'd really like to support that
    //       (by "that" I mean `enum` resolution without specifying it. I.e. if `dependencies` expects `enum Example`
    //       you don't have to write `Example.package` you can just write `.package`, it will be deduced from the
    //       context. I think Java also supports this in some contexts such as `switch` cases? Maybe I'm confusing Java
    //       with something else.)
    // TODO: Also, Ghoul is NOT like C and C++. An array literal is NOT `*T` it is `[T; N]` where `N` is the length of
    //       the flat array. To support an implicit cast from `[T; N]` to any of our `std.containers` we will need to
    //       support:
    //           `public operator implicit<T, const N: usize>(_ flatArray: [T; N]) -> list<T> {}`
    //       I'm not sure if we're going the `operator implicit` route or not but you get the idea for the need for
    //       type inference even in operators.
    Type* indexType = nullptr;

    TypeCompareUtil typeCompareUtil;

    for (Expr*& index : arrayLiteralExpr->indexes) {
        processExpr(index);

        if (indexType == nullptr) {
            if (llvm::isa<ReferenceType>(index->valueType)) {
                indexType = llvm::dyn_cast<ReferenceType>(index->valueType)->nestedType->deepCopy();
            } else {
                indexType = index->valueType->deepCopy();
            }
        } else {
            auto checkType = index->valueType;

            if (llvm::isa<ReferenceType>(checkType)) {
                checkType = llvm::dyn_cast<ReferenceType>(checkType)->nestedType;
            }

            // TODO: Should we try to support implicit casting within array literals? Or at least some attempt at
            //       finding a common base between all types? Or just let the user deal with it? I vote on the user
            //       dealing with it to prevent as much unexpected behaviour as possible.
            if (!typeCompareUtil.compareAreSame(indexType, checkType)) {
                printError("array literal indexes must all be the same type!",
                           arrayLiteralExpr->startPosition(), arrayLiteralExpr->endPosition());
            }
        }
    }

    // TODO: Wouldn't it be a better idea to make `flat array` a special special type that when its index type is `auto`
    //       then it is the expression containing the `flat array`'s duty to handle the type? Just a thought since there
    //       are special cases anyways such as `[]` and `[ @new ParentClass(), @new ChildClass() ]`
    arrayLiteralExpr->valueType = new FlatArrayType(
            Type::Qualifier::Unassigned, indexType,
            new ValueLiteralExpr(
                    ValueLiteralExpr::LiteralType::Integer,
                    std::to_string(arrayLiteralExpr->indexes.size()),
                    "", {}, {}));
}

void gulc::CodeProcessor::processAsExpr(gulc::AsExpr* asExpr) {
    processExpr(asExpr->expr);

    // TODO: How will we handle this? Check the types for cast support? And look for overloads?
    // TODO: Search extensions for an explicit cast overload from `expr->valueType` to `asType`
    //       Then check `expr->valueType` if it is a `struct`, `class`, `union`, or `enum` for a custom `as`/explicit
    //       cast

    Type* fromType = asExpr->expr->valueType;

    if (llvm::isa<ReferenceType>(fromType)) {
        fromType = llvm::dyn_cast<ReferenceType>(fromType)->nestedType;
    }

    if (!llvm::isa<BuiltInType>(fromType) || !llvm::isa<BuiltInType>(asExpr->asType)) {
        printError("no valid conversion from `" + fromType->toString() + "` to `" + asExpr->asType->toString() + "` was found!",
                   asExpr->startPosition(), asExpr->endPosition());
    }

    // At this point we have validated the built in cast is possible, we keep the `AsExpr` since it is our built in
    // explicit cast expression.
    asExpr->valueType = asExpr->asType->deepCopy();
}

void gulc::CodeProcessor::processAssignmentOperatorExpr(gulc::AssignmentOperatorExpr*& assignmentOperatorExpr) {
    processExpr(assignmentOperatorExpr->leftValue);
    processExpr(assignmentOperatorExpr->rightValue);

    Type* checkType = assignmentOperatorExpr->leftValue->valueType;

    if (llvm::isa<ReferenceType>(checkType)) {
        checkType = llvm::dyn_cast<ReferenceType>(checkType)->nestedType;
    }

    // If the left side is a `Struct` then we handle assignments differently. A struct assignment uses copy/move
    // constructors for assignment.
    if (llvm::isa<StructType>(checkType)) {
        if (assignmentOperatorExpr->hasNestedOperator()) {
            // TODO: Handle nested operator the same as our infix operator
            //       Also change `rightType` if the nested operator is overloaded.
        }

        // TODO: If `rightValue->valueType` isn't the same as `checkType` search for a valid cast

        // We only convert lvalue to rvalue if the type is a reference. We still need some kind of underlying
        // reference-type.
        if (llvm::isa<ReferenceType>(assignmentOperatorExpr->rightValue->valueType)) {
            assignmentOperatorExpr->rightValue = convertLValueToRValue(assignmentOperatorExpr->rightValue);
        }

        StructAssignmentOperatorExpr* newStructAssignment;

        // TODO: Default is `move`, not `copy`. Change to `move` when we handle it properly.
        if (assignmentOperatorExpr->hasNestedOperator()) {
            newStructAssignment = new StructAssignmentOperatorExpr(
                    StructAssignmentType::Copy,
                    assignmentOperatorExpr->leftValue,
                    assignmentOperatorExpr->rightValue,
                    assignmentOperatorExpr->nestedOperator(),
                    assignmentOperatorExpr->startPosition(),
                    assignmentOperatorExpr->endPosition()
            );
        } else {
            newStructAssignment = new StructAssignmentOperatorExpr(
                    StructAssignmentType::Copy,
                    assignmentOperatorExpr->leftValue,
                    assignmentOperatorExpr->rightValue,
                    assignmentOperatorExpr->startPosition(),
                    assignmentOperatorExpr->endPosition()
            );
        }

        newStructAssignment->temporaryValues = assignmentOperatorExpr->temporaryValues;

        // Remove any stolen pointers and delete the old assignment
        assignmentOperatorExpr->leftValue = nullptr;
        assignmentOperatorExpr->rightValue = nullptr;
        assignmentOperatorExpr->temporaryValues.clear();
        delete assignmentOperatorExpr;

        newStructAssignment->valueType = checkType->deepCopy();
        newStructAssignment->valueType->setIsLValue(true);

        assignmentOperatorExpr = newStructAssignment;
        return;
    } else {
        if (assignmentOperatorExpr->hasNestedOperator()) {
            // TODO: Handle nested operator the same as our infix operator
            //       Also change `rightType` if the nested operator is overloaded.
        } else {
            assignmentOperatorExpr->rightValue = convertLValueToRValue(assignmentOperatorExpr->rightValue);
        }

        // TODO: Validate that `rightType` can be casted to `leftType` (if they aren't the same type)

        assignmentOperatorExpr->valueType = assignmentOperatorExpr->leftValue->valueType->deepCopy();
        // TODO: Is this right? I believe we'll end up returning the actual alloca from CodeGen so this seems right to me.
        assignmentOperatorExpr->valueType->setIsLValue(true);
    }
}

void gulc::CodeProcessor::processBoolLiteralExpr(gulc::BoolLiteralExpr* boolLiteralExpr) {
    boolLiteralExpr->valueType = new BoolType(Type::Qualifier::Immut, {}, {});
}

void gulc::CodeProcessor::processCallOperatorReferenceExpr(gulc::CallOperatorReferenceExpr* callOperatorReferenceExpr) {

}

void gulc::CodeProcessor::processCheckExtendsTypeExpr(gulc::CheckExtendsTypeExpr* checkExtendsTypeExpr) {
    // TODO: Should this be support at this point? Should we do `const` solving here?
}

void gulc::CodeProcessor::processConstructorCallExpr(gulc::ConstructorCallExpr* constructorCallExpr) {

}

void gulc::CodeProcessor::processConstructorReferenceExpr(gulc::ConstructorReferenceExpr* constructorReferenceExpr) {

}

void gulc::CodeProcessor::processEnumConstRefExpr(gulc::EnumConstRefExpr* enumConstRefExpr) {
    auto enumType = new EnumType(
            Type::Qualifier::Immut,
            llvm::dyn_cast<EnumDecl>(enumConstRefExpr->enumConst()->container),
            {},
            {}
        );

    enumConstRefExpr->valueType = enumType;
    enumConstRefExpr->valueType->setIsLValue(true);
}

void gulc::CodeProcessor::processFunctionCallExpr(gulc::FunctionCallExpr*& functionCallExpr) {
    // NOTE: We need to process the parameters and template parameters first. They might be modified for the call later...
    for (LabeledArgumentExpr* argument : functionCallExpr->arguments) {
        processLabeledArgumentExpr(argument);
    }

    if (llvm::isa<IdentifierExpr>(functionCallExpr->functionReference)) {
        auto identifierExpr = llvm::dyn_cast<IdentifierExpr>(functionCallExpr->functionReference);
        std::string const& findName = identifierExpr->identifier().name();

        if (identifierExpr->hasTemplateArguments()) {
            // If there are template parameters we have to process them first.
            for (Expr*& templateArgument : identifierExpr->templateArguments()) {
                processExpr(templateArgument);
            }
        } else {
            // First we process the template parameters and reduce them into their `const` form so they're usable...
            for (Expr*& templateArgument : identifierExpr->templateArguments()) {
                processExpr(templateArgument);
            }

            // TODO: Search local variables and parameters
            for (VariableDeclExpr* checkLocalVariable : _localVariables) {
                if (findName == checkLocalVariable->identifier().name()) {
                    // TODO: Create a `RefLocalVariableExpr` and exit all loops, delete the old `functionReference`
                    printError("using the `()` operator on local variables not yet supported!",
                               functionCallExpr->startPosition(), functionCallExpr->endPosition());
                    break;
                }
            }

            if (_currentParameters != nullptr) {
                for (ParameterDecl* checkParameter : *_currentParameters) {
                    if (findName == checkParameter->identifier().name()) {
                        // TODO: Create a `RefParameterExpr` and exit all loops, delete the old `functionReference`
                        printError("using the `()` operator on parameters not yet supported!",
                                   functionCallExpr->startPosition(), functionCallExpr->endPosition());
                        break;
                    }
                }
            }

            for (std::vector<TemplateParameterDecl*>* checkTemplateParameterList : _allTemplateParameters) {
                for (TemplateParameterDecl* checkTemplateParameter : *checkTemplateParameterList) {
                    if (findName == checkTemplateParameter->identifier().name()) {
                        // TODO: Create a `RefTemplateParameterConstExpr` or `TypeExpr` and exit all loops, delete the
                        //       old `functionReference`
                        if (checkTemplateParameter->templateParameterKind() == TemplateParameterDecl::TemplateParameterKind::Typename) {
                            // TODO: Check to make sure the `typename` has an `init` with the provided parameters
                            // NOTE: We CANNOT check for any `extensions` on the inherited types, we need to use the
                            //       actual type being supplied to the template for the constructor. The extension
                            //       constructor might not exist for the provided type.
                            // NOTE: We also cannot support checking any `inheritedTypes` for constructors, the
                            //       supplied type isn't guaranteed to implement the same constructors...
                            // TODO: We need to support `where T has init(...)` for this to work properly...
                            printError("template type `" + checkTemplateParameter->identifier().name() + "` does not have a constructor matching the provided parameters!",
                                       functionCallExpr->functionReference->startPosition(),
                                       functionCallExpr->functionReference->endPosition());
                        } else {
                            // TODO: Check to make sure the `const` is a function pointer OR a type that implements the
                            //       `call` operator
                            //if (llvm::isa<FunctionPointerType>(checkTemplateParameter->constType)) {
                            //    TODO: Support `FunctionPointerType`
                            //} else {
                            std::vector<MatchingDecl> matches;

                            // Check for matching `call` operators
                            if (fillListOfMatchingCallOperators(checkTemplateParameter->constType,
                                                                functionCallExpr->arguments, matches)) {
                                // TODO: Check the list of call matches
                                printError("using the `()` operator on template const parameters not yet supported!",
                                           functionCallExpr->startPosition(), functionCallExpr->endPosition());
                            }
                        }
                        break;
                    }
                }
            }
        }

        // Search members of our current containers
        if (_currentContainer != nullptr) {
            // Search the members of our current container
            std::vector<MatchingDecl> matches;

            if (fillListOfMatchingFunctors(_currentContainer, identifierExpr, functionCallExpr->arguments,
                    matches)) {
                Decl* foundDecl = validateAndReturnMatchingFunction(functionCallExpr, matches);

                if (foundDecl != nullptr) {
                    Expr* newFunctionReference = nullptr;
                    Type* functionReturnType = nullptr;

                    if (llvm::isa<FunctionDecl>(foundDecl)) {
                        auto foundFunction = llvm::dyn_cast<FunctionDecl>(foundDecl);

                        functionReturnType = foundFunction->returnType->deepCopy();

                        // We handle argument casting and conversion no matter what. The below function will handle
                        // converting from lvalue to rvalue, casting, and other rules for us.
                        handleArgumentCasting(foundFunction->parameters(), functionCallExpr->arguments);

                        if (foundFunction->isAnyVirtual()) {
                            // TODO: Find the vtable index and create a `VTableFunctionReferenceExpr`
                            if (!llvm::isa<StructDecl>(foundFunction->container)) {
                                printError("[INTERNAL] virtual function found within a non-struct in `processFunctionCallExpr`!",
                                           functionCallExpr->startPosition(), functionCallExpr->endPosition());
                            }

                            auto searchStructVTable = llvm::dyn_cast<StructDecl>(foundFunction->container);
                            std::size_t vtableIndex = 0;
                            bool foundVTableIndex = false;

                            for (std::size_t i = 0; i < searchStructVTable->vtable.size(); ++i) {
                                if (searchStructVTable->vtable[i] == foundFunction) {
                                    vtableIndex = i;
                                    foundVTableIndex = true;
                                    break;
                                }
                            }

                            if (!foundVTableIndex) {
                                printError("[INTERNAL] virtual function not found within the container struct's vtable!",
                                           functionCallExpr->startPosition(), functionCallExpr->endPosition());
                            }

                            // We found the vtable index so we create a `VTableFunctionReferenceExpr` for the new
                            // `MemberFunctionCallExpr` that will be created...
                            newFunctionReference =
                                    new VTableFunctionReferenceExpr(
                                            functionCallExpr->functionReference->startPosition(),
                                            functionCallExpr->functionReference->endPosition(),
                                            searchStructVTable, vtableIndex);
                        } else {
                            newFunctionReference = new FunctionReferenceExpr(
                                    functionCallExpr->functionReference->startPosition(),
                                    functionCallExpr->functionReference->endPosition(),
                                    foundFunction);
                        }
                    } else if (llvm::isa<TemplateFunctionDecl>(foundDecl)) {
                        auto foundTemplateFunction = llvm::dyn_cast<TemplateFunctionDecl>(foundDecl);

                        // TODO: For this to work we will need to instantiate the template function. If there isn't
                        //       already an instantiation with the specified template parameters we will have to
                        //       backtrack back to an earlier process stage for that instantiation.
                        //       We should create a special utility to handle that for us
                        printError("calling template functions not yet supported!",
                                   functionCallExpr->startPosition(), functionCallExpr->endPosition());
                    } else {
                        printError("[INTERNAL ERROR] unknown found decl in `processFunctionCallExpr`!",
                                   functionCallExpr->startPosition(), functionCallExpr->endPosition());
                    }

                    // If the current container is a `StructDecl`, `EnumDecl`, or `TraitDecl` we replace the function
                    // call with a member function call
                    if (llvm::isa<StructDecl>(_currentContainer) || llvm::isa<TraitDecl>(_currentContainer) ||
                            llvm::isa<EnumDecl>(_currentContainer)) {
                        // Replace the `FunctionCallExpr` with a new `MemberFunctionCallExpr`
                        Expr* selfRef = getCurrentSelfRef(functionCallExpr->startPosition(),
                                                          functionCallExpr->endPosition());
                        auto newFunctionCallExpr = new MemberFunctionCallExpr(newFunctionReference, selfRef,
                                                                              functionCallExpr->arguments,
                                                                              functionCallExpr->startPosition(),
                                                                              functionCallExpr->endPosition());

                        // We steal the parameters
                        functionCallExpr->arguments.clear();
                        // Delete the old expression
                        delete functionCallExpr;
                        // Replace it with the new found `MemberFunctionCallExpr`...
                        functionCallExpr = newFunctionCallExpr;

                        functionCallExpr->valueType = functionReturnType;
                        functionCallExpr->valueType->setIsLValue(true);

                        return;
                    } else if (llvm::isa<NamespaceDecl>(_currentContainer)) {
                        // TODO: Constructor
                        // We keep the `functionCallExpr` and only replace its `functionReference`
                        delete functionCallExpr->functionReference;
                        functionCallExpr->functionReference = newFunctionReference;

                        functionCallExpr->valueType = functionReturnType;
                        functionCallExpr->valueType->setIsLValue(true);

                        return;
                    } else {
                        printError("[INTERNAL] unsupported container decl found within `CodeProcessor::processFunctionCallExpr`!",
                                   functionCallExpr->startPosition(), functionCallExpr->endPosition());
                    }
                }
            }
        }

        // Search the current file if we make it this far
        {
            std::vector<MatchingDecl> matches;

            if (fillListOfMatchingFunctors(_currentFile, identifierExpr, functionCallExpr->arguments,
                                           matches)) {
                Decl* foundDecl = validateAndReturnMatchingFunction(functionCallExpr, matches);

                if (foundDecl != nullptr) {
                    if (llvm::isa<ConstructorDecl>(foundDecl)) {
                        auto foundConstructor = llvm::dyn_cast<ConstructorDecl>(foundDecl);
                        auto constructorReferenceExpr = new ConstructorReferenceExpr(
                                identifierExpr->startPosition(),
                                identifierExpr->endPosition(),
                                foundConstructor
                            );
                        auto constructorCallExpr = new ConstructorCallExpr(
                                constructorReferenceExpr,
                                nullptr,
                                functionCallExpr->arguments,
                                functionCallExpr->startPosition(),
                                functionCallExpr->endPosition()
                            );

                        // We handle argument casting and conversion no matter what. The below function will handle
                        // converting from lvalue to rvalue, casting, and other rules for us.
                        handleArgumentCasting(foundConstructor->parameters(), functionCallExpr->arguments);

                        // We steal the parameters...
                        functionCallExpr->arguments.clear();

                        // Delete the old function call and replace it with our constructor call
                        delete functionCallExpr;
                        functionCallExpr = constructorCallExpr;

                        switch (foundDecl->container->getDeclKind()) {
                            case Decl::Kind::TemplateStructInst:
                            case Decl::Kind::Struct:
                                functionCallExpr->valueType = new StructType(Type::Qualifier::Unassigned,
                                        llvm::dyn_cast<StructDecl>(foundDecl->container), {}, {});
                                functionCallExpr->valueType->setIsLValue(true);
                                break;
                            case Decl::Kind::TemplateTraitInst:
                            case Decl::Kind::Trait:
                                functionCallExpr->valueType = new TraitType(Type::Qualifier::Unassigned,
                                        llvm::dyn_cast<TraitDecl>(foundDecl->container), {}, {});
                                functionCallExpr->valueType->setIsLValue(true);
                                break;
                            default:
                                printError("unknown constructor type found in `CodeProcessor::processFunctionCallExpr`!",
                                           functionCallExpr->startPosition(), functionCallExpr->endPosition());
                        }

                        return;
                    } else if (llvm::isa<FunctionDecl>(foundDecl)) {
                        auto foundFunction = llvm::dyn_cast<FunctionDecl>(foundDecl);

                        // We handle argument casting and conversion no matter what. The below function will handle
                        // converting from lvalue to rvalue, casting, and other rules for us.
                        handleArgumentCasting(foundFunction->parameters(), functionCallExpr->arguments);

                        // Delete the old function reference and replace it with the new one.
                        delete functionCallExpr->functionReference;
                        functionCallExpr->functionReference = createStaticFunctionReference(functionCallExpr, foundDecl);

                        functionCallExpr->valueType = llvm::dyn_cast<FunctionDecl>(foundDecl)->returnType->deepCopy();
                        functionCallExpr->valueType->setIsLValue(true);

                        return;
                    } else {
                        printError("unknown decl found in `CodeProcessor::processFunctionCallExpr`!",
                                   functionCallExpr->startPosition(), functionCallExpr->endPosition());
                    }
                }
            }
        }

        // Search the imports if we haven't found anything yet
        {
            std::vector<MatchingDecl> matches;

            // Search ALL imported namespaces (if you import two namespaces that have functions with the same signature
            // you need to specify the absolute path for the function you want)
            for (ImportDecl* checkImport : _currentFile->imports) {
                fillListOfMatchingFunctors(checkImport->pointToNamespace, identifierExpr,
                        functionCallExpr->arguments, matches);
            }

            // If there are any matches, validate the matches and set the new function reference...
            if (!matches.empty()) {
                Decl* foundDecl = validateAndReturnMatchingFunction(functionCallExpr, matches);

                if (foundDecl != nullptr) {
                    if (llvm::isa<ConstructorDecl>(foundDecl)) {
                        auto foundConstructor = llvm::dyn_cast<ConstructorDecl>(foundDecl);
                        auto constructorReferenceExpr = new ConstructorReferenceExpr(
                                identifierExpr->startPosition(),
                                identifierExpr->endPosition(),
                                llvm::dyn_cast<ConstructorDecl>(foundDecl)
                            );
                        auto constructorCallExpr = new ConstructorCallExpr(
                                constructorReferenceExpr,
                                nullptr,
                                functionCallExpr->arguments,
                                functionCallExpr->startPosition(),
                                functionCallExpr->endPosition()
                            );

                        // We handle argument casting and conversion no matter what. The below function will handle
                        // converting from lvalue to rvalue, casting, and other rules for us.
                        handleArgumentCasting(foundConstructor->parameters(), functionCallExpr->arguments);

                        // We steal the parameters...
                        functionCallExpr->arguments.clear();

                        // Delete the old function call and replace it with our constructor call
                        delete functionCallExpr;
                        functionCallExpr = constructorCallExpr;

                        switch (foundDecl->container->getDeclKind()) {
                            case Decl::Kind::TemplateStructInst:
                            case Decl::Kind::Struct:
                                functionCallExpr->valueType = new StructType(Type::Qualifier::Unassigned,
                                        llvm::dyn_cast<StructDecl>(foundDecl->container), {}, {});
                                functionCallExpr->valueType->setIsLValue(true);
                                break;
                            case Decl::Kind::TemplateTraitInst:
                            case Decl::Kind::Trait:
                                functionCallExpr->valueType = new TraitType(Type::Qualifier::Unassigned,
                                        llvm::dyn_cast<TraitDecl>(foundDecl->container), {}, {});
                                functionCallExpr->valueType->setIsLValue(true);
                                break;
                            default:
                                printError("unknown constructor type found in `CodeProcessor::processFunctionCallExpr`!",
                                           functionCallExpr->startPosition(), functionCallExpr->endPosition());
                        }

                        return;
                    } else if (llvm::isa<FunctionDecl>(foundDecl)) {
                        auto foundFunction = llvm::dyn_cast<FunctionDecl>(foundDecl);

                        // We handle argument casting and conversion no matter what. The below function will handle
                        // converting from lvalue to rvalue, casting, and other rules for us.
                        handleArgumentCasting(foundFunction->parameters(), functionCallExpr->arguments);

                        // Delete the old function reference and replace it with the new one.
                        delete functionCallExpr->functionReference;
                        functionCallExpr->functionReference = createStaticFunctionReference(functionCallExpr, foundDecl);

                        functionCallExpr->valueType = llvm::dyn_cast<FunctionDecl>(foundDecl)->returnType->deepCopy();
                        functionCallExpr->valueType->setIsLValue(true);

                        return;
                    } else {
                        printError("unknown decl found in `CodeProcessor::processFunctionCallExpr`!",
                                   functionCallExpr->startPosition(), functionCallExpr->endPosition());
                    }
                }
            }
        }

        // If we've reached this point then the function wasn't found. Error out saying such.
        printError("function `" + identifierExpr->toString() + "` was not found!",
                   functionCallExpr->startPosition(), functionCallExpr->endPosition());
    } else if (llvm::isa<MemberAccessCallExpr>(functionCallExpr->functionReference)) {
        // TODO: Process the object reference normally, search the object for the function
        auto memberAccessCallExpr = llvm::dyn_cast<MemberAccessCallExpr>(functionCallExpr->functionReference);

        processExpr(memberAccessCallExpr->objectRef);

        // TODO: Support `NamespaceRefExpr`
        if (llvm::isa<TypeExpr>(memberAccessCallExpr->objectRef)) {
            if (memberAccessCallExpr->isArrowCall()) {
                printError("operator `->` cannot be applied to types!",
                           memberAccessCallExpr->startPosition(), memberAccessCallExpr->endPosition());
            }

            // NOTE: static `call` is not allowed as it would be confusing determining if the call is `init` or `call`
            auto checkType = llvm::dyn_cast<TypeExpr>(memberAccessCallExpr->objectRef)->type;

            bool isAmbiguous = false;
            Decl* foundMemberDecl = nullptr;

            switch (checkType->getTypeKind()) {
                case Type::Kind::Enum: {
                    printError("accessing enum static members not yet supported!",
                               functionCallExpr->startPosition(), functionCallExpr->endPosition());
                    break;
                }
                case Type::Kind::Struct: {
                    auto structDecl = llvm::dyn_cast<StructType>(checkType)->decl();
                    // TODO: Check if `memberAccessCallExpr->member` is `init` or `deinit` to handle constructor and
                    //       destructor explicit calls.
                    foundMemberDecl = findMatchingFunctorDecl(structDecl->allMembers, memberAccessCallExpr->member,
                                                              functionCallExpr->arguments, true, &isAmbiguous);
                    break;
                }
                case Type::Kind::Trait: {
                    auto traitDecl = llvm::dyn_cast<TraitType>(checkType)->decl();
                    foundMemberDecl = findMatchingFunctorDecl(traitDecl->allMembers, memberAccessCallExpr->member,
                                                              functionCallExpr->arguments, true, &isAmbiguous);
                    break;
                }
                default:
                    break;
            }

            if (isAmbiguous) {
                printError("expression `" + functionCallExpr->toString() + "` is ambiguous!",
                           functionCallExpr->startPosition(), functionCallExpr->endPosition());
            }

            if (foundMemberDecl == nullptr) {
                printError("member `" + memberAccessCallExpr->member->toString() + "` was not found in `" +
                           memberAccessCallExpr->objectRef->toString() + "` with the supplied arguments!",
                           functionCallExpr->startPosition(), functionCallExpr->endPosition());
            }

            switch (foundMemberDecl->getDeclKind()) {
                case Decl::Kind::Constructor: {
                    auto constructorDecl = llvm::dyn_cast<ConstructorDecl>(foundMemberDecl);

                    auto constructorReference = new ConstructorReferenceExpr(
                            memberAccessCallExpr->startPosition(),
                            memberAccessCallExpr->endPosition(),
                            constructorDecl
                        );
                    auto newExpr = new ConstructorCallExpr(
                            constructorReference,
                            nullptr,
                            functionCallExpr->arguments,
                            functionCallExpr->startPosition(),
                            functionCallExpr->endPosition()
                        );
                    processConstructorCallExpr(newExpr);

                    // We handle argument casting and conversion no matter what. The below function will handle
                    // converting from lvalue to rvalue, casting, and other rules for us.
                    handleArgumentCasting(constructorDecl->parameters(), functionCallExpr->arguments);

                    // NOTE: We steal the `arguments`
                    functionCallExpr->arguments.clear();
                    // Delete the old function call and replace it with the constructor call.
                    delete functionCallExpr;
                    functionCallExpr = newExpr;
                    break;
                }
                case Decl::Kind::Destructor: {
                    auto destructorDecl = llvm::dyn_cast<DestructorDecl>(foundMemberDecl);
                    printError("explicit `deinit` call not yet supported!",
                               functionCallExpr->startPosition(), functionCallExpr->endPosition());
                    break;
                }
                case Decl::Kind::TemplateFunctionInst:
                case Decl::Kind::Function: {
                    auto functionDecl = llvm::dyn_cast<FunctionDecl>(foundMemberDecl);

                    auto functionReference = new FunctionReferenceExpr(
                            memberAccessCallExpr->startPosition(),
                            memberAccessCallExpr->endPosition(),
                            functionDecl
                        );
                    processFunctionReferenceExpr(functionReference);

                    // We handle argument casting and conversion no matter what. The below function will handle
                    // converting from lvalue to rvalue, casting, and other rules for us.
                    handleArgumentCasting(functionDecl->parameters(), functionCallExpr->arguments);

                    // Delete the old reference and replace it with the new one.
                    delete functionCallExpr->functionReference;
                    functionCallExpr->functionReference = functionReference;
                    break;
                }
                case Decl::Kind::Property: {
                    auto propertyDecl = llvm::dyn_cast<PropertyDecl>(foundMemberDecl);

                    // TODO: How should we do this? Like so?
                    //        1. Create a `CallOperatorReferenceExpr` for holding the `CallOperatorDecl`
                    //        2. Create a `MemberFunctionCallExpr` that will take `memberAccessCallExpr->objectRef`
                    //           as the `self`
                    //        3. For `objectRef` since it is a `property` we will need to create a `PropertyRefExpr`
                    //        4. Create a `get` call to the `PropertyRefExpr`
                    //        5. Delete the old `functionCallExpr` and replace it with the member version?
                    // TODO: It isn't always a `call` operator, it could also be a `func` pointer
                    printError("using `call` operator on a `prop` `get` value not yet supported!",
                               functionCallExpr->startPosition(), functionCallExpr->endPosition());
                    break;
                }
                case Decl::Kind::Variable: {
                    auto variableDecl = llvm::dyn_cast<VariableDecl>(foundMemberDecl);

                    // TODO: Same as `prop`
                    printError("using `call` operator on a `var` not yet supported!",
                               functionCallExpr->startPosition(), functionCallExpr->endPosition());
                    break;
                }
                default:
                    printError("member `" + memberAccessCallExpr->toString() + "` call not supported!",
                               functionCallExpr->startPosition(), functionCallExpr->endPosition());
                    break;
            }
        } else {
            // TODO: Search for non-static functions (call operator, normal functions, properties returning function
            //       pointers, variables of function pointer type,
            auto checkType = memberAccessCallExpr->objectRef->valueType;

            bool isAmbiguous = false;
            Decl* foundDecl = nullptr;

            // TODO: Extensions.
            switch (checkType->getTypeKind()) {
                case Type::Kind::Enum: {
                    printError("accessing enum members from an instance not yet supported!",
                               functionCallExpr->startPosition(), functionCallExpr->endPosition());
                    break;
                }
                case Type::Kind::Struct: {
                    auto structDecl = llvm::dyn_cast<StructType>(checkType)->decl();
                    foundDecl = findMatchingFunctorDecl(
                            structDecl->allMembers,
                            memberAccessCallExpr->member,
                            functionCallExpr->arguments,
                            false,
                            &isAmbiguous
                        );
                    break;
                }
                case Type::Kind::Trait: {
                    auto traitDecl = llvm::dyn_cast<TraitType>(checkType)->decl();
                    foundDecl = findMatchingFunctorDecl(
                            traitDecl->allMembers,
                            memberAccessCallExpr->member,
                            functionCallExpr->arguments,
                            false,
                            &isAmbiguous
                        );
                    break;
                }
                default:
                    break;
            }

            switch (foundDecl->getDeclKind()) {
                case Decl::Kind::CallOperator: {
                    auto callOperatorDecl = llvm::dyn_cast<CallOperatorDecl>(foundDecl);

                    auto callOperatorReference = new CallOperatorReferenceExpr(
                            memberAccessCallExpr->startPosition(),
                            memberAccessCallExpr->endPosition(),
                            callOperatorDecl
                        );
                    processCallOperatorReferenceExpr(callOperatorReference);
                    auto newExpr = new MemberFunctionCallExpr(
                            callOperatorReference,
                            memberAccessCallExpr->objectRef,
                            functionCallExpr->arguments,
                            functionCallExpr->startPosition(),
                            functionCallExpr->endPosition()
                        );

                    // We handle argument casting and conversion no matter what. The below function will handle
                    // converting from lvalue to rvalue, casting, and other rules for us.
                    handleArgumentCasting(callOperatorDecl->parameters(), functionCallExpr->arguments);

                    // We steal the object reference
                    memberAccessCallExpr->objectRef = nullptr;
                    // And we steal the arguments
                    functionCallExpr->arguments.clear();
                    // Delete the old function call and replace it with the new one
                    delete functionCallExpr;
                    functionCallExpr = newExpr;
                    functionCallExpr->valueType = callOperatorDecl->returnType->deepCopy();
                    functionCallExpr->valueType->setIsLValue(true);
                    break;
                }
                case Decl::Kind::TemplateFunctionInst:
                case Decl::Kind::Function: {
                    auto functionDecl = llvm::dyn_cast<FunctionDecl>(foundDecl);

                    auto functionReference = new FunctionReferenceExpr(
                            memberAccessCallExpr->startPosition(),
                            memberAccessCallExpr->endPosition(),
                            functionDecl
                        );
                    processFunctionReferenceExpr(functionReference);
                    auto newExpr = new MemberFunctionCallExpr(
                            functionReference,
                            memberAccessCallExpr->objectRef,
                            functionCallExpr->arguments,
                            functionCallExpr->startPosition(),
                            functionCallExpr->endPosition()
                        );

                    // We handle argument casting and conversion no matter what. The below function will handle
                    // converting from lvalue to rvalue, casting, and other rules for us.
                    handleArgumentCasting(functionDecl->parameters(), functionCallExpr->arguments);

                    // We steal the object reference
                    memberAccessCallExpr->objectRef = nullptr;
                    // And we steal the arguments
                    functionCallExpr->arguments.clear();
                    // Delete the old function call and replace it with the new one
                    delete functionCallExpr;
                    functionCallExpr = newExpr;
                    functionCallExpr->valueType = functionDecl->returnType->deepCopy();
                    functionCallExpr->valueType->setIsLValue(true);
                    break;
                }
                case Decl::Kind::Property: {
                    auto propertyDecl = llvm::dyn_cast<PropertyDecl>(foundDecl);

                    // TODO: How should we do this? Like so?
                    //        1. Create a `CallOperatorReferenceExpr` for holding the `CallOperatorDecl`
                    //        2. Create a `MemberFunctionCallExpr` that will take `memberAccessCallExpr->objectRef`
                    //           as the `self`
                    //        3. For `objectRef` since it is a `property` we will need to create a `PropertyRefExpr`
                    //        4. Create a `get` call to the `PropertyRefExpr`
                    //        5. Delete the old `functionCallExpr` and replace it with the member version?
                    // TODO: It isn't always a `call` operator, it could also be a `func` pointer
                    printError("using `call` operator on a `prop` `get` value not yet supported!",
                               functionCallExpr->startPosition(), functionCallExpr->endPosition());
                    break;
                }
                case Decl::Kind::Variable: {
                    auto variableDecl = llvm::dyn_cast<VariableDecl>(foundDecl);

                    // TODO: Same as `prop`
                    printError("using `call` operator on a `var` not yet supported!",
                               functionCallExpr->startPosition(), functionCallExpr->endPosition());
                    break;
                }
                default:
                    printError("unsupported function call `" + functionCallExpr->toString() + "`!",
                               functionCallExpr->startPosition(), functionCallExpr->endPosition());
                    break;
            }
        }
    } else {
        // TODO: Make sure the value type is a function pointer or a type containing `call`
        processExpr(functionCallExpr->functionReference);
    }
}

gulc::Decl* gulc::CodeProcessor::findMatchingFunctorDecl(std::vector<Decl*>& searchDecls,
                                                         gulc::IdentifierExpr* identifierExpr,
                                                         std::vector<LabeledArgumentExpr*> const& arguments,
                                                         bool findStatic, bool* outIsAmbiguous) {
    std::vector<MatchingDecl> foundMatches;

    for (Decl* checkDecl : searchDecls) {
        // Skip if identifiers don't match.
        if (checkDecl->identifier().name() != identifierExpr->identifier().name()) continue;

        // NOTE: Even if `findStatic` is true we have to check `StructDecl`, `EnumDecl`, `TraitDecl`, etc.
        //       Anything that can be used as a type with a constructor MUST be checked even when `findStatic` is true.
        if (llvm::isa<EnumDecl>(checkDecl)) {
            if (identifierExpr->hasTemplateArguments()) continue;

            // TODO: Compare
        } else if (llvm::isa<StructDecl>(checkDecl)) {
            if (identifierExpr->hasTemplateArguments()) continue;

            auto structDecl = llvm::dyn_cast<StructDecl>(checkDecl);

            fillListOfMatchingConstructors(structDecl, arguments, foundMatches);
        } else if (llvm::isa<TemplateStructDecl>(checkDecl)) {
            // TODO: If the template has all default values we don't need to check if there are template parameters
            if (!identifierExpr->hasTemplateArguments()) continue;

            auto templateStructDecl = llvm::dyn_cast<TemplateStructDecl>(checkDecl);

            SignatureComparer::ArgMatchResult templateArgMatchResult =
                    SignatureComparer::compareTemplateArgumentsToParameters(templateStructDecl->templateParameters(),
                                                                            identifierExpr->templateArguments());

            if (templateArgMatchResult != SignatureComparer::ArgMatchResult::Fail) {
                // TODO: How should we handle `Castable` on the template arguments??
                // TODO: Should we attempt to delay template instantiation? I think it's fine since we've found a match...
                DeclInstantiator declInstantiator(_target, _filePaths);
                TemplateStructInstDecl* checkTemplateStructInst =
                        declInstantiator.instantiateTemplateStruct(_currentFile, templateStructDecl,
                                                                   identifierExpr->templateArguments(),
                                                                   identifierExpr->toString(),
                                                                   identifierExpr->startPosition(),
                                                                   identifierExpr->endPosition());

                // TODO: We need to account for if `checkTemplateStructInst` has already been code processed and make
                //       sure it gets processed (since its parent template might have already been processed before it)

                fillListOfMatchingConstructors(checkTemplateStructInst, arguments, foundMatches);
            }
        } else if (llvm::isa<TraitDecl>(checkDecl)) {
            if (identifierExpr->hasTemplateArguments()) continue;

            // TODO: Identifiers match and template parameters match. Traits only have a couple built in constructors
            //       so validate the parameters.
            //       I think the below will be the only supported constructors:
            //           `init<T>(_ obj: &T) where T : Self`
            //           `init copy(_ other: &Self)`
            //           `init move(_ other: &Self)`
        } else if (llvm::isa<TemplateTraitDecl>(checkDecl)) {
            // TODO: If the template has all default values we don't need to check if there are template parameters
            if (!identifierExpr->hasTemplateArguments()) continue;

            // TODO: Identifiers match. Compare the template parameters and create an instantiation.
        } else if (checkDecl->isStatic() == findStatic) {
            if (identifierExpr->hasTemplateArguments()) {
                if (llvm::isa<TemplateFunctionDecl>(checkDecl)) {
                    auto checkTemplateFunction = llvm::dyn_cast<TemplateFunctionDecl>(checkDecl);

                    SignatureComparer::ArgMatchResult templateArgMatchResult =
                            SignatureComparer::compareTemplateArgumentsToParameters(
                                    checkTemplateFunction->templateParameters(),
                                    identifierExpr->templateArguments()
                                );

                    if (templateArgMatchResult != SignatureComparer::ArgMatchResult::Fail) {
                        // NOTE: With template functions we can safely delay template instantiation until after we've
                        //       verified a match (since our `SignatureComparer` is made to account for template
                        //       arguments)
                        SignatureComparer::ArgMatchResult argMatchResult =
                                SignatureComparer::compareArgumentsToParameters(
                                        checkTemplateFunction->parameters(),
                                        arguments,
                                        checkTemplateFunction->templateParameters(),
                                        identifierExpr->templateArguments()
                                    );

                        if (argMatchResult != SignatureComparer::ArgMatchResult::Fail) {
                            DeclInstantiator declInstantiator(_target, _filePaths);
                            TemplateFunctionInstDecl* templateFunctionInst =
                                    declInstantiator.instantiateTemplateFunction(_currentFile, checkTemplateFunction,
                                                                                 identifierExpr->templateArguments(),
                                                                                 identifierExpr->toString(),
                                                                                 identifierExpr->startPosition(),
                                                                                 identifierExpr->endPosition());

                            // TODO: We need to account for if `checkTemplateStructInst` has already been code processed and make
                            //       sure it gets processed (since its parent template might have already been processed before it)

                            // TODO: How should we handle `Castable` on the template arguments??
                            MatchingDecl::Kind matchKind = argMatchResult == SignatureComparer::ArgMatchResult::Match
                                    ? MatchingDecl::Kind::Match
                                    : MatchingDecl::Kind::Castable;

                            foundMatches.emplace_back(MatchingDecl(matchKind, templateFunctionInst));
                        }
                    }
                }
            } else {
                if (llvm::isa<OperatorDecl>(checkDecl) || llvm::isa<TypeSuffixDecl>(checkDecl)) {
                    // NOTE: `Operator` and `TypeSuffix` are both considered `Function`s so we nee to manually skip them
                    //       It should be impossible to get here due to the identifier match but I'm paranoid.
                    continue;
                } else if (llvm::isa<FunctionDecl>(checkDecl)) {
                    // NOTE: `variable.call(...)` is legal and is the same as `variable(...)`
                    auto checkFunction = llvm::dyn_cast<FunctionDecl>(checkDecl);

                    SignatureComparer::ArgMatchResult argMatchResult =
                            SignatureComparer::compareArgumentsToParameters(checkFunction->parameters(), arguments);

                    if (argMatchResult != SignatureComparer::ArgMatchResult::Fail) {
                        MatchingDecl::Kind matchKind =
                                argMatchResult == SignatureComparer::ArgMatchResult::Match
                                ? MatchingDecl::Kind::Match
                                : MatchingDecl::Kind::Castable;

                        foundMatches.emplace_back(MatchingDecl(matchKind, checkDecl));
                    }

                    continue;
                } else if (llvm::isa<PropertyDecl>(checkDecl)) {
                    auto checkProperty = llvm::dyn_cast<PropertyDecl>(checkDecl);

                    Decl* outFoundDecl = nullptr;

                    SignatureComparer::ArgMatchResult argMatchResult =
                            FunctorUtil::checkValidFunctorCall(checkProperty->type, arguments, &outFoundDecl);

                    if (argMatchResult != SignatureComparer::ArgMatchResult::Fail) {
                        MatchingDecl::Kind matchKind =
                                argMatchResult == SignatureComparer::ArgMatchResult::Match
                                ? MatchingDecl::Kind::Match
                                : MatchingDecl::Kind::Castable;

                        foundMatches.emplace_back(MatchingDecl(matchKind, checkDecl));
                    }

                    continue;
                } else if (llvm::isa<VariableDecl>(checkDecl)) {
                    auto checkVariable = llvm::dyn_cast<VariableDecl>(checkDecl);

                    Decl* outFoundDecl = nullptr;

                    SignatureComparer::ArgMatchResult argMatchResult =
                            FunctorUtil::checkValidFunctorCall(checkVariable->type, arguments, &outFoundDecl);

                    if (argMatchResult != SignatureComparer::ArgMatchResult::Fail) {
                        MatchingDecl::Kind matchKind =
                                argMatchResult == SignatureComparer::ArgMatchResult::Match
                                ? MatchingDecl::Kind::Match
                                : MatchingDecl::Kind::Castable;

                        foundMatches.emplace_back(MatchingDecl(matchKind, checkDecl));
                    }

                    continue;
                }
            }
        }
    }

    Decl* foundDecl = nullptr;

    if (foundMatches.size() > 1) {
        for (MatchingDecl& checkMatch : foundMatches) {
            if (checkMatch.kind == MatchingDecl::Kind::Match) {
                if (foundDecl == nullptr) {
                    foundDecl = checkMatch.matchingDecl;
                } else {
                    *outIsAmbiguous = true;
                    return nullptr;
                }
            }
        }
    } else if (!foundMatches.empty()) {
        foundDecl = foundMatches[0].matchingDecl;
    }

    return foundDecl;
}

void gulc::CodeProcessor::fillListOfMatchingConstructors(gulc::StructDecl* structDecl,
                                                         std::vector<LabeledArgumentExpr*> const& arguments,
                                                         std::vector<MatchingDecl>& matchingDecls) {
    // TODO: Handle type inference on template constructors
    for (ConstructorDecl* constructor : structDecl->constructors()) {
        SignatureComparer::ArgMatchResult argMatchResult =
                SignatureComparer::compareArgumentsToParameters(constructor->parameters(), arguments);

        if (argMatchResult != SignatureComparer::ArgMatchResult::Fail) {
            MatchingDecl::Kind matchKind =
                    argMatchResult == SignatureComparer::ArgMatchResult::Match
                    ? MatchingDecl::Kind::Match
                    : MatchingDecl::Kind::Castable;

            matchingDecls.emplace_back(MatchingDecl(matchKind, constructor));
        }
    }
}

bool gulc::CodeProcessor::fillListOfMatchingCallOperators(gulc::Type* fromType,
                                                          const std::vector<LabeledArgumentExpr*>& arguments,
                                                          std::vector<MatchingDecl>& matchingDecls) {
    Type* processType = fromType;

    if (llvm::isa<DependentType>(processType)) {
        processType = llvm::dyn_cast<DependentType>(processType)->dependent;
    }

    std::vector<Decl*>* checkDecls = nullptr;
    std::vector<TemplateParameterDecl*>* templateParameters = nullptr;
    std::vector<Expr*>* templateArguments = nullptr;

    if (llvm::isa<StructType>(processType)) {
        auto structType = llvm::dyn_cast<StructType>(processType);

        checkDecls = &structType->decl()->allMembers;
    } else if (llvm::isa<TemplateStructType>(processType)) {
        auto templateStructType = llvm::dyn_cast<TemplateStructType>(processType);

        checkDecls = &templateStructType->decl()->allMembers;
        templateParameters = &templateStructType->decl()->templateParameters();
        templateArguments = &templateStructType->templateArguments();
    } else if (llvm::isa<TraitType>(processType)) {
        auto traitType = llvm::dyn_cast<TraitType>(processType);

        checkDecls = &traitType->decl()->allMembers;
    } else if (llvm::isa<TemplateTraitType>(processType)) {
        auto templateTraitType = llvm::dyn_cast<TemplateTraitType>(processType);

        checkDecls = &templateTraitType->decl()->allMembers;
        templateParameters = &templateTraitType->decl()->templateParameters();
        templateArguments = &templateTraitType->templateArguments();
    }
    // TODO: We ALSO need to account for `TemplateTypenameRefType` (it will only have `inheritedMembers` and constraints)

    bool matchFound = false;

    // TODO: We will also have to handle checking the extensions for a type...
    if (checkDecls != nullptr) {
        for (Decl* checkDecl : *checkDecls) {
            if (llvm::isa<CallOperatorDecl>(checkDecl)) {
                auto checkCallOperator = llvm::dyn_cast<CallOperatorDecl>(checkDecl);

                SignatureComparer::ArgMatchResult result = SignatureComparer::ArgMatchResult::Fail;

                if (templateParameters == nullptr) {
                    result = SignatureComparer::compareArgumentsToParameters(checkCallOperator->parameters(),
                                                                             arguments);
                } else {
                    result = SignatureComparer::compareArgumentsToParameters(checkCallOperator->parameters(),
                                                                             arguments, *templateParameters,
                                                                             *templateArguments);
                }

                if (result != SignatureComparer::ArgMatchResult::Fail) {
                    matchFound = true;

                    matchingDecls.emplace_back(MatchingDecl(result == SignatureComparer::ArgMatchResult::Match ?
                                                            MatchingDecl::Kind::Match : MatchingDecl::Kind::Castable,
                                                            checkCallOperator));
                }
            }
        }
    }

    return matchFound;
}

bool gulc::CodeProcessor::fillListOfMatchingFunctors(gulc::Decl* fromContainer, IdentifierExpr* identifierExpr,
                                                     const std::vector<LabeledArgumentExpr*>& arguments,
                                                     std::vector<MatchingDecl> &matchingDecls) {
    bool matchFound = false;

    if (llvm::isa<EnumDecl>(fromContainer)) {
        // TODO: Support `EnumDecl`
        printError("[INTERNAL] searching `EnumDecl` as a container is not yet supported!",
                   fromContainer->startPosition(), fromContainer->endPosition());
    } else if (llvm::isa<ExtensionDecl>(fromContainer)) {
        // TODO: Support `ExtensionDecl`? I think so? For when we're processing a function within an extension?
        printError("[INTERNAL] searching `ExtensionDecl` as a container is not yet supported!",
                   fromContainer->startPosition(), fromContainer->endPosition());
    } else if (llvm::isa<NamespaceDecl>(fromContainer)) {
        auto namespaceContainer = llvm::dyn_cast<NamespaceDecl>(fromContainer);

        // We have an out variable we don't care about here.
        bool ignored = true;

        for (Decl* checkDecl : namespaceContainer->nestedDecls()) {
            if (llvm::isa<StructDecl>(checkDecl) || llvm::isa<TemplateStructDecl>(checkDecl)) {
                if (addToListIfMatchingStructInit(checkDecl, identifierExpr, arguments, matchingDecls)) {
                    matchFound = true;
                }
            } else if (llvm::isa<FunctionDecl>(checkDecl) ||
                    llvm::isa<TemplateFunctionDecl>(checkDecl)) {
                if (addToListIfMatchingFunction(checkDecl, identifierExpr,
                                                arguments, matchingDecls, ignored)) {
                    matchFound = true;
                }
            }
        }
    } else if (llvm::isa<StructDecl>(fromContainer)) {
        auto structContainer = llvm::dyn_cast<StructDecl>(fromContainer);

        bool foundExact = false;

        for (Decl* checkDecl : structContainer->allMembers) {
            if (llvm::isa<FunctionDecl>(checkDecl) ||
                    llvm::isa<TemplateFunctionDecl>(checkDecl)) {
                if (addToListIfMatchingFunction(checkDecl, identifierExpr,
                                                arguments, matchingDecls, foundExact)) {
                    matchFound = true;
                }
            }
        }
    } else if (llvm::isa<TemplateStructDecl>(fromContainer)) {
        // TODO: Support `TemplateStructDecl`
        // TODO: I don't think this function should ever be called within templates. We should have custom
        //       instantiations that handle validation for templates.
        printError("[INTERNAL] searching `TemplateStructDecl` as a container is not yet supported!",
                   fromContainer->startPosition(), fromContainer->endPosition());
    } else if (llvm::isa<TemplateTraitDecl>(fromContainer)) {
        // TODO: Support `TemplateTraitDecl`
        // TODO: I don't think this function should ever be called within templates. We should have custom
        //       instantiations that handle validation for templates.
        printError("[INTERNAL] searching `TemplateTraitDecl` as a container is not yet supported!",
                   fromContainer->startPosition(), fromContainer->endPosition());
    } else if (llvm::isa<TraitDecl>(fromContainer)) {
        auto traitContainer = llvm::dyn_cast<TraitDecl>(fromContainer);

        bool foundExact = false;

        for (Decl* checkDecl : traitContainer->allMembers) {
            if (llvm::isa<FunctionDecl>(checkDecl) ||
                llvm::isa<TemplateFunctionDecl>(checkDecl)) {
                if (addToListIfMatchingFunction(checkDecl, identifierExpr,
                                                arguments, matchingDecls, foundExact)) {
                    matchFound = true;
                }
            }
        }
    } else {
        printError("[INTERNAL] unsupported container Decl found in `CodeProcessor::fillListOfMatchingFunctors`!",
                   fromContainer->startPosition(), fromContainer->endPosition());
    }

    return matchFound;
}

bool gulc::CodeProcessor::fillListOfMatchingFunctors(gulc::ASTFile* file, gulc::IdentifierExpr* identifierExpr,
                                                     std::vector<LabeledArgumentExpr*> const& arguments,
                                                     std::vector<MatchingDecl>& matchingDecls) {
    bool matchFound = false;
    bool ignoreFoundExact = false;

    for (Decl* checkDecl : file->declarations) {
        if (llvm::isa<StructDecl>(checkDecl) || llvm::isa<TemplateStructDecl>(checkDecl)) {
            if (addToListIfMatchingStructInit(checkDecl, identifierExpr, arguments, matchingDecls)) {
                matchFound = true;
            }
        } else if (llvm::isa<FunctionDecl>(checkDecl) ||
                llvm::isa<TemplateFunctionDecl>(checkDecl)) {
            if (addToListIfMatchingFunction(checkDecl, identifierExpr,
                                            arguments, matchingDecls, ignoreFoundExact)) {
                matchFound = true;
            }
        }
    }

    return matchFound;
}

bool gulc::CodeProcessor::addToListIfMatchingFunction(Decl* checkFunction, IdentifierExpr* identifierExpr,
                                                      std::vector<LabeledArgumentExpr*> const& arguments,
                                                      std::vector<MatchingDecl>& matchingDecls, bool& isExact) {
    if (llvm::isa<TemplateFunctionDecl>(checkFunction)) {
        // TODO: We need to allow template type inference.
        //       `func example<T>(param: T)`
        //       `example(12)` == `example<i32>(12)`
        auto templateFunctionDecl = llvm::dyn_cast<TemplateFunctionDecl>(checkFunction);

        // Skip if the names don't match
        if (templateFunctionDecl->identifier().name() != identifierExpr->identifier().name()) {
            return false;
        }

        // TODO: If the template parameters are empty we need to check if the types of the expressions can fit
        //       For missing template parameters we need to make sure `foundExact` is false
        SignatureComparer::ArgMatchResult templateParametersMatchResult =
                SignatureComparer::compareTemplateArgumentsToParameters(
                        templateFunctionDecl->templateParameters(), identifierExpr->templateArguments());

        // Skip if the template parameters don't match the template parameters
        if (templateParametersMatchResult == SignatureComparer::ArgMatchResult::Fail) {
            return false;
        }

        SignatureComparer::ArgMatchResult parameterMatchResult =
                SignatureComparer::compareArgumentsToParameters(templateFunctionDecl->parameters(), arguments,
                        templateFunctionDecl->templateParameters(), identifierExpr->templateArguments());

        // Skip if the normal parameters don't match the parameters
        if (parameterMatchResult == SignatureComparer::ArgMatchResult::Fail) {
            return false;
        }

        // TODO: If we reach this point then everything fits. If the template parameters are empty we'll need to
        //       account for that and make it known. The below is NOT ambiguous:
        //       `func example(_: i32)`
        //       `func example<T>(_: T)`
        //       `example(12)`
        //       The `example(_: i32)` will be called as it is a specialized function. And you can call
        //       `example<T>` by doing `example<i32>(12)` instead.
        if (templateParametersMatchResult == SignatureComparer::ArgMatchResult::Match &&
            parameterMatchResult == SignatureComparer::ArgMatchResult::Match) {
            // Requiring a cast means the match isn't exact...
            isExact = true;

            matchingDecls.emplace_back(MatchingDecl(MatchingDecl::Kind::Match, checkFunction));
            return true;
        } else {
            matchingDecls.emplace_back(MatchingDecl(MatchingDecl::Kind::Castable, checkFunction));
            return true;
        }
    } else if (llvm::isa<FunctionDecl>(checkFunction)) {
        // If template parameters are provided then it cannot be a normal function
        if (identifierExpr->hasTemplateArguments()) return false;

        auto functionDecl = llvm::dyn_cast<FunctionDecl>(checkFunction);

        // Skip when names aren't the same
        if (checkFunction->identifier().name() != identifierExpr->identifier().name()) {
            return false;
        }

        SignatureComparer::ArgMatchResult parameterMatchResult =
                SignatureComparer::compareArgumentsToParameters(functionDecl->parameters(), arguments);

        // Skip if the normal parameters don't match the parameters
        if (parameterMatchResult == SignatureComparer::ArgMatchResult::Fail) {
            return false;
        }

        if (parameterMatchResult == SignatureComparer::ArgMatchResult::Match) {
            // Requiring a cast means the match isn't exact...
            isExact = true;

            matchingDecls.emplace_back(MatchingDecl(MatchingDecl::Kind::Match, checkFunction));
            return true;
        } else {
            matchingDecls.emplace_back(MatchingDecl(MatchingDecl::Kind::Castable, checkFunction));
            return true;
        }
    } else {
        // TODO: I don't think we should error here... I think just returning false is okay, right?
//        printError("unknown declaration found in `CodeProcessor::addToListIfMatchingFunction`!",
//                   identifierExpr->startPosition(), identifierExpr->endPosition());
    }

    return false;
}

bool gulc::CodeProcessor::addToListIfMatchingStructInit(gulc::Decl* structDecl, gulc::IdentifierExpr* identifierExpr,
                                                        std::vector<LabeledArgumentExpr*> const& arguments,
                                                        std::vector<MatchingDecl>& matchingDecls,
                                                        bool ignoreTemplateArguments) {
    if (llvm::isa<TemplateStructDecl>(structDecl)) {
        // Names don't match, can't be the right struct obviously.
        if (identifierExpr->identifier().name() != structDecl->identifier().name()) return false;

        if (!identifierExpr->hasTemplateArguments()) {
            // TODO: Support type inference
            return false;
        }

        auto checkTemplateStruct = llvm::dyn_cast<TemplateStructDecl>(structDecl);

        // Validate the template parameters match the template parameters
        SignatureComparer::ArgMatchResult templateArgMatchResult =
                SignatureComparer::compareTemplateArgumentsToParameters(checkTemplateStruct->templateParameters(),
                                                                        identifierExpr->templateArguments());

        if (templateArgMatchResult == SignatureComparer::ArgMatchResult::Fail) {
            return false;
        }

        // At this point the template is a match, we retrieve or instantiate a template struct inst to check the
        // constructor for.
        // TODO: Should we attempt to delay template instantiation? I think it's fine since we've found a match...
        DeclInstantiator declInstantiator(_target, _filePaths);
        TemplateStructInstDecl* checkTemplateStructInst =
                declInstantiator.instantiateTemplateStruct(_currentFile, checkTemplateStruct,
                                                           identifierExpr->templateArguments(),
                                                           identifierExpr->toString(),
                                                           identifierExpr->startPosition(),
                                                           identifierExpr->endPosition());

        // TODO: We need to account for if `checkTemplateStructInst` has already been code processed and make sure
        //       it gets processed (since its parent template might have already been processed before it)

        // Now that we have the template struct instantiation we can call `addToListIfMatchingStructInit` with the inst
        // and let the below `StructDecl` handler to check the constructors...
        // TODO: what should we do if `templateArgMatchResult` is `Castable`?
        return addToListIfMatchingStructInit(checkTemplateStructInst, identifierExpr, arguments, matchingDecls,
                                             true);
    } else if (llvm::isa<StructDecl>(structDecl)) {
        // You can't call a non-template struct with template parameters.
        // NOTE: This check is skippable through `ignoreTemplateArguments`, which is needed for `TemplateStructInstDecl`
        if (!ignoreTemplateArguments && identifierExpr->hasTemplateArguments()) return false;

        auto checkStruct = llvm::dyn_cast<StructDecl>(structDecl);

        // Names don't match, can't be the right struct obviously.
        if (identifierExpr->identifier().name() != structDecl->identifier().name()) return false;

        bool foundMatch = false;

        // TODO: We need to support extensions. We'll have to do a lookup to grab all extensions and check them for
        //       matching constructors first.
        for (ConstructorDecl* checkConstructor : checkStruct->constructors()) {
            // TODO: If/when we support template `init` declarations we will have to handle type inference here.
            SignatureComparer::ArgMatchResult parameterMatchResult =
                    SignatureComparer::compareArgumentsToParameters(checkConstructor->parameters(), arguments);

            // Skip if the normal parameters don't match the parameters
            if (parameterMatchResult == SignatureComparer::ArgMatchResult::Fail) {
                continue;
            }

            if (parameterMatchResult == SignatureComparer::ArgMatchResult::Match) {
                matchingDecls.emplace_back(MatchingDecl(MatchingDecl::Kind::Match, checkConstructor));
                foundMatch = true;
            } else {
                matchingDecls.emplace_back(MatchingDecl(MatchingDecl::Kind::Castable, checkConstructor));
                foundMatch = true;
            }
        }

        return foundMatch;
    }

    return false;
}

gulc::Decl* gulc::CodeProcessor::validateAndReturnMatchingFunction(FunctionCallExpr* functionCallExpr,
                                                                   std::vector<MatchingDecl>& matchingDecls) {
    if (matchingDecls.size() == 1) {
        // If there is only one match we don't have anything else to do, this is the function that was
        // intended.
        // TODO: Cast anything that needs to be casted (if it isn't exact)
        return matchingDecls[0].matchingDecl;
    } else {
        // If there are more than one match we need to check there is only a single exact match.
        // If there aren't any exact matches or there are more than one we need to error saying the call
        // is ambiguous
        bool foundExact = false;
        Decl* foundDecl = nullptr;

        for (MatchingDecl& checkMatch : matchingDecls) {
            if (checkMatch.kind == MatchingDecl::Kind::Match) {
                if (foundExact) {
                    // If there already is a match we need to error saying it is ambiguous...
                    // To do that we just set the found declaration to null and break the loop to trigger
                    // the error below.
                    foundExact = false;
                    foundDecl = nullptr;
                    break;
                } else {
                    foundExact = true;
                    foundDecl = checkMatch.matchingDecl;
                }
            }
        }

        if (foundDecl == nullptr) {
            printError("ambiguous call for `" + functionCallExpr->toString() + "`!",
                       functionCallExpr->startPosition(), functionCallExpr->endPosition());

            return nullptr;
        } else {
            return foundDecl;
        }
    }
}

gulc::Expr* gulc::CodeProcessor::createStaticFunctionReference(Expr* forExpr, gulc::Decl* function) const {
    Expr* newFunctionReference = nullptr;

    if (llvm::isa<FunctionDecl>(function)) {
        auto foundFunction = llvm::dyn_cast<FunctionDecl>(function);

        if (foundFunction->isAnyVirtual()) {
            // NOTE: This should NEVER happen. This means our earlier validation pass missed something.
            printError("[INTERNAL] virtual function found outside struct in `CodeProcessor::processFunctionCallExpr!",
                       foundFunction->startPosition(), foundFunction->endPosition());
        }

        newFunctionReference = new FunctionReferenceExpr(forExpr->startPosition(),
                                                         forExpr->endPosition(),
                                                         foundFunction);
    } else if (llvm::isa<TemplateFunctionDecl>(function)) {
        // TODO: We need to create an instantiation of the template which will require us to go to
        //       an earlier compiler pass to properly handle it...
        printError("template functions not yet supported!",
                   forExpr->startPosition(), forExpr->endPosition());
    } else {
        printError("[INTERNAL] unknown decl type found within `CodeProcessor::processFunctionCallExpr`!",
                   forExpr->startPosition(), forExpr->endPosition());
    }

    return newFunctionReference;
}

void gulc::CodeProcessor::processFunctionReferenceExpr(gulc::FunctionReferenceExpr* functionReferenceExpr) {
    functionReferenceExpr->valueType = TypeHelper::getFunctionPointerTypeFromDecl(functionReferenceExpr->functionDecl());
}

void gulc::CodeProcessor::processHasExpr(gulc::HasExpr* hasExpr) {
    // TODO: We need to fix this. I want to support `T has func test(_: i32)`
}

void gulc::CodeProcessor::processIdentifierExpr(gulc::Expr*& expr) {
    // NOTE: The generic `processIdentifierExpr` should ONLY look for normal variables and types, it should NOT look
    //       for functions. `processFunctionCallExpr` and `processSubscriptCallExpr` should handle `IdentifierExpr` as
    //       special cases in their own functions. They should ALSO handle `MemberAccessCallExpr` as a special case.
    //       Once `CodeProcessor` is done `IdentifierExpr` should NEVER appear in the AST again (except in
    //       uninstantiated templates)
    auto identifierExpr = llvm::dyn_cast<IdentifierExpr>(expr);

    // Local variables, parameters, and template parameters cannot have template parameters
    if (!identifierExpr->hasTemplateArguments()) {
        // First we check if it is a built in type
        if (BuiltInType::isBuiltInType(identifierExpr->identifier().name())) {
            auto builtInType = BuiltInType::get(Type::Qualifier::Unassigned,
                    identifierExpr->identifier().name(), expr->startPosition(), expr->endPosition());
            auto newExpr = new TypeExpr(builtInType);
            // Delete the old identifier
            delete expr;
            // Set it to the local variable reference and exit our function.
            expr = newExpr;
            return;
        }

        // Check for `self` and `Self` (for an easier `typeof(self)`, taken from Swift)
        // TODO: Check for `base`
        if (identifierExpr->identifier().name() == "self") {
            auto newExpr = getCurrentSelfRef(expr->startPosition(), expr->endPosition());
            // Delete the old identifier
            delete expr;
            // Set it to the local variable reference and exit our function.
            expr = newExpr;
            return;
        } else if (identifierExpr->identifier().name() == "Self") {
            if (_currentContainer != nullptr) {
                switch (_currentContainer->getDeclKind()) {
                    case Decl::Kind::Enum: {
                        auto enumDecl = llvm::dyn_cast<EnumDecl>(_currentContainer);
                        auto enumType = new EnumType(Type::Qualifier::Unassigned, enumDecl,
                                                     expr->startPosition(), expr->endPosition());
                        auto newExpr = new TypeExpr(enumType);
                        // Delete the old identifier
                        delete expr;
                        // Set it to the local variable reference and exit our function.
                        expr = newExpr;
                        return;
                    }
                    case Decl::Kind::Struct: {
                        auto structDecl = llvm::dyn_cast<StructDecl>(_currentContainer);
                        auto structType = new StructType(Type::Qualifier::Unassigned, structDecl,
                                                         expr->startPosition(), expr->endPosition());
                        auto newExpr = new TypeExpr(structType);
                        // Delete the old identifier
                        delete expr;
                        // Set it to the local variable reference and exit our function.
                        expr = newExpr;
                        return;
                    }
                    case Decl::Kind::Trait: {
                        auto traitDecl = llvm::dyn_cast<TraitDecl>(_currentContainer);
                        auto traitType = new TraitType(Type::Qualifier::Unassigned, traitDecl,
                                                       expr->startPosition(), expr->endPosition());
                        auto newExpr = new TypeExpr(traitType);
                        // Delete the old identifier
                        delete expr;
                        // Set it to the local variable reference and exit our function.
                        expr = newExpr;
                        return;
                    }
                    default:
                        printError("self type `Self` cannot be used outside of `struct`, `class`, `union`, or `enum`!",
                                   expr->startPosition(), expr->endPosition());
                }
            }
        }

        // Check local variables
        for (VariableDeclExpr* localVariable : _localVariables) {
            if (localVariable->identifier().name() == identifierExpr->identifier().name()) {
                auto newExpr = new LocalVariableRefExpr(expr->startPosition(), expr->endPosition(),
                                                        localVariable->identifier().name());
                // NOTE: The local variable must have it's type assigned by this point. Either through inference or
                //       being explicitly set.
                // TODO: We need to handle the variables mutability here.
                newExpr->valueType = localVariable->type->deepCopy();
                newExpr->valueType->setIsLValue(true);
                // Delete the old identifier
                delete expr;
                // Set it to the local variable reference and exit our function.
                expr = newExpr;
                return;
            }
        }

        // Check parameters
        if (_currentParameters != nullptr) {
            for (std::size_t paramIndex = 0; paramIndex < _currentParameters->size(); ++paramIndex) {
                ParameterDecl* parameter = (*_currentParameters)[paramIndex];

                if (parameter->identifier().name() == identifierExpr->identifier().name()) {
                    auto newExpr = new ParameterRefExpr(expr->startPosition(), expr->endPosition(),
                                                        paramIndex, parameter->identifier().name());
                    // TODO: We need to handle the variables mutability here.
                    newExpr->valueType = parameter->type->deepCopy();
                    newExpr->valueType->setIsLValue(true);
                    // Delete the old identifier
                    delete expr;
                    // Set it to the local variable reference and exit our function.
                    expr = newExpr;
                    return;
                }
            }
        }

        // TODO: Then check template parameters (if there are any) (is this needed? shouldn't template parameters be removed here?)
    }

    Decl* foundDecl = nullptr;

    // Check our current container
    if (_currentContainer != nullptr) {
        findMatchingDeclInContainer(_currentContainer, identifierExpr, &foundDecl);
    }

    // Check our current file
    if (foundDecl == nullptr) {
        findMatchingDecl(_currentFile->declarations, identifierExpr, &foundDecl);
    }

    // Check the imports with an ambiguity check
    if (foundDecl == nullptr) {
        // NOTE: We continue searching even after `foundDecl` is set for the imports,
        //       if `foundDecl` isn't null on a match then there is an ambiguity and we need to error out saying such.
        for (ImportDecl* checkImport : _currentFile->imports) {
            Decl* tmpFoundDecl = nullptr;

            if (findMatchingDecl(checkImport->pointToNamespace->nestedDecls(), identifierExpr, &tmpFoundDecl)) {
                if (foundDecl != nullptr) {
                    // TODO: Use `foundDecl` and `tmpFoundDecl` to show the two ambiguous identifier paths
                    printError("identifier `" + identifierExpr->toString() + "` is ambiguous!",
                               identifierExpr->startPosition(), identifierExpr->endPosition());
                }

                foundDecl = tmpFoundDecl;
            }
        }
    }

    if (foundDecl == nullptr) {
        printError("identifier `" + identifierExpr->toString() + "` was not found!",
                   identifierExpr->startPosition(), identifierExpr->endPosition());
    }

    switch (foundDecl->getDeclKind()) {
        case Decl::Kind::CallOperator:
            // TODO: Is this possible? Maybe with `call` but no `()`?
            // TODO: I think this should be handled the same way as `FunctionDecl`, it is a member function that
            //       should just be a `FunctionRefExpr` with `valueType` being a `FunctionPointerType` that has an
            //       explicit `self` that must be filled
            printError("`call` operator reference not yet supported!",
                       expr->startPosition(), expr->endPosition());
            break;
        case Decl::Kind::EnumConst: {
            auto enumConst = llvm::dyn_cast<EnumConstDecl>(foundDecl);
            auto enumDecl = llvm::dyn_cast<EnumDecl>(enumConst->container);
            auto newExpr = new EnumConstRefExpr(expr->startPosition(), expr->endPosition(), enumConst);
            processEnumConstRefExpr(newExpr);
            // Delete the old identifier
            delete expr;
            // Set it to the local variable reference and exit our function.
            expr = newExpr;
            return;
        }
        case Decl::Kind::Enum: {
            auto enumDecl = llvm::dyn_cast<EnumDecl>(foundDecl);
            auto newExpr = new TypeExpr(new EnumType(Type::Qualifier::Unassigned, enumDecl,
                                                     expr->startPosition(), expr->endPosition()));
            // Delete the old identifier
            delete expr;
            // Set it to the local variable reference and exit our function.
            expr = newExpr;
            return;
        }
        case Decl::Kind::Function: {
            auto functionDecl = llvm::dyn_cast<FunctionDecl>(foundDecl);
            auto newExpr = new FunctionReferenceExpr(expr->startPosition(), expr->endPosition(), functionDecl);
            processFunctionReferenceExpr(newExpr);
            // Delete the old identifier
            delete expr;
            // Set it to the local variable reference and exit our function.
            expr = newExpr;
            return;
        }
        case Decl::Kind::Namespace: {
            // TODO: Create a `NamespaceRef`? Or should we require `MemberAccess...` to handle that?
            printError("namespace paths not yet supported!",
                       identifierExpr->startPosition(), identifierExpr->endPosition());
            break;
        }
        case Decl::Kind::Property: {
            // TODO: Create a `PropertyRef` that will need to be converted to `PropertyGet`/`PropertySet` depending on context
            printError("properties not yet supported!",
                       expr->startPosition(), expr->endPosition());
            break;
        }
        case Decl::Kind::Struct: {
            auto structDecl = llvm::dyn_cast<StructDecl>(foundDecl);
            auto structType = new StructType(Type::Qualifier::Unassigned, structDecl,
                                             expr->startPosition(), expr->endPosition());
            auto newExpr = new TypeExpr(structType);
            // Delete the old identifier
            delete expr;
            // Set it to the local variable reference and exit our function.
            expr = newExpr;
            return;
        }
        case Decl::Kind::Trait: {
            auto traitDecl = llvm::dyn_cast<TraitDecl>(foundDecl);
            auto traitType = new TraitType(Type::Qualifier::Unassigned, traitDecl,
                                           expr->startPosition(), expr->endPosition());
            auto newExpr = new TypeExpr(traitType);
            // Delete the old identifier
            delete expr;
            // Set it to the local variable reference and exit our function.
            expr = newExpr;
            return;
        }
        case Decl::Kind::TypeAlias: {
            // TODO: instantiate the alias
            printError("type aliases not yet supported!",
                       expr->startPosition(), expr->endPosition());
            break;
        }
        case Decl::Kind::Variable: {
            auto variableDecl = llvm::dyn_cast<VariableDecl>(foundDecl);

            // NOTE: Only `struct`, `class`, and `union` can contain variables as a `MemberVariableRef`...
            if (llvm::isa<StructDecl>(foundDecl->container)) {
                // If the variable is a member of a struct we have to create a reference to `self` for accessing it.
                auto selfRef = getCurrentSelfRef(expr->startPosition(), expr->endPosition());
                auto selfStructType = new StructType(
                        Type::Qualifier::Immut,
                        llvm::dyn_cast<StructDecl>(foundDecl->container),
                        {}, {}
                    );
                auto newExpr = new MemberVariableRefExpr(expr->startPosition(), expr->endPosition(), selfRef,
                                                         selfStructType, variableDecl);
                processMemberVariableRefExpr(newExpr);
                // Delete the old identifier
                delete expr;
                // Set it to the local variable reference and exit our function.
                expr = newExpr;
                return;
            } else {
                auto newExpr = new VariableRefExpr(expr->startPosition(), expr->endPosition(),
                                                   variableDecl);
                processVariableRefExpr(newExpr);
                // Delete the old identifier
                delete expr;
                // Set it to the local variable reference and exit our function.
                expr = newExpr;
                return;
            }
        }
        default:
            printError("unknown decl reference!",
                       expr->startPosition(), expr->endPosition());
    }
}

bool gulc::CodeProcessor::findMatchingDeclInContainer(gulc::Decl* container, gulc::IdentifierExpr* identifierExpr,
                                                      gulc::Decl** outFoundDecl) {
    // NOTE: This function returns the first matching `Decl`. If there are template parameters we will validate them
    //       before returning but if there aren't template parameters we will return the first match for the identifier
    //       By this point all containers should have their members validated for name reuse so all members within
    //       `container` will have unique names when not templates.

    if (llvm::isa<NamespaceDecl>(container)) {
        auto checkNamespace = llvm::dyn_cast<NamespaceDecl>(container);

        return findMatchingDecl(checkNamespace->nestedDecls(), identifierExpr, outFoundDecl);
    } else if (llvm::isa<StructDecl>(container)) {
        auto checkStruct = llvm::dyn_cast<StructDecl>(container);

        return findMatchingDecl(checkStruct->allMembers, identifierExpr, outFoundDecl);
    } else if (llvm::isa<TraitDecl>(container)) {
        auto checkTrait = llvm::dyn_cast<TraitDecl>(container);

        return findMatchingDecl(checkTrait->allMembers, identifierExpr, outFoundDecl);
    } else {
        printError("[INTERNAL] unsupported container found in `CodeProcessor::findMatchingDeclInContainer`!",
                   identifierExpr->startPosition(), identifierExpr->endPosition());
        return false;
    }
}

bool gulc::CodeProcessor::findMatchingDecl(std::vector<Decl*> const& searchDecls, gulc::IdentifierExpr* identifierExpr,
                                           gulc::Decl** outFoundDecl) {
    for (Decl* checkDecl : searchDecls) {
        if (identifierExpr->identifier().name() == checkDecl->identifier().name()) {
            if (identifierExpr->hasTemplateArguments()) {
                if (llvm::isa<TemplateStructDecl>(checkDecl)) {
                    auto checkTemplateStruct = llvm::dyn_cast<TemplateStructDecl>(checkDecl);

                    if (SignatureComparer::compareTemplateArgumentsToParameters(checkTemplateStruct->templateParameters(),
                            identifierExpr->templateArguments()) != SignatureComparer::ArgMatchResult::Fail) {
                        // NOTE: We don't instantiate here. That should be handled by our caller.
                        *outFoundDecl = checkDecl;
                        return true;
                    }
                } else if (llvm::isa<TemplateTraitDecl>(checkDecl)) {
                    auto checkTemplateTrait = llvm::dyn_cast<TemplateTraitDecl>(checkDecl);

                    if (SignatureComparer::compareTemplateArgumentsToParameters(checkTemplateTrait->templateParameters(),
                            identifierExpr->templateArguments()) != SignatureComparer::ArgMatchResult::Fail) {
                        // NOTE: We don't instantiate here. That should be handled by our caller.
                        *outFoundDecl = checkDecl;
                        return true;
                    }
                } else if (llvm::isa<TemplateFunctionDecl>(checkDecl)) {
                    auto checkTemplateFunction = llvm::dyn_cast<TemplateFunctionDecl>(checkDecl);

                    if (SignatureComparer::compareTemplateArgumentsToParameters(checkTemplateFunction->templateParameters(),
                            identifierExpr->templateArguments()) != SignatureComparer::ArgMatchResult::Fail) {
                        // NOTE: We don't instantiate here. That should be handled by our caller.
                        *outFoundDecl = checkDecl;
                        return true;
                    }
                }
            } else {
                *outFoundDecl = checkDecl;
                return true;
            }
        }
    }

    return false;
}

void gulc::CodeProcessor::processInfixOperatorExpr(gulc::InfixOperatorExpr* infixOperatorExpr) {
    // NOTE: We need to make sure we check extensions as well. We will also need to check for any ambiguities on
    //       extensions. Two namespaces could define the same extension operator with the same signature causing
    //       ambiguities

    processExpr(infixOperatorExpr->leftValue);
    processExpr(infixOperatorExpr->rightValue);

    // TODO: For overriding do the following:
    //        1. Check if the `leftValue` type has an operator matching the `rightValue` type (check extensions as well)
    //        2. If still no, then should we check for being able to implicitly cast to any other `leftValue`'s other operators?
    //        3. If there are no overloads we then use the built in operators if they exist
    auto checkType = infixOperatorExpr->leftValue->valueType;

    if (llvm::isa<ReferenceType>(checkType)) {
        checkType = llvm::dyn_cast<ReferenceType>(checkType)->nestedType;
    }

    // TODO: Once supported we should check extensions first. Extensions can work on both built in types and custom
    //       types so we can handle extensions without checking what the `checkType` actually is.
    // Only `enum`, `class`, `struct`, `union`, and `trait` can define operators within their declarations.
    {
        std::vector<MatchingDecl> matchingDecls;

        switch (checkType->getTypeKind()) {
            case Type::Kind::Enum: {
                auto enumType = llvm::dyn_cast<EnumType>(checkType);

                // TODO: Use `ownedmembers`
                printError("operators on `enum` not yet supported!",
                           infixOperatorExpr->startPosition(), infixOperatorExpr->endPosition());
//                fillListOfMatchingInfixOperators(enumType->decl())
                break;
            }
            case Type::Kind::Struct: {
                auto structType = llvm::dyn_cast<StructType>(checkType);

                fillListOfMatchingInfixOperators(structType->decl()->allMembers,
                                                 infixOperatorExpr->infixOperator(),
                                                 infixOperatorExpr->rightValue->valueType,
                                                 matchingDecls);
                break;
            }
            case Type::Kind::Trait: {
                auto traitType = llvm::dyn_cast<TraitType>(checkType);

                fillListOfMatchingInfixOperators(traitType->decl()->allMembers,
                                                 infixOperatorExpr->infixOperator(),
                                                 infixOperatorExpr->rightValue->valueType,
                                                 matchingDecls);
                break;
            }
            default:
                break;
        }

        if (!matchingDecls.empty()) {
            // TODO: Search the matching infix operators:
            //        * Make sure there aren't duplicate matches (causing ambiguity)
            //        * ???
        }
    }

    // TODO: For built in operators we will have to add a `dereference` instruction here if either types are references
    auto leftType = infixOperatorExpr->leftValue->valueType;
    auto rightType = infixOperatorExpr->rightValue->valueType;

    if (llvm::isa<ReferenceType>(leftType)) {
        leftType = llvm::dyn_cast<ReferenceType>(leftType)->nestedType;
    }

    if (llvm::isa<ReferenceType>(rightType)) {
        rightType = llvm::dyn_cast<ReferenceType>(rightType)->nestedType;
    }

    switch (infixOperatorExpr->infixOperator()) {
        case InfixOperators::LogicalAnd: // &&
        case InfixOperators::LogicalOr: // ||
            // Once we've reached this point we need to convert any lvalues to rvalues and dereference any implicit references
            infixOperatorExpr->leftValue = convertLValueToRValue(infixOperatorExpr->leftValue);
            infixOperatorExpr->rightValue = convertLValueToRValue(infixOperatorExpr->rightValue);
            // Dereference the references (if there are any references)
            infixOperatorExpr->leftValue = dereferenceReference(infixOperatorExpr->leftValue);
            infixOperatorExpr->rightValue = dereferenceReference(infixOperatorExpr->rightValue);

            // For logical and and logical or we need both sides to be `bool`
            if (!llvm::isa<BoolType>(leftType)) {
                printError("left operand for `&&` must be of type `bool`! (it is of type `" +
                           leftType->toString() +
                           "`!",
                           infixOperatorExpr->leftValue->startPosition(), infixOperatorExpr->leftValue->endPosition());
            }

            if (!llvm::isa<BoolType>(rightType)) {
                printError("right operand for `&&` must be of type `bool`! (it is of type `" +
                           rightType->toString() +
                           "`!",
                           infixOperatorExpr->rightValue->startPosition(), infixOperatorExpr->rightValue->endPosition());
            }

            infixOperatorExpr->valueType = new BoolType(Type::Qualifier::Immut, {}, {});
            infixOperatorExpr->valueType->setIsLValue(false);
            return;
        default:
            // We continue, looking for built in type instead.
            break;
    }

    if (!llvm::isa<BuiltInType>(leftType) || !llvm::isa<BuiltInType>(rightType)) {
        // If one of the sides isn't a built in type we error saying an operator doesn't exist for the specified values
        // At this point we've already checked for an overload and found none.
        printError("invalid operation, no valid operator exists for the specified values!",
                   infixOperatorExpr->startPosition(), infixOperatorExpr->endPosition());
    }

    // Now that we know both sides are built in types we need to validate they can be added together (through implicit
    // casting if needed)
    // NOTES: For non-overloaded/built in infix operator usage we do NOT allow conversions that could lose data
    //        implicitly
    //         * Implicit u64 -> i64 is illegal as it could overflow and lose accuracy
    //         * Implicit f64 -> f32 is illegal
    //         * Implicit f* -> i*/u* is illegal as it will lose the decimal values
    //         * Implicit i*/u* -> f* is illegal as it will lose precision in most situations with larger numbers.
    //         * Implicit i* -> u* is illegal as it loses negative accuracy
    //         * Implicit i8 -> i16 is legal as it loses nothing
    //         * Implicit u8 -> i16 is legal as it loses nothing
    auto leftBuiltIn = llvm::dyn_cast<BuiltInType>(leftType);
    auto rightBuiltIn = llvm::dyn_cast<BuiltInType>(rightType);

    // Either both sides need to be floating or both sides need to be non-floating. If they don't match there needs to
    // be an explicit cast.
    if (leftBuiltIn->isFloating() != rightBuiltIn->isFloating()) {
        printError("cannot implicitly convert between `" + leftBuiltIn->toString() + "` and `" + rightBuiltIn->toString() + "`!",
                   infixOperatorExpr->startPosition(), infixOperatorExpr->endPosition());
    }

    // The sides having differing signage needs an extra check to validate the unsigned value is smaller than the
    // signed value. (i.e. `u8 -> i16`) if the signed value is the same size or larger than the unsigned then there
    // needs to be an explicit cast.
    if (leftBuiltIn->isSigned() != rightBuiltIn->isSigned()) {
        if (leftBuiltIn->isSigned()) {
            if (leftBuiltIn->sizeInBytes() <= rightBuiltIn->sizeInBytes()) {
                printError("cannot implicitly convert from `" + leftBuiltIn->toString() + "` to `" + rightBuiltIn->toString() + "`!",
                           infixOperatorExpr->startPosition(), infixOperatorExpr->endPosition());
            }
        } else {
            if (rightBuiltIn->sizeInBytes() <= leftBuiltIn->sizeInBytes()) {
                printError("cannot implicitly convert from `" + rightBuiltIn->toString() + "` to `" + leftBuiltIn->toString() + "`!",
                           infixOperatorExpr->startPosition(), infixOperatorExpr->endPosition());
            }
        }
    }

    // NOTE: We are using `leftBuiltIn` and `rightBuiltIn` as the result of a built in operator does NOT return the
    //       reference. It returns an entirely new value with no reference.
    Type* resultType = leftBuiltIn;

    // At this point any needed implicit casts are valid. If the size doesn't match or the signage doesn't match we
    // need to cast to the bigger or the signed of the two types.
    // Because of the fact that casting from unsigned to signed requires the signed type to be bigger for an implicit
    // cast we can skip another check of sign not being the same and only check if one size is bigger and cast to the
    // bigger size if they aren't the same size.
    if (leftBuiltIn->sizeInBytes() > rightBuiltIn->sizeInBytes()) {
        // Cast `right` to the type of `left`
        // TODO: Check if there is an extension to implicitly cast `right` to `left` as that is overloadable
        // NOTE: We use `leftBuiltIn` and `rightBuiltIn` for the implicit cast since `left` and `right could be
        //       references.
        infixOperatorExpr->rightValue = new ImplicitCastExpr(infixOperatorExpr->rightValue,
                                                             leftBuiltIn->deepCopy());
    } else if (rightBuiltIn->sizeInBytes() > leftBuiltIn->sizeInBytes()) {
        // Cast `left` to the type of `right`
        // TODO: Check if there is an extension to implicitly cast `left` to `right` as that is overloadable
        infixOperatorExpr->leftValue = new ImplicitCastExpr(infixOperatorExpr->leftValue,
                                                            rightBuiltIn->deepCopy());

        resultType = rightBuiltIn;
    }

    // Once we've reached this point we need to convert any lvalues to rvalues and dereference any implicit references
    infixOperatorExpr->leftValue = convertLValueToRValue(infixOperatorExpr->leftValue);
    infixOperatorExpr->rightValue = convertLValueToRValue(infixOperatorExpr->rightValue);
    // Dereference the references (if there are any references)
    infixOperatorExpr->leftValue = dereferenceReference(infixOperatorExpr->leftValue);
    infixOperatorExpr->rightValue = dereferenceReference(infixOperatorExpr->rightValue);

    // We need to change `resultType` if the operator is a logical (i.e. `bool`) operator.
    switch (infixOperatorExpr->infixOperator()) {
        // NOTE: `LogicalAnd` and `LogicalOr` are handled before the built in check.
        case InfixOperators::EqualTo: // ==
        case InfixOperators::NotEqualTo: // !=
        case InfixOperators::GreaterThan: // >
        case InfixOperators::LessThan: // <
        case InfixOperators::GreaterThanEqualTo: // >=
        case InfixOperators::LessThanEqualTo: // <=
            infixOperatorExpr->valueType = new BoolType(Type::Qualifier::Immut, {}, {});
            break;
        default:
            infixOperatorExpr->valueType = resultType->deepCopy();
            break;
    }

    infixOperatorExpr->valueType->setIsLValue(false);

    // At this point is properly validated and any required casts are complete.
}

bool gulc::CodeProcessor::fillListOfMatchingInfixOperators(std::vector<Decl*>& decls, gulc::InfixOperators findOperator,
                                                           gulc::Type* argType,
                                                           std::vector<MatchingDecl>& matchingDecls) {
    TypeCompareUtil typeCompareUtil;
    bool matchFound = false;

    for (Decl* checkDecl : decls) {
        if (llvm::isa<OperatorDecl>(checkDecl)) {
            auto checkOperatorDecl = llvm::dyn_cast<OperatorDecl>(checkDecl);

            if (checkOperatorDecl->operatorType() == OperatorType::Infix &&
                    checkOperatorDecl->operatorIdentifier().name() == getInfixOperatorStringValue(findOperator)) {
                // TODO: Support checking for casts
                if (typeCompareUtil.compareAreSameOrInherits(argType, checkOperatorDecl->parameters()[0]->type)) {
                    // It is a match. Keep looking in case of ambiguity
                    matchingDecls.emplace_back(MatchingDecl(MatchingDecl::Kind::Match, checkDecl));
                }
            }
        }
    }

    // TODO: We need to replace `inheritedMembers` with `allMembers` or something. Our way of detecting `exact` and
    //       then ignoring inherited won't work properly. Having `allMembers` will be the most efficient way to handle

    return matchFound;
}

void gulc::CodeProcessor::processIsExpr(gulc::IsExpr* isExpr) {
    processExpr(isExpr->expr);

    // TODO: Should we just do `const` solving here again?
}

void gulc::CodeProcessor::processLabeledArgumentExpr(gulc::LabeledArgumentExpr* labeledArgumentExpr) {
    processExpr(labeledArgumentExpr->argument);

    // We need the labeled argument type to be the same as the value type for the contained expression
    labeledArgumentExpr->valueType = labeledArgumentExpr->argument->valueType->deepCopy();
}

void gulc::CodeProcessor::processMemberAccessCallExpr(gulc::Expr*& expr) {
    // NOTE: We don't have to account for function calls here. `FunctionCallExpr` handles `MemberAccessCallExpr` as a
    //       special case. So there is no need for an ambiguity check, the first detect we match on is the result. All
    //       other redeclarations will be handled in another pass
    // NOTE: We DO need to check ambiguity on extensions. Multiple extensions could define the same decls.
    auto memberAccessCallExpr = llvm::dyn_cast<gulc::MemberAccessCallExpr>(expr);

    processExpr(memberAccessCallExpr->objectRef);

    // TODO: We will have to account for extensions
    // TODO: Support `NamespaceReferenceExpr` once it is created.
    if (llvm::isa<TypeExpr>(memberAccessCallExpr->objectRef)) {
        if (memberAccessCallExpr->isArrowCall()) {
            printError("operator `->` cannot be used on types!",
                       memberAccessCallExpr->startPosition(), memberAccessCallExpr->endPosition());
        }

        auto checkTypeExpr = llvm::dyn_cast<TypeExpr>(memberAccessCallExpr->objectRef);

        // TODO: Search extensions
        bool isAmbiguous = false;
        Decl* foundDecl = nullptr;

        switch (checkTypeExpr->type->getTypeKind()) {
            case Type::Kind::Enum: {
                auto checkEnum = llvm::dyn_cast<EnumType>(checkTypeExpr->type)->decl();
                // TODO:
//                foundDecl = findMatchingMemberDecl(checkStruct->ownedMembers(), memberAccessCallExpr->member, true, &isAmbiguous);
                printError("operator `.` not yet supported on `enum` types!",
                           memberAccessCallExpr->startPosition(), memberAccessCallExpr->endPosition());
                break;
            }
            case Type::Kind::Struct: {
                auto checkStruct = llvm::dyn_cast<StructType>(checkTypeExpr->type)->decl();
                foundDecl = findMatchingMemberDecl(checkStruct->allMembers, memberAccessCallExpr->member, true, &isAmbiguous);
                break;
            }
            case Type::Kind::Trait: {
                auto checkTrait = llvm::dyn_cast<TraitType>(checkTypeExpr->type)->decl();
                foundDecl = findMatchingMemberDecl(checkTrait->allMembers, memberAccessCallExpr->member, true, &isAmbiguous);
                break;
            }
            default:
                break;
        }

        if (isAmbiguous) {
            printError("member call `" + memberAccessCallExpr->toString() + "` is ambiguous!",
                       memberAccessCallExpr->startPosition(), memberAccessCallExpr->endPosition());
        }

        if (foundDecl == nullptr) {
            printError("type `" + checkTypeExpr->toString() + "` does not contain a member `" + memberAccessCallExpr->member->toString() + "`!",
                       memberAccessCallExpr->startPosition(), memberAccessCallExpr->endPosition());
        }

        // Support:
        //  * `EnumConst`
        //  * `prop`
        //  * `var`
        //  * `struct`, `class`, `union`
        //  * `enum`
        //  * `trait`
        switch (foundDecl->getDeclKind()) {
            case Decl::Kind::EnumConst: {
                auto newExpr = new EnumConstRefExpr(
                        memberAccessCallExpr->startPosition(),
                        memberAccessCallExpr->endPosition(),
                        llvm::dyn_cast<EnumConstDecl>(foundDecl)
                    );
                processEnumConstRefExpr(newExpr);
                // Delete the old reference and replace it with the new enum const reference.
                delete expr;
                expr = newExpr;
                return;
            }
            case Decl::Kind::Enum: {
                auto enumType = new EnumType(
                        Type::Qualifier::Unassigned,
                        llvm::dyn_cast<EnumDecl>(foundDecl),
                        expr->startPosition(),
                        expr->endPosition()
                    );
                auto newExpr = new TypeExpr(enumType);
                processTypeExpr(newExpr);
                // Delete the old reference and replace it with the new enum type expression.
                delete expr;
                expr = newExpr;
                return;
            }
            case Decl::Kind::CallOperator:
            case Decl::Kind::TemplateFunctionInst:
            case Decl::Kind::Function:
            case Decl::Kind::TemplateFunction: {
                // NOTE: `FunctionCallExpr` handles static member function calls as a special case. `Type.staticFunc`
                //       cannot be used to grab a function pointer, use `Type::staticFunc` to grab the pointer instead.
                printError("`.` cannot be used to grab a function pointer, use `::` instead!",
                           expr->startPosition(), expr->endPosition());
                return;
            }
            case Decl::Kind::Property: {
                auto newExpr = new PropertyRefExpr(
                        expr->startPosition(),
                        expr->endPosition(),
                        llvm::dyn_cast<PropertyDecl>(foundDecl)
                    );
                processPropertyRefExpr(newExpr);
                // Delete the old reference and replace it with the new property reference.
                delete expr;
                expr = newExpr;
                return;
            }
            case Decl::Kind::TemplateStructInst:
            case Decl::Kind::Struct: {
                auto structType = new StructType(
                        Type::Qualifier::Unassigned,
                        llvm::dyn_cast<StructDecl>(foundDecl),
                        expr->startPosition(),
                        expr->endPosition()
                    );
                auto newExpr = new TypeExpr(structType);
                processTypeExpr(newExpr);
                // Delete the old reference and replace it with the new struct type.
                delete expr;
                expr = newExpr;
                return;
            }
            case Decl::Kind::TemplateStruct: {
                // TODO: Instantiate the struct and do the same as above.
                printError("[INTERNAL] referencing template structs outside of designated areas not yet supported!",
                           expr->startPosition(), expr->endPosition());
                break;
            }
            case Decl::Kind::TemplateTraitInst:
            case Decl::Kind::Trait: {
                auto traitType = new TraitType(
                        Type::Qualifier::Unassigned,
                        llvm::dyn_cast<TraitDecl>(foundDecl),
                        expr->startPosition(),
                        expr->endPosition()
                    );
                auto newExpr = new TypeExpr(traitType);
                processTypeExpr(newExpr);
                // Delete the old reference and replace it with the new trait type.
                delete expr;
                expr = newExpr;
                return;
            }
            case Decl::Kind::TemplateTrait: {
                // TODO: Instantiate the trait and do the same as above.
                printError("[INTERNAL] referencing template traits outside of designated areas not yet supported!",
                           expr->startPosition(), expr->endPosition());
                break;
            }
            case Decl::Kind::Variable: {
                auto newExpr = new VariableRefExpr(
                        expr->startPosition(),
                        expr->endPosition(),
                        llvm::dyn_cast<VariableDecl>(foundDecl)
                    );
                processVariableRefExpr(newExpr);
                // Delete the old reference and replace it with the new variable reference.
                delete expr;
                expr = newExpr;
                return;
            }
            default:
                printError("type `" + checkTypeExpr->toString() + "` does not contain a member `" + memberAccessCallExpr->member->toString() + "`!",
                           memberAccessCallExpr->startPosition(), memberAccessCallExpr->endPosition());
        }
    } else {
        auto checkType = memberAccessCallExpr->objectRef->valueType;

        if (llvm::isa<ReferenceType>(checkType)) {
            checkType = llvm::dyn_cast<ReferenceType>(checkType)->nestedType;
        }

        if (memberAccessCallExpr->isArrowCall()) {
            // TODO: Check for operator overload if this isn't a pointer
            if (!llvm::isa<PointerType>(checkType)) {
                printError("operator `->` does not exist for non-pointer type `" + checkType->toString() + "`!",
                           memberAccessCallExpr->startPosition(), memberAccessCallExpr->endPosition());
            }

            checkType = llvm::dyn_cast<PointerType>(checkType)->nestedType;
        }

        // TODO: Search extensions
        bool isAmbiguous = false;
        Decl* foundDecl = nullptr;

        switch (checkType->getTypeKind()) {
            case Type::Kind::Enum: {
                auto checkEnumType = llvm::dyn_cast<EnumType>(checkType);
                auto checkEnum = checkEnumType->decl();
                // TODO:
//                searchDecls = &checkEnum->ownedMembers;
                printError("operator `.` not yet supported on `enum` types!",
                           memberAccessCallExpr->startPosition(), memberAccessCallExpr->endPosition());
                break;
            }
            case Type::Kind::Struct: {
                auto checkStructType = llvm::dyn_cast<StructType>(checkType);
                auto checkStruct = checkStructType->decl();
                foundDecl = findMatchingMemberDecl(checkStruct->allMembers, memberAccessCallExpr->member, false, &isAmbiguous);
                break;
            }
            case Type::Kind::Trait: {
                auto checkTraitType = llvm::dyn_cast<TraitType>(checkType);
                auto checkTrait = checkTraitType->decl();
                foundDecl = findMatchingMemberDecl(checkTrait->allMembers, memberAccessCallExpr->member, false, &isAmbiguous);
                break;
            }
            default:
                // Method not found...
                break;
        }

        if (isAmbiguous) {
            printError("member call `" + memberAccessCallExpr->toString() + "` is ambiguous!",
                       memberAccessCallExpr->startPosition(), memberAccessCallExpr->endPosition());
        }

        if (foundDecl == nullptr) {
            printError("type `" + checkType->toString() + "` does not contain a member `" + memberAccessCallExpr->member->toString() + "`!",
                       memberAccessCallExpr->startPosition(), memberAccessCallExpr->endPosition());
        }

        // If we've reached this point we need to convert any lvalue object references to rvalues and dereference any
        // implicit references
        // We only convert lvalue to rvalue if the object is a reference type. The reason for this is because we
        // actually need SOME type of reference to actually index tha data out of the struct. So `objectRef` either
        // needs to be an `lvalue` or a `ref`. If it is neither something is wrong
        if (llvm::isa<ReferenceType>(memberAccessCallExpr->objectRef->valueType)) {
            memberAccessCallExpr->objectRef = convertLValueToRValue(memberAccessCallExpr->objectRef);
        }

        // Support:
        //  * `prop`
        //  * `var`
        switch (foundDecl->getDeclKind()) {
            case Decl::Kind::EnumConst: {
                printError("accessing enum constants from instanced variables is not supported, use a type name instead!",
                           expr->startPosition(), expr->endPosition());
                return;
            }
            case Decl::Kind::CallOperator:
            case Decl::Kind::TemplateFunctionInst:
            case Decl::Kind::Function:
            case Decl::Kind::TemplateFunction: {
                // NOTE: `FunctionCallExpr` handles static member function calls as a special case. `Type.staticFunc`
                //       cannot be used to grab a function pointer, use `Type::staticFunc` to grab the pointer instead.
                printError("`.` cannot be used to grab a function pointer, use `typename::` instead!",
                           expr->startPosition(), expr->endPosition());
                return;
            }
            case Decl::Kind::Enum:
            case Decl::Kind::TemplateStructInst:
            case Decl::Kind::Struct:
            case Decl::Kind::TemplateStruct:
            case Decl::Kind::TemplateTraitInst:
            case Decl::Kind::Trait:
            case Decl::Kind::TemplateTrait: {
                printError("accessing types from instanced variables is not supported, use a type name instead!",
                           expr->startPosition(), expr->endPosition());
                return;
            }
            case Decl::Kind::Property: {
                auto newExpr = new MemberPropertyRefExpr(
                        expr->startPosition(),
                        expr->endPosition(),
                        memberAccessCallExpr->objectRef,
                        llvm::dyn_cast<PropertyDecl>(foundDecl)
                );
                processMemberPropertyRefExpr(newExpr);
                // We steal the actual object reference.
                memberAccessCallExpr->objectRef = nullptr;
                // Delete the old reference and replace it with the new property reference.
                delete expr;
                expr = newExpr;
                return;
            }
            case Decl::Kind::Variable: {
                auto varOwnerStructType =
                        new StructType(
                                Type::Qualifier::Unassigned,
                                llvm::dyn_cast<StructDecl>(foundDecl->container),
                                {}, {}
                            );
                auto newExpr = new MemberVariableRefExpr(
                        expr->startPosition(),
                        expr->endPosition(),
                        memberAccessCallExpr->objectRef,
                        varOwnerStructType,
                        llvm::dyn_cast<VariableDecl>(foundDecl)
                );
                processMemberVariableRefExpr(newExpr);
                // We steal the actual object reference.
                memberAccessCallExpr->objectRef = nullptr;
                // Delete the old reference and replace it with the new variable reference.
                delete expr;
                expr = newExpr;
                return;
            }
            default:
                printError("type `" + checkType->toString() + "` does not contain a member `" + memberAccessCallExpr->member->toString() + "`!",
                           memberAccessCallExpr->startPosition(), memberAccessCallExpr->endPosition());
        }
    }
}

gulc::Decl* gulc::CodeProcessor::findMatchingMemberDecl(std::vector<Decl*> const& searchDecls,
                                                        gulc::IdentifierExpr* memberIdentifier,
                                                        bool searchForStatic, bool* outIsAmbiguous) {
    Decl* foundDecl = nullptr;

    // TODO: We need to account for template parameters. It IS allowed to get a function pointer to an instantiated
    //       template function!
    //       Or is it?
    //       Also it is needed for `TemplateStruct` and `TemplateTrait`...
    // These will ONLY be templates, if a non-template matches then we set `foundDecl` directly
    std::vector<MatchingDecl> potentialMatches;

    for (Decl* checkDecl : searchDecls) {
        if (checkDecl->isStatic() == searchForStatic &&
                checkDecl->identifier().name() == memberIdentifier->identifier().name()) {
            if (memberIdentifier->hasTemplateArguments()) {
                std::vector<TemplateParameterDecl*>* checkTemplateParameters = nullptr;

                // Obviously if there are template parameters you can only call a template.
                if (llvm::isa<TemplateFunctionDecl>(checkDecl)) {
                    auto checkTemplateFunction = llvm::dyn_cast<TemplateFunctionDecl>(checkDecl);

                    checkTemplateParameters = &checkTemplateFunction->templateParameters();
                } else if (llvm::isa<TemplateStructDecl>(checkDecl)) {
                    auto checkTemplateStruct = llvm::dyn_cast<TemplateStructDecl>(checkDecl);

                    checkTemplateParameters = &checkTemplateStruct->templateParameters();
                } else if (llvm::isa<TemplateTraitDecl>(checkDecl)) {
                    auto checkTemplateTrait = llvm::dyn_cast<TemplateTraitDecl>(checkDecl);

                    checkTemplateParameters = &checkTemplateTrait->templateParameters();
                }

                if (checkTemplateParameters != nullptr) {
                    SignatureComparer::ArgMatchResult argMatchResult =
                            SignatureComparer::compareTemplateArgumentsToParameters(
                                    *checkTemplateParameters,
                                    memberIdentifier->templateArguments()
                            );

                    if (argMatchResult != SignatureComparer::ArgMatchResult::Fail) {
                        MatchingDecl::Kind matchKind =
                                argMatchResult == SignatureComparer::ArgMatchResult::Match
                                ? MatchingDecl::Kind::Match
                                : MatchingDecl::Kind::Castable;

                        potentialMatches.emplace_back(MatchingDecl(matchKind, checkDecl));
                    }
                }
            } else {
                // TODO: We should still check templates as long as all of the template has default values
                //       Doing so WILL require an ambiguity check though, or at least the ability to replace
                //       a template with default values call with an absolute, no template call.
                //       I.e.
                //           struct {
                //               func example<const i: i32 = 12>();
                //               func example();
                //           }
                //       In the above `var.example` would find the template first, be okay with it, and then
                //       find the non-template which it should then exit on using the non-template. If the user
                //       wants to use the template they should specify a template argument.
                if (!(llvm::isa<TemplateFunctionDecl>(checkDecl) || llvm::isa<TemplateStructDecl>(checkDecl) ||
                      llvm::isa<TemplateTraitDecl>(checkDecl))) {
                    foundDecl = checkDecl;
                    break;
                }
            }
        }
    }

    // If we didn't find a non-template declaration then we have to search the `potentialMatches` which will
    // contain only templates.
    // If there are more than 1 exact matches then we error out saying something was ambiguous, if there are
    // more than 1 castable matches then we error out saying something was ambiguous, but there is 1 castable
    // or a single exact match then we set `foundDecl` to that.
    // (NOTE: There can be multiple castable matches with a single exact match and we will choose the exact)
    if (foundDecl == nullptr) {
        if (potentialMatches.size() > 1) {
            // Search for a single exact match, if there are more than 1 exact match we error out saying it is
            // ambiguous
            for (MatchingDecl& checkMatch : potentialMatches) {
                if (checkMatch.kind == MatchingDecl::Kind::Match) {
                    if (foundDecl == nullptr) {
                        foundDecl = checkMatch.matchingDecl;
                    } else {
                        // Notify that the identifier is ambiguous and return nothing.
                        *outIsAmbiguous = true;
                        return nullptr;
                    }
                }
            }
        } else if (!potentialMatches.empty()) {
            // The single match is the `foundDecl`
            foundDecl = potentialMatches[0].matchingDecl;
        } else {
            // NOTE: There was no matches in this branch, we do nothing and let the below error be triggered.
        }
    }

    return foundDecl;
}

void gulc::CodeProcessor::processMemberPostfixOperatorCallExpr(
        gulc::MemberPostfixOperatorCallExpr* memberPostfixOperatorCallExpr) {
    // TODO: Do we need to do `lvalue -> rvalue`?
    if (memberPostfixOperatorCallExpr->postfixOperatorDecl->returnType != nullptr) {
        memberPostfixOperatorCallExpr->valueType =
                memberPostfixOperatorCallExpr->postfixOperatorDecl->returnType->deepCopy();
        memberPostfixOperatorCallExpr->valueType->setIsLValue(true);
    }
}

void gulc::CodeProcessor::processMemberPrefixOperatorCallExpr(
        gulc::MemberPrefixOperatorCallExpr* memberPrefixOperatorCallExpr) {
    // TODO: Do we need to do `lvalue -> rvalue`?
    if (memberPrefixOperatorCallExpr->prefixOperatorDecl->returnType != nullptr) {
        memberPrefixOperatorCallExpr->valueType =
                memberPrefixOperatorCallExpr->prefixOperatorDecl->returnType->deepCopy();
        memberPrefixOperatorCallExpr->valueType->setIsLValue(true);
    }
}

void gulc::CodeProcessor::processMemberPropertyRefExpr(gulc::MemberPropertyRefExpr* memberPropertyRefExpr) {
    // NOTE: This doesn't ever need to do anything. Properties are handled by the assignment operator handling it
    //       as a special case. Any other function should make a call to convert any `get/set` decls into their getter
    //       forms.
}

void gulc::CodeProcessor::processMemberSubscriptCallExpr(gulc::MemberSubscriptCallExpr* memberSubscriptCallExpr) {
    // SEE `processMemberPropertyRefExpr`. Same deal applies here.
}

void gulc::CodeProcessor::processMemberVariableRefExpr(gulc::MemberVariableRefExpr* memberVariableRefExpr) {
    auto varType = memberVariableRefExpr->variableDecl()->type->deepCopy();
    // TODO: Based on `object` mutability for `varType`
    // TODO: Do we need to do `lvalue -> rvalue`?

    memberVariableRefExpr->valueType = varType;
    memberVariableRefExpr->valueType->setIsLValue(true);
}

void gulc::CodeProcessor::processParenExpr(gulc::ParenExpr* parenExpr) {
    processExpr(parenExpr->nestedExpr);

    parenExpr->valueType = parenExpr->nestedExpr->valueType->deepCopy();
}

void gulc::CodeProcessor::processPostfixOperatorExpr(gulc::PostfixOperatorExpr*& postfixOperatorExpr) {
    processExpr(postfixOperatorExpr->nestedExpr);

    auto checkType = postfixOperatorExpr->nestedExpr->valueType;

    if (llvm::isa<ReferenceType>(checkType)) {
        checkType = llvm::dyn_cast<ReferenceType>(checkType)->nestedType;
    }

    // NOTE: Postfix operators are non-static members of a class so as long as the type of
    //       `nestedExpr` contains a correctly named operator decl we have a match.
    //       We still need to keep a list of `MatchingDecl` which WON'T contain castable, we
    //       only need that list for differentiating between `mut` and `immut` operators the
    //       same way we do with everything else.
    std::vector<MatchingDecl> matchingOperators;

    // TODO: Check extensions
    switch (checkType->getTypeKind()) {
        case Type::Kind::Enum: {
            printError("using postfix operators on enum instances not yet supported!",
                       postfixOperatorExpr->startPosition(), postfixOperatorExpr->endPosition());
            break;
        }
        case Type::Kind::Struct: {
            auto structDecl = llvm::dyn_cast<StructType>(checkType)->decl();

            for (Decl* checkDecl : structDecl->allMembers) {
                if (!checkDecl->isStatic() && llvm::isa<OperatorDecl>(checkDecl)) {
                    auto checkOperator = llvm::dyn_cast<OperatorDecl>(checkDecl);

                    if (checkOperator->operatorType() == OperatorType::Postfix) {
                        // NOTE: For now we're comparing string values to check if the operator is the same.
                        //       That was because I originally planned on making user-defined operators through the
                        //       same declaration but I have since decided against that. We will at some point need
                        //       to change `OperatorDecl` into three new `Decl` -
                        //       `InfixOperatorDecl`, `PostfixOperatorDecl`, `PrefixOperatorDecl`.
                        if (gulc::getPostfixOperatorString(postfixOperatorExpr->postfixOperator()) ==
                                checkOperator->operatorIdentifier().name()) {
                            matchingOperators.emplace_back(MatchingDecl(MatchingDecl::Kind::Match, checkOperator));
                        }
                    }
                }
            }

            break;
        }
        case Type::Kind::Trait: {
            auto traitDecl = llvm::dyn_cast<TraitType>(checkType)->decl();

            for (Decl* checkDecl : traitDecl->allMembers) {
                if (!checkDecl->isStatic() && llvm::isa<OperatorDecl>(checkDecl)) {
                    auto checkOperator = llvm::dyn_cast<OperatorDecl>(checkDecl);

                    if (checkOperator->operatorType() == OperatorType::Postfix) {
                        // NOTE: For now we're comparing string values to check if the operator is the same.
                        //       That was because I originally planned on making user-defined operators through the
                        //       same declaration but I have since decided against that. We will at some point need
                        //       to change `OperatorDecl` into three new `Decl` -
                        //       `InfixOperatorDecl`, `PostfixOperatorDecl`, `PrefixOperatorDecl`.
                        if (gulc::getPostfixOperatorString(postfixOperatorExpr->postfixOperator()) ==
                                checkOperator->operatorIdentifier().name()) {
                            matchingOperators.emplace_back(MatchingDecl(MatchingDecl::Kind::Match, checkOperator));
                        }
                    }
                }
            }

            break;
        }
        default:
            break;
    }

    if (!matchingOperators.empty()) {
        Decl* foundDecl = nullptr;

        if (matchingOperators.size() == 1) {
            if (checkType->qualifier() != Type::Qualifier::Mut) {
                // NOTE: For `mut` the operator can be both `immut` or `mut`, but anything else the operator MUST be
                //       `immut`
                if (matchingOperators[0].matchingDecl->isMutable()) {
                    printError("call to `mut` operator with `immut` reference!",
                               postfixOperatorExpr->startPosition(), postfixOperatorExpr->endPosition());
                }
            }

            foundDecl = matchingOperators[0].matchingDecl;
        } else {
            // NOTE: `matchingOperators.size()` SHOULD only be `2` here. 1 mut and 1 immut.
            if (checkType->qualifier() == Type::Qualifier::Mut) {
                if (matchingOperators[0].matchingDecl->isMutable()) {
                    foundDecl = matchingOperators[0].matchingDecl;
                } else if (matchingOperators[1].matchingDecl->isMutable()) {
                    foundDecl = matchingOperators[1].matchingDecl;
                } else {
                    // If both are immut we choose the first match (NOTE: This should NEVER happen)
                    foundDecl = matchingOperators[0].matchingDecl;
                }
            } else {
                if (!matchingOperators[0].matchingDecl->isMutable()) {
                    foundDecl = matchingOperators[0].matchingDecl;
                } else if (!matchingOperators[1].matchingDecl->isMutable()) {
                    foundDecl = matchingOperators[1].matchingDecl;
                } else {
                    // If both are mut we error out
                    printError("no matching immutable operator found, only `mut` function found!",
                               postfixOperatorExpr->startPosition(), postfixOperatorExpr->endPosition());
                }
            }
        }

        // Replace the postfix operator call with a member postfix operator call
        auto postfixOperatorDecl = llvm::dyn_cast<OperatorDecl>(foundDecl);
        auto newExpr = new MemberPostfixOperatorCallExpr(
                postfixOperatorDecl,
                postfixOperatorExpr->postfixOperator(),
                postfixOperatorExpr->nestedExpr,
                postfixOperatorExpr->startPosition(),
                postfixOperatorExpr->endPosition());
        processMemberPostfixOperatorCallExpr(newExpr);
        delete postfixOperatorExpr;
        postfixOperatorExpr = newExpr;
        return;
    }

    if (checkType->qualifier() != Type::Qualifier::Mut) {
        printError("built in postfix operator `" +
                   gulc::getPostfixOperatorString(postfixOperatorExpr->postfixOperator()) +
                   "` can only be used on `mut` values!",
                   postfixOperatorExpr->startPosition(), postfixOperatorExpr->endPosition());
    }

    // NOTE: `Pointer` basically is a built in integer type. But we treat it special. Using `++` is legal on a pointer
    //       but can only be used within `unsafe` areas.
    if (!llvm::isa<BuiltInType>(checkType) && !llvm::isa<PointerType>(checkType)) {
        printError("postfix operator `" +
                   gulc::getPostfixOperatorString(postfixOperatorExpr->postfixOperator()) +
                   "` was not found for type `" +
                   postfixOperatorExpr->nestedExpr->valueType->toString() + "`!",
                   postfixOperatorExpr->startPosition(), postfixOperatorExpr->endPosition());
    }

    // Once we reach this point we handle lvalue to rvalue and dereference implicit references
    postfixOperatorExpr->nestedExpr = convertLValueToRValue(postfixOperatorExpr->nestedExpr);
    postfixOperatorExpr->nestedExpr = dereferenceReference(postfixOperatorExpr->nestedExpr);

    // At this point it is validated as a built in operator, we clone the `checkType` as the return type instead of
    // `valueType` to remove any references.
    // The result of `++` is NOT modifiable.
    postfixOperatorExpr->valueType = checkType->deepCopy();
    postfixOperatorExpr->valueType->setIsLValue(false);
}

void gulc::CodeProcessor::processPrefixOperatorExpr(gulc::PrefixOperatorExpr*& prefixOperatorExpr) {
    switch (prefixOperatorExpr->prefixOperator()) {
        // TODO: These currently need special treatment.
        //  NOTE: These also CANNOT be overloaded so we don't need to search extensions or look at the type.
        case PrefixOperators::SizeOf:
        case PrefixOperators::AlignOf:
        case PrefixOperators::OffsetOf:
        case PrefixOperators::NameOf:
        case PrefixOperators::TraitsOf:
            printError("built in prefix operator not yet supported!",
                       prefixOperatorExpr->startPosition(), prefixOperatorExpr->endPosition());
            break;
        default:
            break;
    }

    processExpr(prefixOperatorExpr->nestedExpr);

    auto checkType = prefixOperatorExpr->nestedExpr->valueType;

    if (llvm::isa<ReferenceType>(checkType)) {
        checkType = llvm::dyn_cast<ReferenceType>(checkType)->nestedType;
    }

    // NOTE: Prefix operators are non-static members of a class so as long as the type of
    //       `nestedExpr` contains a correctly named operator decl we have a match.
    //       We still need to keep a list of `MatchingDecl` which WON'T contain castable, we
    //       only need that list for differentiating between `mut` and `immut` operators the
    //       same way we do with everything else.
    std::vector<MatchingDecl> matchingOperators;

    // TODO: Check extensions
    switch (checkType->getTypeKind()) {
        case Type::Kind::Enum: {
            printError("using prefix operators on enum instances not yet supported!",
                       prefixOperatorExpr->startPosition(), prefixOperatorExpr->endPosition());
            break;
        }
        case Type::Kind::Struct: {
            auto structDecl = llvm::dyn_cast<StructType>(checkType)->decl();

            for (Decl* checkDecl : structDecl->allMembers) {
                if (!checkDecl->isStatic() && llvm::isa<OperatorDecl>(checkDecl)) {
                    auto checkOperator = llvm::dyn_cast<OperatorDecl>(checkDecl);

                    if (checkOperator->operatorType() == OperatorType::Prefix) {
                        // NOTE: For now we're comparing string values to check if the operator is the same.
                        //       That was because I originally planned on making user-defined operators through the
                        //       same declaration but I have since decided against that. We will at some point need
                        //       to change `OperatorDecl` into three new `Decl` -
                        //       `InfixOperatorDecl`, `PostfixOperatorDecl`, `PrefixOperatorDecl`.
                        if (gulc::getPrefixOperatorString(prefixOperatorExpr->prefixOperator()) ==
                                checkOperator->operatorIdentifier().name()) {
                            matchingOperators.emplace_back(MatchingDecl(MatchingDecl::Kind::Match, checkOperator));
                        }
                    }
                }
            }

            break;
        }
        case Type::Kind::Trait: {
            auto traitDecl = llvm::dyn_cast<TraitType>(checkType)->decl();

            for (Decl* checkDecl : traitDecl->allMembers) {
                if (!checkDecl->isStatic() && llvm::isa<OperatorDecl>(checkDecl)) {
                    auto checkOperator = llvm::dyn_cast<OperatorDecl>(checkDecl);

                    if (checkOperator->operatorType() == OperatorType::Prefix) {
                        // NOTE: For now we're comparing string values to check if the operator is the same.
                        //       That was because I originally planned on making user-defined operators through the
                        //       same declaration but I have since decided against that. We will at some point need
                        //       to change `OperatorDecl` into three new `Decl` -
                        //       `InfixOperatorDecl`, `PostfixOperatorDecl`, `PrefixOperatorDecl`.
                        if (gulc::getPrefixOperatorString(prefixOperatorExpr->prefixOperator()) ==
                                checkOperator->operatorIdentifier().name()) {
                            matchingOperators.emplace_back(MatchingDecl(MatchingDecl::Kind::Match, checkOperator));
                        }
                    }
                }
            }

            break;
        }
        default:
            break;
    }

    if (!matchingOperators.empty()) {
        Decl* foundDecl = nullptr;

        if (matchingOperators.size() == 1) {
            if (checkType->qualifier() != Type::Qualifier::Mut) {
                // NOTE: For `mut` the operator can be both `immut` or `mut`, but anything else the operator MUST be
                //       `immut`
                if (matchingOperators[0].matchingDecl->isMutable()) {
                    printError("call to `mut` operator with `immut` reference!",
                               prefixOperatorExpr->startPosition(), prefixOperatorExpr->endPosition());
                }
            }

            foundDecl = matchingOperators[0].matchingDecl;
        } else {
            // NOTE: `matchingOperators.size()` SHOULD only be `2` here. 1 mut and 1 immut.
            if (checkType->qualifier() == Type::Qualifier::Mut) {
                if (matchingOperators[0].matchingDecl->isMutable()) {
                    foundDecl = matchingOperators[0].matchingDecl;
                } else if (matchingOperators[1].matchingDecl->isMutable()) {
                    foundDecl = matchingOperators[1].matchingDecl;
                } else {
                    // If both are immut we choose the first match (NOTE: This should NEVER happen)
                    foundDecl = matchingOperators[0].matchingDecl;
                }
            } else {
                if (!matchingOperators[0].matchingDecl->isMutable()) {
                    foundDecl = matchingOperators[0].matchingDecl;
                } else if (!matchingOperators[1].matchingDecl->isMutable()) {
                    foundDecl = matchingOperators[1].matchingDecl;
                } else {
                    // If both are mut we error out
                    printError("no matching immutable operator found, only `mut` function found!",
                               prefixOperatorExpr->startPosition(), prefixOperatorExpr->endPosition());
                }
            }
        }

        // Replace the prefix operator call with a member prefix operator call
        auto prefixOperatorDecl = llvm::dyn_cast<OperatorDecl>(foundDecl);
        auto newExpr = new MemberPrefixOperatorCallExpr(
                prefixOperatorDecl,
                prefixOperatorExpr->prefixOperator(),
                prefixOperatorExpr->nestedExpr,
                prefixOperatorExpr->startPosition(),
                prefixOperatorExpr->endPosition());
        processMemberPrefixOperatorCallExpr(newExpr);
        delete prefixOperatorExpr;
        prefixOperatorExpr = newExpr;
        return;
    }

    // TODO: We still need to handle mutability better
    switch (prefixOperatorExpr->prefixOperator()) {
        case PrefixOperators::Increment:
        case PrefixOperators::Decrement:
            // NOTE: Increment and decrement require `mut`, the others can be `immut`
            if (checkType->qualifier() != Type::Qualifier::Mut) {
                printError("built in prefix operator `" +
                           gulc::getPrefixOperatorString(prefixOperatorExpr->prefixOperator()) +
                           "` can only be used on `mut` values!",
                           prefixOperatorExpr->startPosition(), prefixOperatorExpr->endPosition());
            }

            // NOTE: `Pointer` basically is a built in integer type. But we treat it special. Using `++` is legal on a
            //       pointer but can only be used within `unsafe` areas.
            if (!llvm::isa<BuiltInType>(checkType) && !llvm::isa<PointerType>(checkType)) {
                printError("prefix operator `" +
                           gulc::getPrefixOperatorString(prefixOperatorExpr->prefixOperator()) +
                           "` was not found for type `" +
                           prefixOperatorExpr->nestedExpr->valueType->toString() + "`!",
                           prefixOperatorExpr->startPosition(), prefixOperatorExpr->endPosition());
            }

            // At this point it is validated as a built in operator, we clone the `valueType` as the return type.
            // The result of `++` is still modifiable.
            prefixOperatorExpr->valueType = prefixOperatorExpr->nestedExpr->valueType->deepCopy();
            prefixOperatorExpr->valueType->setIsLValue(true);
            break;
        case PrefixOperators::LogicalNot:
            // NOTE: `!` can ONLY be applied to `bool` type with built in and MUST return `bool`
            if (!llvm::isa<BoolType>(checkType)) {
                printError("prefix operator `!` expects type `bool`, found `" + checkType->toString() + "`!",
                           prefixOperatorExpr->startPosition(), prefixOperatorExpr->endPosition());
            }

            // Convert lvalue to rvalue and dereference implicit references (to get value type)
            prefixOperatorExpr->nestedExpr = convertLValueToRValue(prefixOperatorExpr->nestedExpr);
            prefixOperatorExpr->nestedExpr = dereferenceReference(prefixOperatorExpr->nestedExpr);

            // With `!` and `~` we remove any references, they are `immut` operators.
            prefixOperatorExpr->valueType = new BoolType(Type::Qualifier::Immut, {}, {});
            break;
        case PrefixOperators::BitwiseNot:
            // NOTE: `~` CAN be applied to pointers as well, only in `unsafe` areas.
            if (!llvm::isa<BuiltInType>(checkType) && !llvm::isa<PointerType>(checkType)) {
                printError("prefix operator `" +
                           gulc::getPrefixOperatorString(prefixOperatorExpr->prefixOperator()) +
                           "` was not found for type `" +
                           prefixOperatorExpr->nestedExpr->valueType->toString() + "`!",
                           prefixOperatorExpr->startPosition(), prefixOperatorExpr->endPosition());
            }

            // Convert lvalue to rvalue and dereference implicit references (to get value type)
            prefixOperatorExpr->nestedExpr = convertLValueToRValue(prefixOperatorExpr->nestedExpr);
            prefixOperatorExpr->nestedExpr = dereferenceReference(prefixOperatorExpr->nestedExpr);

            // With `!` and `~` we remove any references, they are `immut` operators.
            prefixOperatorExpr->valueType = checkType->deepCopy();
            prefixOperatorExpr->valueType->setIsLValue(false);
            break;
        case PrefixOperators::Positive:
            // NOTE: `+` does nothing by default. There is no built in `abs` function
            printError("prefix operator `+` was not found for type `" +
                       prefixOperatorExpr->nestedExpr->valueType->toString() + "`!",
                       prefixOperatorExpr->startPosition(), prefixOperatorExpr->endPosition());
            break;
        case PrefixOperators::Negative:
            // NOTE: `-` only works on built in types, pointers cannot be made negative.
            if (!llvm::isa<BuiltInType>(checkType)) {
                printError("prefix operator `-` was not found for type `" +
                           prefixOperatorExpr->nestedExpr->valueType->toString() + "`!",
                           prefixOperatorExpr->startPosition(), prefixOperatorExpr->endPosition());
            }

            // Convert lvalue to rvalue and dereference implicit references (to get value type)
            prefixOperatorExpr->nestedExpr = convertLValueToRValue(prefixOperatorExpr->nestedExpr);
            prefixOperatorExpr->nestedExpr = dereferenceReference(prefixOperatorExpr->nestedExpr);

            // `-` removes any references, they are `immut` operators.
            prefixOperatorExpr->valueType = checkType->deepCopy();
            prefixOperatorExpr->valueType->setIsLValue(false);
            break;
        case PrefixOperators::Dereference:
            // NOTE: Obviously you cannot deference a non-pointer type by default.
            //       Also, dereferencing a pointer requires `unsafe` as it could be null.
            if (!llvm::isa<PointerType>(checkType)) {
                printError("prefix operator `*` was not found for type `" +
                           prefixOperatorExpr->nestedExpr->valueType->toString() + "`!",
                           prefixOperatorExpr->startPosition(), prefixOperatorExpr->endPosition());
            }

            // Convert lvalue to rvalue and dereference implicit references (to get value type)
            prefixOperatorExpr->nestedExpr = convertLValueToRValue(prefixOperatorExpr->nestedExpr);
            prefixOperatorExpr->nestedExpr = dereferenceReference(prefixOperatorExpr->nestedExpr);

            // Grab the type that is pointed to by the pointer.
            prefixOperatorExpr->valueType = llvm::dyn_cast<PointerType>(checkType)->nestedType->deepCopy();
            break;
        case PrefixOperators::Reference:
            // NOTE: To reference you either need an lvalue or a reference.
            if (!llvm::isa<ReferenceType>(prefixOperatorExpr->nestedExpr->valueType) ||
                    !prefixOperatorExpr->nestedExpr->valueType->isLValue()) {
                printError("cannot grab pointer of non-reference, non-lvalue type `" +
                           prefixOperatorExpr->nestedExpr->valueType->toString() + "`!",
                           prefixOperatorExpr->startPosition(), prefixOperatorExpr->endPosition());
            }

            // NOTE: If it is BOTH a reference and lvalue then we need to convert from lvalue to rvalue
            if (llvm::isa<ReferenceType>(prefixOperatorExpr->nestedExpr->valueType) &&
                    prefixOperatorExpr->nestedExpr->valueType->isLValue()) {
                prefixOperatorExpr->nestedExpr = convertLValueToRValue(prefixOperatorExpr->nestedExpr);
            }

            // Make the result type a pointer to the `checkType`
            prefixOperatorExpr->valueType = new PointerType(
                    prefixOperatorExpr->nestedExpr->valueType->qualifier(),
                    checkType->deepCopy()
                );
            break;
        default:
            printError("prefix operator `" +
                       gulc::getPrefixOperatorString(prefixOperatorExpr->prefixOperator()) +
                       "` was not found for type `" +
                       prefixOperatorExpr->nestedExpr->valueType->toString() + "`!",
                       prefixOperatorExpr->startPosition(), prefixOperatorExpr->endPosition());
            break;
    }
}

void gulc::CodeProcessor::processPropertyRefExpr(gulc::PropertyRefExpr* propertyRefExpr) {
    // NOTE: We don't do anything here. `PropertyRef` and `SubscriptRef` are special references to `get/set` decls that
    //       are handled special within assignments. All other expressions should make a call to convert `get/set`
    //       references to only their getter forms.
}

void gulc::CodeProcessor::processRefExpr(gulc::RefExpr* refExpr) {
    processExpr(refExpr->nestedExpr);

    if (refExpr->nestedExpr->valueType->isLValue()) {
        // TODO: What about Unassigned?
        if (refExpr->isMutable && refExpr->nestedExpr->valueType->qualifier() != Type::Qualifier::Mut) {
            printError("cannot create `ref mut` from immutable expression!",
                       refExpr->startPosition(), refExpr->endPosition());
        }

        auto valueType = refExpr->nestedExpr->valueType->deepCopy();
        valueType->setIsLValue(false);

        refExpr->valueType = new ReferenceType(Type::Qualifier::Unassigned, valueType);
    } else {
        printError("cannot create reference to non-lvalue expression!",
                   refExpr->startPosition(), refExpr->endPosition());
    }
}

void gulc::CodeProcessor::processSubscriptCallExpr(gulc::Expr*& expr) {
    auto subscriptCallExpr = llvm::dyn_cast<SubscriptCallExpr>(expr);

    processExpr(subscriptCallExpr->subscriptReference);

    for (LabeledArgumentExpr* argument : subscriptCallExpr->arguments) {
        processLabeledArgumentExpr(argument);
    }

    // TODO: Support searching extensions
    if (llvm::isa<TypeExpr>(subscriptCallExpr->subscriptReference)) {
        auto searchTypeExpr = llvm::dyn_cast<TypeExpr>(subscriptCallExpr->subscriptReference);

        bool isAmbiguous = false;
        SubscriptOperatorDecl* foundSubscriptOperator = nullptr;

        switch (searchTypeExpr->type->getTypeKind()) {
            case Type::Kind::Enum: {
                auto searchEnumType = llvm::dyn_cast<EnumType>(searchTypeExpr->type);
                auto searchEnum = searchEnumType->decl();

                printError("enum types currently do not support static subscript operators!",
                           subscriptCallExpr->startPosition(), subscriptCallExpr->endPosition());
                // TODO: Support this
//                searchDecls = &searchEnum->ownedMembers();
                break;
            }
            case Type::Kind::Struct: {
                auto searchStructType = llvm::dyn_cast<StructType>(searchTypeExpr->type);
                auto searchStruct = searchStructType->decl();

                foundSubscriptOperator = findMatchingSubscriptOperator(searchStruct->allMembers,
                                                                       subscriptCallExpr->arguments,
                                                                       true, &isAmbiguous);
                break;
            }
            case Type::Kind::Trait: {
                auto searchTraitType = llvm::dyn_cast<TraitType>(searchTypeExpr->type);
                auto searchTrait = searchTraitType->decl();

                foundSubscriptOperator = findMatchingSubscriptOperator(searchTrait->allMembers,
                                                                       subscriptCallExpr->arguments,
                                                                       true, &isAmbiguous);
                break;
            }
            default:
                printError("specified type `" + searchTypeExpr->type->toString() + "` does not contain a static subscript operator!",
                           subscriptCallExpr->startPosition(), subscriptCallExpr->endPosition());
                break;
        }

        if (isAmbiguous) {
            printError("subscript operator call is ambiguous!",
                       subscriptCallExpr->startPosition(), subscriptCallExpr->endPosition());
        }

        if (foundSubscriptOperator == nullptr) {
            printError("specified type `" + searchTypeExpr->type->toString() + "` does not contain a static subscript operator matching the specified parameters!",
                       subscriptCallExpr->startPosition(), subscriptCallExpr->endPosition());
        }

        auto newSubscriptReference = new SubscriptRefExpr(
                subscriptCallExpr->startPosition(),
                subscriptCallExpr->endPosition(),
                foundSubscriptOperator
            );
        processSubscriptRefExpr(newSubscriptReference);
        // Delete the old reference and replace it with the new reference
        delete subscriptCallExpr->subscriptReference;
        subscriptCallExpr->subscriptReference = newSubscriptReference;
        // NOTE: We don't set `valueType` as `SubscriptOperatorDecl` is a special `get/set` decl that will need further
        //       processing by whatever contains the `SubscriptCallExpr`.
    } else {
        auto searchType = subscriptCallExpr->subscriptReference->valueType;

        if (llvm::isa<ReferenceType>(searchType)) {
            searchType = llvm::dyn_cast<ReferenceType>(searchType)->nestedType;
        }

        bool isAmbiguous = false;
        SubscriptOperatorDecl* foundSubscriptOperator = nullptr;

        switch (searchType->getTypeKind()) {
            case Type::Kind::Enum:
                printError("enums currently do not support subscript operators!",
                           subscriptCallExpr->startPosition(), subscriptCallExpr->endPosition());
                break;
            case Type::Kind::Struct: {
                auto structType = llvm::dyn_cast<StructType>(searchType);
                auto searchStruct = structType->decl();

                foundSubscriptOperator = findMatchingSubscriptOperator(searchStruct->allMembers,
                                                                       subscriptCallExpr->arguments,
                                                                       true, &isAmbiguous);
                break;
            }
            case Type::Kind::Trait: {
                auto traitType = llvm::dyn_cast<TraitType>(searchType);
                auto searchTrait = traitType->decl();

                foundSubscriptOperator = findMatchingSubscriptOperator(searchTrait->allMembers,
                                                                       subscriptCallExpr->arguments,
                                                                       true, &isAmbiguous);
                break;
            }
            default:
                printError("specified type `" + searchType->toString() + "` does not contain a subscript operator!",
                           subscriptCallExpr->startPosition(), subscriptCallExpr->endPosition());
                break;
        }

        if (isAmbiguous) {
            printError("subscript operator call is ambiguous!",
                       subscriptCallExpr->startPosition(), subscriptCallExpr->endPosition());
        }

        if (foundSubscriptOperator == nullptr) {
            printError("specified type `" + searchType->toString() + "` does not contain a subscript operator matching the specified parameters!",
                       subscriptCallExpr->startPosition(), subscriptCallExpr->endPosition());
        }

        auto newSubscriptReference = new SubscriptRefExpr(
                subscriptCallExpr->startPosition(),
                subscriptCallExpr->endPosition(),
                foundSubscriptOperator
        );
        processSubscriptRefExpr(newSubscriptReference);

        // TODO: Replace `subscriptCallExpr` with `MemberSubscriptCallExpr` with the object being the `self` argument.
        auto newExpr = new MemberSubscriptCallExpr(
                newSubscriptReference,
                subscriptCallExpr->subscriptReference,
                subscriptCallExpr->arguments,
                subscriptCallExpr->startPosition(),
                subscriptCallExpr->endPosition()
            );
        // NOTE: We steal both of these
        subscriptCallExpr->subscriptReference = nullptr;
        subscriptCallExpr->arguments.clear();
        processMemberSubscriptCallExpr(newExpr);
        // Delete the old expression and replace it with the member version.
        delete expr;
        expr = newExpr;
    }
}

gulc::SubscriptOperatorDecl* gulc::CodeProcessor::findMatchingSubscriptOperator(std::vector<Decl*>& searchDecls,
                                                                                std::vector<LabeledArgumentExpr*>& arguments,
                                                                                bool findStatic, bool* outIsAmbiguous) {
    std::vector<MatchingDecl> matchingDecls;

    for (Decl* checkDecl : searchDecls) {
        if (checkDecl->isStatic() == findStatic && llvm::isa<SubscriptOperatorDecl>(checkDecl)) {
            auto checkSubscriptOperator = llvm::dyn_cast<SubscriptOperatorDecl>(checkDecl);

            SignatureComparer::ArgMatchResult argMatchResult =
                    SignatureComparer::compareArgumentsToParameters(
                            checkSubscriptOperator->parameters(),
                            arguments
                    );

            if (argMatchResult != SignatureComparer::ArgMatchResult::Fail) {
                MatchingDecl::Kind matchKind =
                        argMatchResult == SignatureComparer::ArgMatchResult::Match
                        ? MatchingDecl::Kind::Match
                        : MatchingDecl::Kind::Castable;

                matchingDecls.emplace_back(MatchingDecl(matchKind, checkDecl));
            }
        }
    }

    SubscriptOperatorDecl* foundSubscriptOperator = nullptr;

    if (matchingDecls.size() > 1) {
        // There are multiple matches. If we don't find an exact match or there are multiple exact matches, we have
        // to error out saying the subscript call is ambiguous.
        for (MatchingDecl& checkMatch : matchingDecls) {
            if (checkMatch.kind == MatchingDecl::Kind::Match) {
                if (foundSubscriptOperator != nullptr) {
                    // If there are multiple exact matches then the call was ambiguous.
                    *outIsAmbiguous = true;
                    return nullptr;
                } else {
                    foundSubscriptOperator = llvm::dyn_cast<SubscriptOperatorDecl>(checkMatch.matchingDecl);
                }
            }
        }
    } else if (!matchingDecls.empty()) {
        // There is only one match so we choose it no matter what.
        foundSubscriptOperator = llvm::dyn_cast<SubscriptOperatorDecl>(matchingDecls[0].matchingDecl);
    }

    return foundSubscriptOperator;
}

void gulc::CodeProcessor::processSubscriptRefExpr(gulc::SubscriptRefExpr* subscriptReferenceExpr) {

}

void gulc::CodeProcessor::processTemplateConstRefExpr(gulc::TemplateConstRefExpr* templateConstRefExpr) {
    // TODO: Should we error here? Or return the const value? Or do nothing until a const solving stage?
}

void gulc::CodeProcessor::processTernaryExpr(gulc::TernaryExpr* ternaryExpr) {
    // TODO: We will need to check if `condition` returns a bool or not
    processExpr(ternaryExpr->condition);
    processExpr(ternaryExpr->trueExpr);
    processExpr(ternaryExpr->falseExpr);

    TypeCompareUtil typeCompareUtil;

    // TODO: We need to handle when one side is `ref` and the other isn't. Also casting?
    if (!typeCompareUtil.compareAreSame(ternaryExpr->trueExpr->valueType, ternaryExpr->falseExpr->valueType)) {
        printError("both sides of a ternary operator must be the same type!",
                   ternaryExpr->startPosition(), ternaryExpr->endPosition());
    }

    // If one side or the other isn't an lvalue then we need to convert the lvalue to an rvalue
    if (!ternaryExpr->trueExpr->valueType->isLValue()) {
        ternaryExpr->falseExpr = convertLValueToRValue(ternaryExpr->falseExpr);
    }

    if (!ternaryExpr->falseExpr->valueType->isLValue()) {
        ternaryExpr->trueExpr = convertLValueToRValue(ternaryExpr->trueExpr);
    }

    ternaryExpr->valueType = ternaryExpr->trueExpr->valueType->deepCopy();
}

void gulc::CodeProcessor::processTryExpr(gulc::TryExpr* tryExpr) {
    processExpr(tryExpr->nestedExpr);

    tryExpr->valueType = tryExpr->nestedExpr->valueType->deepCopy();
}

void gulc::CodeProcessor::processTypeExpr(gulc::TypeExpr* typeExpr) {

}

void gulc::CodeProcessor::processValueLiteralExpr(gulc::ValueLiteralExpr* valueLiteralExpr) {
    switch (valueLiteralExpr->literalType()) {
        case ValueLiteralExpr::LiteralType::Integer:
            if (!valueLiteralExpr->hasSuffix()) {
                valueLiteralExpr->valueType = BuiltInType::get(Type::Qualifier::Unassigned, "i32", {}, {});
            } else {
                printError("custom type suffixes not yet supported!",
                           valueLiteralExpr->startPosition(), valueLiteralExpr->endPosition());
            }
            break;
        case ValueLiteralExpr::LiteralType::Float:
            if (!valueLiteralExpr->hasSuffix()) {
                valueLiteralExpr->valueType = BuiltInType::get(Type::Qualifier::Unassigned, "f32", {}, {});
            } else {
                printError("custom type suffixes not yet supported!",
                           valueLiteralExpr->startPosition(), valueLiteralExpr->endPosition());
            }
            break;
        case ValueLiteralExpr::LiteralType::Char:
            if (!valueLiteralExpr->hasSuffix()) {
                // TODO: Should we have a special `char` type?
                valueLiteralExpr->valueType = BuiltInType::get(Type::Qualifier::Unassigned, "u8", {}, {});
            } else {
                printError("custom type suffixes not yet supported!",
                           valueLiteralExpr->startPosition(), valueLiteralExpr->endPosition());
            }
            break;
        case ValueLiteralExpr::LiteralType::String:
            printError("string literals not yet supported!",
                       valueLiteralExpr->startPosition(), valueLiteralExpr->endPosition());
            break;
        default:
            printError("[INTERNAL] unknown literal type found in `CodeProcessor::processValueLiteralExpr`!",
                       valueLiteralExpr->startPosition(), valueLiteralExpr->endPosition());
            break;
    }
}

void gulc::CodeProcessor::processVariableDeclExpr(gulc::VariableDeclExpr* variableDeclExpr) {
    if (variableDeclExpr->initialValue != nullptr) {
        processExpr(variableDeclExpr->initialValue);

        Type* checkType = variableDeclExpr->type;

        if (checkType == nullptr) {
            checkType = variableDeclExpr->initialValue->valueType;
        }

        // If the variable is a struct type we have to handle initial value differently. Initial value assignments MUST
        // be done through a struct assignment, meaning we either `copy` or `move`
        if (llvm::isa<StructType>(checkType)) {
            // TODO: Default struct assignment type should be `move` when possible.
            variableDeclExpr->initialValueAssignmentType = InitialValueAssignmentType::Copy;
        } else {
            // We convert lvalue to rvalue but we DO NOT remove `ref` UNLESS `variableDeclExpr->type` isn't `ref`
            // TODO: Handle implicit references
            variableDeclExpr->initialValue = convertLValueToRValue(variableDeclExpr->initialValue);
        }

        if (variableDeclExpr->type == nullptr) {
            // If the type wasn't explicitly set then we infer the type from the value type of the initial value
            variableDeclExpr->type = variableDeclExpr->initialValue->valueType->deepCopy();
        }
    }

    // TODO: Should we support local variable shadowing like Rust (and partially like Swift?)
    //       I really like the idea, especially with immut-by-default. Just need emit warnings in scenarios where the
    //       shadowing looks unintentional (as ambiguous as that is)...
    for (VariableDeclExpr* checkVariable : _localVariables) {
        if (checkVariable->identifier().name() == variableDeclExpr->identifier().name()) {
            printError("local variable `" + variableDeclExpr->identifier().name() + "` redefined!",
                       variableDeclExpr->startPosition(), variableDeclExpr->endPosition());
        }
    }

    _localVariables.push_back(variableDeclExpr);
}

void gulc::CodeProcessor::processVariableRefExpr(gulc::VariableRefExpr* variableRefExpr) {
    variableRefExpr->valueType = variableRefExpr->variableDecl->type->deepCopy();
    variableRefExpr->valueType->setIsLValue(true);
}

gulc::Expr* gulc::CodeProcessor::convertLValueToRValue(gulc::Expr* potentialLValue) const {
    if (potentialLValue->valueType->isLValue()) {
        auto result = new LValueToRValueExpr(potentialLValue);
        result->valueType = potentialLValue->valueType->deepCopy();
        result->valueType->setIsLValue(false);
        return result;
    }

    return potentialLValue;
}

void gulc::CodeProcessor::handleArgumentCasting(std::vector<ParameterDecl*> const& parameters,
                                                std::vector<LabeledArgumentExpr*>& arguments) {
    for (std::size_t i = 0; i < parameters.size(); ++i) {
        if (i >= arguments.size()) {
            // TODO: Append the default value to arguments
        } else {
            // TODO: Remember that `ref mut` MUST be a `ref mut`, no implicit conversion from `lvalue` to `ref mut`
            if (llvm::isa<ReferenceType>(parameters[i]->type)) {
                if (!llvm::isa<ReferenceType>(arguments[i]->argument->valueType)) {
                    // TODO: If it is an `lvalue` convert the `lvalue` to a real `ref` if possible
                } else if (arguments[i]->argument->valueType->isLValue()) {
                    // If the argument is an lvalue to a reference then we convert the lvalue to an rvalue, keeping the
                    // implicit reference.
                    arguments[i]->argument = convertLValueToRValue(arguments[i]->argument);
                }
            } else {
                // The parameter is a value type so we remove any `lvalue` and dereference any implicit references.
                arguments[i]->argument = convertLValueToRValue(arguments[i]->argument);
                // TODO: Dereference any references as well.
            }
        }
    }
}

gulc::Expr* gulc::CodeProcessor::dereferenceReference(gulc::Expr* potentialReference) const {
    // All lvalues must be converted to rvalues before dereferencing
    if (potentialReference->valueType->isLValue()) {
        printError("[INTERNAL] `CodeProcessor::dereferenceReference` cannot be called with an lvalue!",
                   potentialReference->startPosition(), potentialReference->endPosition());
    }

    if (llvm::isa<ReferenceType>(potentialReference->valueType)) {
        auto derefType = llvm::dyn_cast<ReferenceType>(potentialReference->valueType)->nestedType->deepCopy();

        auto derefExpr = new ImplicitDerefExpr(potentialReference);
        derefExpr->valueType = derefType;

        return derefExpr;
    } else {
        return potentialReference;
    }
}
