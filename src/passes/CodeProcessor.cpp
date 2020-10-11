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
#include <ast/exprs/PropertyGetCallExpr.hpp>
#include <ast/exprs/PropertySetCallExpr.hpp>
#include <ast/exprs/SubscriptOperatorRefExpr.hpp>
#include <ast/exprs/MemberSubscriptOperatorRefExpr.hpp>
#include <ast/exprs/SubscriptOperatorGetCallExpr.hpp>
#include <ast/exprs/SubscriptOperatorSetCallExpr.hpp>
#include <ast/exprs/RValueToInRefExpr.hpp>
#include <ast/exprs/MemberInfixOperatorCallExpr.hpp>

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

void gulc::CodeProcessor::processPrototypeDecl(gulc::Decl* decl) {
    switch (decl->getDeclKind()) {
        case Decl::Kind::TraitPrototype:
            processTraitPrototypeDecl(llvm::dyn_cast<TraitPrototypeDecl>(decl));
            break;
        default:
            processDecl(decl);
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
    Decl* oldContainer = _currentContainer;
    _currentContainer = enumDecl;

    for (Decl* member : enumDecl->ownedMembers()) {
        processDecl(member);
    }

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

void gulc::CodeProcessor::processTraitPrototypeDecl(gulc::TraitPrototypeDecl* traitPrototypeDecl) {
    // TODO: Is there anything to do here?
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
        std::vector<MatchingFunctorDecl> foundMatches;

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
                for (MatchingFunctorDecl& checkMatch : foundMatches) {
                    if (checkMatch.kind == MatchingDecl::Kind::Match) {
                        if (foundConstructor == nullptr) {
                            foundConstructor = llvm::dyn_cast<ConstructorDecl>(checkMatch.functorDecl);
                        } else {
                            printError("base constructor call is ambiguous!",
                                       functionCallExpr->startPosition(), functionCallExpr->endPosition());
                            return;
                        }
                    }
                }
            } else if (!foundMatches.empty()) {
                foundConstructor = llvm::dyn_cast<ConstructorDecl>(foundMatches[0].functorDecl);
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
        case Stmt::Kind::RepeatWhile:
            processRepeatWhileStmt(llvm::dyn_cast<RepeatWhileStmt>(stmt));
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

        caseStmt->condition = handleGetter(caseStmt->condition);
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

void gulc::CodeProcessor::processForStmt(gulc::ForStmt* forStmt) {
    std::size_t oldLocalVariableCount = _localVariables.size();

    if (forStmt->init != nullptr) {
        processExpr(forStmt->init);
    }

    if (forStmt->condition != nullptr) {
        processExpr(forStmt->condition);

        forStmt->condition = handleGetter(forStmt->condition);
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

    ifStmt->condition = handleGetter(ifStmt->condition);
    ifStmt->condition = convertLValueToRValue(ifStmt->condition);
    ifStmt->condition = dereferenceReference(ifStmt->condition);
}

void gulc::CodeProcessor::processLabeledStmt(gulc::LabeledStmt* labeledStmt) {
    labeledStmt->currentNumLocalVariables = _localVariables.size();

    _currentFunction->labeledStmts.insert({labeledStmt->label().name(), labeledStmt});

    labelResolved(labeledStmt->label().name());

    processStmt(labeledStmt->labeledStmt);
}

void gulc::CodeProcessor::processRepeatWhileStmt(gulc::RepeatWhileStmt* repeatWhileStmt) {
    processCompoundStmt(repeatWhileStmt->body());
    processExpr(repeatWhileStmt->condition);

    repeatWhileStmt->condition = handleGetter(repeatWhileStmt->condition);
    repeatWhileStmt->condition = convertLValueToRValue(repeatWhileStmt->condition);
    repeatWhileStmt->condition = dereferenceReference(repeatWhileStmt->condition);
}

void gulc::CodeProcessor::processReturnStmt(gulc::ReturnStmt* returnStmt) {
    if (returnStmt->returnValue != nullptr) {
        processExpr(returnStmt->returnValue);

        // NOTE: We don't implicitly convert from `lvalue` to reference here if the function returns a reference.
        //       To return a reference you MUST specify `ref {value}`, we don't do implicitly referencing like in C++.
        returnStmt->returnValue = handleGetter(returnStmt->returnValue);
        returnStmt->returnValue = convertLValueToRValue(returnStmt->returnValue);

        if (!llvm::isa<ReferenceType>(_currentFunction->returnType)) {
            returnStmt->returnValue = dereferenceReference(returnStmt->returnValue);
        }
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
            processAssignmentOperatorExpr(expr);
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
        case Expr::Kind::InfixOperator: {
            auto infixOperatorExpr = llvm::dyn_cast<InfixOperatorExpr>(expr);
            processInfixOperatorExpr(infixOperatorExpr);
            expr = infixOperatorExpr;
            break;
        }
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

void gulc::CodeProcessor::processAssignmentOperatorExpr(gulc::Expr*& expr) {
    auto assignmentOperatorExpr = llvm::dyn_cast<gulc::AssignmentOperatorExpr>(expr);

    processExpr(assignmentOperatorExpr->leftValue);
    processExpr(assignmentOperatorExpr->rightValue);

    assignmentOperatorExpr->rightValue = handleGetter(assignmentOperatorExpr->rightValue);

    // `MemberPropertyRefExpr` and `PropertyRefExpr` are special cases. We convert `AssignmentOperatorExpr` to
    // `PropertySetCallExpr`
    // Ditto for `MemberSubscriptOperatorRefExpr` and `SubscriptOperatorRefExpr` -> `SubscroptOperatorSetCallExpr`
    if (llvm::isa<PropertyRefExpr>(assignmentOperatorExpr->leftValue)) {
        auto propertyRef = llvm::dyn_cast<PropertyRefExpr>(assignmentOperatorExpr->leftValue);

        if (assignmentOperatorExpr->hasNestedOperator()) {
            // TODO: We will need to create both a getter and a setter expression...
            printError("compound assignment operators not yet supported for `prop`!",
                       assignmentOperatorExpr->startPosition(), assignmentOperatorExpr->endPosition());
        } else {
            assignmentOperatorExpr->rightValue = convertLValueToRValue(assignmentOperatorExpr->rightValue);
            assignmentOperatorExpr->rightValue = dereferenceReference(assignmentOperatorExpr->rightValue);
        }

        // TODO: If `rightValue->valueType` isn't the same as `checkType` search for a valid cast

        if (!propertyRef->propertyDecl->hasSetter()) {
            printError("property `" + propertyRef->toString() + "` does not have a valid setter!",
                       propertyRef->startPosition(), propertyRef->endPosition());
        }

        // We have to validate that the object is mutable for the setter to be allowed
        if (llvm::isa<MemberPropertyRefExpr>(propertyRef)) {
            auto memberPropertyRef = llvm::dyn_cast<MemberPropertyRefExpr>(propertyRef);

            if (memberPropertyRef->object->valueType->qualifier() != Type::Qualifier::Mut) {
                printError("property setter cannot be called on an immutable object!",
                           memberPropertyRef->object->startPosition(), memberPropertyRef->object->endPosition());
            }
        }

        auto setterCall = new PropertySetCallExpr(
                propertyRef,
                propertyRef->propertyDecl->setter(),
                assignmentOperatorExpr->rightValue
        );
        // TODO: Does a setter return anything?
        setterCall->valueType = setterCall->propertySetter->returnType->deepCopy();
        setterCall->valueType->setIsLValue(false);

        // We steal these before deleting...
        assignmentOperatorExpr->leftValue = nullptr;
        assignmentOperatorExpr->rightValue = nullptr;
        delete assignmentOperatorExpr;

        expr = setterCall;
        return;
    } else if (llvm::isa<SubscriptOperatorRefExpr>(assignmentOperatorExpr->leftValue)) {
        auto subscriptOperatorRef = llvm::dyn_cast<SubscriptOperatorRefExpr>(assignmentOperatorExpr->leftValue);

        if (assignmentOperatorExpr->hasNestedOperator()) {
            // TODO: We will need to create both a getter and a setter expression...
            printError("compound assignment operators not yet supported for `prop`!",
                       assignmentOperatorExpr->startPosition(), assignmentOperatorExpr->endPosition());
        } else {
            assignmentOperatorExpr->rightValue = convertLValueToRValue(assignmentOperatorExpr->rightValue);
            assignmentOperatorExpr->rightValue = dereferenceReference(assignmentOperatorExpr->rightValue);
        }

        // TODO: If `rightValue->valueType` isn't the same as `checkType` search for a valid cast

        if (!subscriptOperatorRef->subscriptOperatorDecl->hasSetter()) {
            printError("subscript `" + subscriptOperatorRef->toString() + "` does not have a valid setter!",
                       subscriptOperatorRef->startPosition(), subscriptOperatorRef->endPosition());
        }

        // We have to validate that the object is mutable for the setter to be allowed
        if (llvm::isa<MemberSubscriptOperatorRefExpr>(subscriptOperatorRef)) {
            auto memberSubscriptOperatorRef = llvm::dyn_cast<MemberSubscriptOperatorRefExpr>(subscriptOperatorRef);

            if (memberSubscriptOperatorRef->object->valueType->qualifier() != Type::Qualifier::Mut) {
                printError("subscript setter cannot be called on an immutable object!",
                           memberSubscriptOperatorRef->object->startPosition(),
                           memberSubscriptOperatorRef->object->endPosition());
            }
        }

        auto setterCall = new SubscriptOperatorSetCallExpr(
                subscriptOperatorRef,
                subscriptOperatorRef->subscriptOperatorDecl->setter(),
                assignmentOperatorExpr->rightValue
        );
        // TODO: Does a setter return anything?
        setterCall->valueType = setterCall->subscriptOperatorSetter->returnType->deepCopy();
        setterCall->valueType->setIsLValue(false);

        // We steal these before deleting...
        assignmentOperatorExpr->leftValue = nullptr;
        assignmentOperatorExpr->rightValue = nullptr;
        delete assignmentOperatorExpr;

        expr = setterCall;
        return;
    } else {
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
                printError("compound assignment operators not yet supported!",
                           assignmentOperatorExpr->startPosition(), assignmentOperatorExpr->endPosition());
            }

            // TODO: If `rightValue->valueType` isn't the same as `checkType` search for a valid cast

            assignmentOperatorExpr->rightValue = handleGetter(assignmentOperatorExpr->rightValue);

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
                assignmentOperatorExpr->rightValue = dereferenceReference(assignmentOperatorExpr->rightValue);
            }

            // TODO: Validate that `rightType` can be casted to `leftType` (if they aren't the same type)

            // TODO: If `leftValue` is both an `lvalue` and a `ref` what should we do?
            //       Convert lvalue to rvalue or dereference the ref?
            if (assignmentOperatorExpr->leftValue->valueType->isLValue() &&
                    llvm::isa<ReferenceType>(assignmentOperatorExpr->leftValue->valueType)) {
                assignmentOperatorExpr->leftValue = convertLValueToRValue(assignmentOperatorExpr->leftValue);
            }

            assignmentOperatorExpr->valueType = assignmentOperatorExpr->leftValue->valueType->deepCopy();
            // TODO: Is this right? I believe we'll end up returning the actual alloca from CodeGen so this seems right
            //       to me.
            assignmentOperatorExpr->valueType->setIsLValue(true);
        }
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

    MatchingFunctorDecl foundDecl(MatchingDecl::Kind::Unknown, nullptr, nullptr);

    // NOTES: When we search for a template function call there is a lot that needs to be accounted for such as:
    //         1. Template matching
    //         2. Template specialization strength
    //         3. Normal argument and parameter matching
    //        That is the order for what is accounted for. I.e.
    //            Template Parameters > Template Specialization > Normal Parameters
    //        So for the function call `Example<i32>(12)`
    //            `func Example<T: i32>(_ param1: i32, _ param2: i32 = 4)`
    //        The above will be chosen before the below:
    //            `func Example<T: i32, const v: usize = 0>(_ param: i32)`
    //        Because the _template parameters_ are an exact match.
    //        The specialization and handling of templates can become very confusing the deeper and more advanced you
    //        go, be careful.
    if (llvm::isa<IdentifierExpr>(functionCallExpr->functionReference)) {
        auto identifierExpr = llvm::dyn_cast<IdentifierExpr>(functionCallExpr->functionReference);
        std::string const& findName = identifierExpr->identifier().name();

        // If the function being called is a template then we first gather a list of templates that match the template
        // arguments. Then we search that list for matching functors (e.g. constructors, functions, etc.)
        if (identifierExpr->hasTemplateArguments()) {
            for (Expr*& templateArgument : identifierExpr->templateArguments()) {
                processExpr(templateArgument);
            }

            // We use a "2d" vector as a way of prioritizing results. A lower index is higher priority but if we only
            // find `Castable` at index `0` but find `Exact` at index `12` then we choose the `Exact`.
            std::vector<std::vector<MatchingTemplateDecl>> templateMatches;

            // Check `currentContainer` for templates ONLY
            if (_currentContainer != nullptr) {
                templateMatches.push_back({});

                bool findStaticOnly =
                        llvm::isa<EnumDecl>(_currentContainer) || llvm::isa<StructDecl>(_currentContainer) ||
                                llvm::isa<TraitDecl>(_currentContainer);

                fillListOfMatchingTemplatesInContainer(_currentContainer, findName, findStaticOnly,
                                                       identifierExpr->templateArguments(),
                                                       templateMatches[templateMatches.size() - 1]);
            }

            // TODO: If the `_currentContainer` isn't a namespace should we also check the current namespace?

            // Check `currentFile` for templates ONLY
            if (_currentFile != nullptr) {
                templateMatches.push_back({});

                fillListOfMatchingTemplates(_currentFile->declarations, findName, false,
                                            identifierExpr->templateArguments(),
                                            templateMatches[templateMatches.size() - 1]);
            }

            // Search imports. All imports have the same depth.
            if (!_currentFile->imports.empty()) {
                templateMatches.push_back({});

                for (ImportDecl* checkImport : _currentFile->imports) {
                    fillListOfMatchingTemplates(checkImport->pointToNamespace->nestedDecls(), findName,
                                                false, identifierExpr->templateArguments(),
                                                templateMatches[templateMatches.size() - 1]);
                }
            }

            bool isAmbiguous = false;
            MatchingTemplateDecl* foundTemplate = findMatchingTemplateDecl(templateMatches,
                                                                           identifierExpr->templateArguments(),
                                                                           functionCallExpr->arguments,
                                                                           functionCallExpr->startPosition(),
                                                                           functionCallExpr->endPosition(),
                                                                           &isAmbiguous);

            if (foundTemplate == nullptr) {
                printError("template `" + identifierExpr->toString() + "` was not found!",
                           identifierExpr->startPosition(), identifierExpr->endPosition());
            }

            if (isAmbiguous) {
                printError("template `" + identifierExpr->toString() + "` is ambiguous!",
                           identifierExpr->startPosition(), identifierExpr->endPosition());
            }

            foundDecl = getTemplateFunctorInstantiation(foundTemplate, identifierExpr->templateArguments(),
                                                        functionCallExpr->arguments, identifierExpr->toString(),
                                                        identifierExpr->startPosition(), identifierExpr->endPosition());
        } else {
            // If we find a matching local variable we immediately end the search. You can't do overloading with local
            // variables.
            {
                std::vector<MatchingFunctorDecl> matchingFunctors;
                VariableDeclExpr* foundLocalVariable = nullptr;

                for (VariableDeclExpr* checkLocalVariable : _localVariables) {
                    if (findName == checkLocalVariable->identifier().name()) {
                        Type* checkType = checkLocalVariable->type;

                        // TODO: Account for smart references
                        if (llvm::isa<ReferenceType>(checkType)) {
                            checkType = llvm::dyn_cast<ReferenceType>(checkType)->nestedType;
                        }

                        fillListOfMatchingFunctorsInType(checkLocalVariable->type, nullptr,
                                                         functionCallExpr->arguments, matchingFunctors);
                        foundLocalVariable = checkLocalVariable;
                        break;
                    }
                }

                if (foundLocalVariable != nullptr) {
                    if (matchingFunctors.empty()) {
                        printError("local variable `" + foundLocalVariable->identifier().name() + "` is not callable "
                                   "with the provided arguments!",
                                   functionCallExpr->startPosition(), functionCallExpr->endPosition());
                    }

                    Decl* varFoundDecl = nullptr;

                    if (matchingFunctors.size() == 1) {
                        varFoundDecl = matchingFunctors[0].functorDecl;
                    } else {
                        MatchingDecl::Kind foundKind = MatchingDecl::Kind::Unknown;
                        bool isAmbiguous = false;

                        for (MatchingFunctorDecl& checkFunctor : matchingFunctors) {
                            if (varFoundDecl == nullptr) {
                                varFoundDecl = checkFunctor.functorDecl;
                                foundKind = checkFunctor.kind;
                                isAmbiguous = false;
                            } else {
                                if (foundKind == checkFunctor.kind) {
                                    isAmbiguous = true;
                                } else if (foundKind == MatchingDecl::Kind::Match) {
                                    // Nothing to do. if `check` is also `Match` we will handle it above. Else ignore.
                                } else if (foundKind == MatchingDecl::Kind::DefaultValues) {
                                    // `Match` is better than `DefaultValues`
                                    if (checkFunctor.kind == MatchingDecl::Kind::Match) {
                                        varFoundDecl = checkFunctor.functorDecl;
                                        foundKind = checkFunctor.kind;
                                        isAmbiguous = false;
                                    }
                                } else {
                                    // At this point `check` is either `Match` or `DefaultValues` while `found` is
                                    // `Castable`, so we replace the castable.
                                    varFoundDecl = checkFunctor.functorDecl;
                                    foundKind = checkFunctor.kind;
                                    isAmbiguous = false;
                                }
                            }
                        }
                    }

                    if (varFoundDecl == nullptr) {
                        auto checkType = foundLocalVariable->type;

                        // TODO: Account for smart references
                        if (llvm::isa<ReferenceType>(foundLocalVariable->type)) {
                            checkType = llvm::dyn_cast<ReferenceType>(foundLocalVariable->type)->nestedType;
                        }

                        if (!llvm::isa<FunctionPointerType>(checkType)) {
                            printError("local variable `" + identifierExpr->toString() + "` is not callable!",
                                       functionCallExpr->startPosition(), functionCallExpr->endPosition());
                        }

                        auto funcPointerType = llvm::dyn_cast<FunctionPointerType>(checkType);

                        // TODO: Create a `FunctionPointerCallExpr` or something.
                        printError("calling function pointers not yet supported!",
                                   functionCallExpr->startPosition(), functionCallExpr->endPosition());
                    } else if (llvm::isa<CallOperatorDecl>(varFoundDecl)) {
                        // TODO: Create a new `MemberFunctionCallExpr` with `CallOperatorReferenceExpr` as the function
                        //       reference and `RefLocalVariableExpr` as the `self`
                        printError("using `call` operators not yet supported!",
                                   functionCallExpr->startPosition(), functionCallExpr->endPosition());
                    } else {
                        printError("[INTERNAL] unknown functor type for local variable!",
                                   functionCallExpr->startPosition(), functionCallExpr->endPosition());
                    }
                }
            }

            // If we find a matching parameter we do the same as we do with local variables, end as soon as we find it.
            if (_currentParameters != nullptr) {
                std::vector<MatchingFunctorDecl> matchingFunctors;
                ParameterDecl* foundParameter = nullptr;

                for (ParameterDecl* checkParameter : *_currentParameters) {
                    if (findName == checkParameter->identifier().name()) {
                        Type* checkType = checkParameter->type;

                        // TODO: Account for smart references
                        if (llvm::isa<ReferenceType>(checkType)) {
                            checkType = llvm::dyn_cast<ReferenceType>(checkType)->nestedType;
                        }

                        fillListOfMatchingFunctorsInType(checkParameter->type, nullptr,
                                                         functionCallExpr->arguments, matchingFunctors);
                        foundParameter = checkParameter;
                        break;
                    }
                }

                if (foundParameter != nullptr) {
                    if (matchingFunctors.empty()) {
                        printError("parameter `" + foundParameter->identifier().name() + "` is not callable "
                                   "with the provided arguments!",
                                   functionCallExpr->startPosition(), functionCallExpr->endPosition());
                    }

                    Decl* paramFoundDecl = nullptr;

                    if (matchingFunctors.size() == 1) {
                        paramFoundDecl = matchingFunctors[0].functorDecl;
                    } else {
                        MatchingDecl::Kind foundKind = MatchingDecl::Kind::Unknown;
                        bool isAmbiguous = false;

                        for (MatchingFunctorDecl& checkFunctor : matchingFunctors) {
                            if (paramFoundDecl == nullptr) {
                                paramFoundDecl = checkFunctor.functorDecl;
                                foundKind = checkFunctor.kind;
                                isAmbiguous = false;
                            } else {
                                if (foundKind == checkFunctor.kind) {
                                    isAmbiguous = true;
                                } else if (foundKind == MatchingDecl::Kind::Match) {
                                    // Nothing to do. if `check` is also `Match` we will handle it above. Else ignore.
                                } else if (foundKind == MatchingDecl::Kind::DefaultValues) {
                                    // `Match` is better than `DefaultValues`
                                    if (checkFunctor.kind == MatchingDecl::Kind::Match) {
                                        paramFoundDecl = checkFunctor.functorDecl;
                                        foundKind = checkFunctor.kind;
                                        isAmbiguous = false;
                                    }
                                } else {
                                    // At this point `check` is either `Match` or `DefaultValues` while `found` is
                                    // `Castable`, so we replace the castable.
                                    paramFoundDecl = checkFunctor.functorDecl;
                                    foundKind = checkFunctor.kind;
                                    isAmbiguous = false;
                                }
                            }
                        }
                    }

                    if (paramFoundDecl == nullptr) {
                        auto checkType = foundParameter->type;

                        // TODO: Account for smart references
                        if (llvm::isa<ReferenceType>(foundParameter->type)) {
                            checkType = llvm::dyn_cast<ReferenceType>(foundParameter->type)->nestedType;
                        }

                        if (!llvm::isa<FunctionPointerType>(checkType)) {
                            printError("parameter `" + identifierExpr->toString() + "` is not callable!",
                                       functionCallExpr->startPosition(), functionCallExpr->endPosition());
                        }

                        auto funcPointerType = llvm::dyn_cast<FunctionPointerType>(checkType);

                        // TODO: Create a `FunctionPointerCallExpr` or something.
                        printError("calling function pointers not yet supported!",
                                   functionCallExpr->startPosition(), functionCallExpr->endPosition());
                    } else if (llvm::isa<CallOperatorDecl>(paramFoundDecl)) {
                        // TODO: Create a new `MemberFunctionCallExpr` with `CallOperatorReferenceExpr` as the function
                        //       reference and `RefLocalVariableExpr` as the `self`
                        printError("using `call` operators not yet supported!",
                                   functionCallExpr->startPosition(), functionCallExpr->endPosition());
                    } else {
                        printError("[INTERNAL] unknown functor type for local variable!",
                                   functionCallExpr->startPosition(), functionCallExpr->endPosition());
                    }
                }
            }

            // NOTE: We only fill this if `foundDecl` is null, if `foundDecl` isn't null then we don't continue
            //       searching.
            std::vector<std::vector<MatchingFunctorDecl>> layeredMatchingFunctors;

            // When searching containers, the file, and imports we end as soon as we find an exact match. If not we
            // keep searching while keeping a `depth` stored.
            if (foundDecl.functorDecl == nullptr && _currentContainer != nullptr) {
                layeredMatchingFunctors.push_back({});

                fillListOfMatchingFunctorsInContainer(_currentContainer, findName, false, functionCallExpr->arguments,
                                                      layeredMatchingFunctors[layeredMatchingFunctors.size() - 1]);

                // Check for an exact match and error on unrecoverable ambiguities (i.e. two exact matches is
                // unrecoverable at the same depths)
                bool checkIsAmbiguous = false;
                MatchingFunctorDecl* checkExactMatch = findMatchingFunctorDecl(
                        layeredMatchingFunctors[layeredMatchingFunctors.size() - 1], true, &checkIsAmbiguous);

                if (checkExactMatch != nullptr) {
                    if (checkIsAmbiguous) {
                        printError("function call `" + functionCallExpr->toString() + "` is ambiguous!",
                                   functionCallExpr->startPosition(), functionCallExpr->endPosition());
                    }

                    foundDecl = *checkExactMatch;
                }
            }

            // TODO: Search the `_currentNamespace` if it exists.

            if (foundDecl.functorDecl == nullptr && _currentFile != nullptr) {
                layeredMatchingFunctors.push_back({});

                fillListOfMatchingFunctors(_currentFile->declarations, findName, false, functionCallExpr->arguments,
                                           layeredMatchingFunctors[layeredMatchingFunctors.size() - 1]);

                // Check for an exact match and error on unrecoverable ambiguities (i.e. two exact matches is
                // unrecoverable at the same depths)
                bool checkIsAmbiguous = false;
                MatchingFunctorDecl* checkExactMatch = findMatchingFunctorDecl(
                        layeredMatchingFunctors[layeredMatchingFunctors.size() - 1], true, &checkIsAmbiguous);

                if (checkExactMatch != nullptr) {
                    if (checkIsAmbiguous) {
                        printError("function call `" + functionCallExpr->toString() + "` is ambiguous!",
                                   functionCallExpr->startPosition(), functionCallExpr->endPosition());
                    }

                    foundDecl = *checkExactMatch;
                }
            }

            // Handle imported namespaces if needed
            if (foundDecl.functorDecl == nullptr && _currentFile != nullptr && !_currentFile->imports.empty()) {
                layeredMatchingFunctors.push_back({});

                for (ImportDecl* checkImport : _currentFile->imports) {
                    fillListOfMatchingFunctors(checkImport->pointToNamespace->nestedDecls(), findName,
                                               false, functionCallExpr->arguments,
                                               layeredMatchingFunctors[layeredMatchingFunctors.size() - 1]);
                }
            }

            if (foundDecl.functorDecl == nullptr) {
                std::size_t foundDepth = std::numeric_limits<std::size_t>::max();
                bool isAmbiguous = false;

                for (std::size_t checkDepth = 0; checkDepth < layeredMatchingFunctors.size(); ++checkDepth) {
                    for (MatchingFunctorDecl& checkMatch : layeredMatchingFunctors[checkDepth]) {
                        if (foundDecl.functorDecl == nullptr) {
                            foundDecl = checkMatch;
                            foundDepth = checkDepth;
                        } else {
                            if (foundDecl.kind == checkMatch.kind) {
                                // NOTE: `isAmbiguous` can only be true if the depths are also the same
                                if (foundDepth == checkDepth) {
                                    isAmbiguous = true;
                                }
                            } else if (foundDecl.kind == MatchingDecl::Kind::Match) {
                                // Do nothing. Our only case that needs handled is above.
                            } else if (foundDecl.kind == MatchingDecl::Kind::DefaultValues) {
                                if (checkMatch.kind == MatchingDecl::Kind::Match) {
                                    // Regardless of depth `Match` is better than `DefaultValues`
                                    foundDecl = checkMatch;
                                    foundDepth = checkDepth;
                                    isAmbiguous = false;
                                }
                            } else {
                                // In this scenario `found` is `Castable` and `check` is either `Match` or
                                // `DefaultValues`, both of which are better than `Castable` regardless of depth
                                foundDecl = checkMatch;
                                foundDepth = checkDepth;
                                isAmbiguous = false;
                            }
                        }
                    }

                    // If we found an exact match at the end of a depth search then we immediately break, duplicate
                    // exact matches at different depths are not ambiguous
                    if (foundDecl.kind == MatchingDecl::Kind::Match) {
                        break;
                    }
                }

                if (foundDecl.functorDecl == nullptr) {
                    printError("function `" + identifierExpr->toString() + "` was not found!",
                               functionCallExpr->startPosition(), functionCallExpr->endPosition());
                }

                if (isAmbiguous) {
                    printError("function call `" + functionCallExpr->toString() + "` is ambiguous!",
                               functionCallExpr->startPosition(), functionCallExpr->endPosition());
                }
            }
        }

        if (foundDecl.functorDecl == nullptr) {
            printError("function `" + identifierExpr->toString() + "` was not found!",
                       functionCallExpr->startPosition(), functionCallExpr->endPosition());
        }

        switch (foundDecl.functorDecl->getDeclKind()) {
            case Decl::Kind::CallOperator: {
                auto callOperatorDecl = llvm::dyn_cast<CallOperatorDecl>(foundDecl.functorDecl);

                // We handle argument casting and conversion no matter what. The below function will handle
                // converting from lvalue to rvalue, casting, and other rules for us.
                handleArgumentCasting(callOperatorDecl->parameters(), functionCallExpr->arguments);

                Expr* selfRef = nullptr;

                // Context is either a `VariableDecl` or a `PropertyDecl` here. It cannot be anything else.
                if (llvm::isa<VariableDecl>(foundDecl.contextDecl)) {
                    auto contextVariableDecl = llvm::dyn_cast<VariableDecl>(foundDecl.contextDecl);
                    Expr* refVariableDecl = new VariableRefExpr(
                            identifierExpr->startPosition(),
                            identifierExpr->endPosition(),
                            contextVariableDecl
                    );
                    refVariableDecl->valueType = contextVariableDecl->type->deepCopy();
                    refVariableDecl->valueType->setIsLValue(true);

                    // If it is a reference we must dereference it for the call.
                    // TODO: Support smart references
                    refVariableDecl = dereferenceReference(refVariableDecl);

                    // The `self` for the call is the `VariableDecl`
                    selfRef = refVariableDecl;
                } else if (llvm::isa<PropertyDecl>(foundDecl.contextDecl)) {
                    auto contextPropertyDecl = llvm::dyn_cast<PropertyDecl>(foundDecl.contextDecl);
                    Expr* refPropertyDecl = new PropertyRefExpr(
                            identifierExpr->startPosition(),
                            identifierExpr->endPosition(),
                            contextPropertyDecl
                    );

                    // We can't use a `PropertyRefExpr` by itself. It needs to be converted into the `get` call
                    refPropertyDecl = handleGetter(refPropertyDecl);
                    // If it is a reference we must dereference it for the call.
                    // TODO: Support smart references
                    refPropertyDecl = dereferenceReference(refPropertyDecl);

                    // The `self` for the call is the `PropertyDecl`
                    selfRef = refPropertyDecl;
                } else {
                    printError("[INTERNAL] unknown context declaration use for `CallOperatorDecl`!",
                               functionCallExpr->startPosition(), functionCallExpr->endPosition());
                }

                auto callOperatorReference = new CallOperatorReferenceExpr(
                        functionCallExpr->startPosition(),
                        functionCallExpr->endPosition(),
                        callOperatorDecl
                );
                processCallOperatorReferenceExpr(callOperatorReference);

                auto newExpr = new MemberFunctionCallExpr(
                        callOperatorReference,
                        selfRef,
                        functionCallExpr->arguments,
                        functionCallExpr->startPosition(),
                        functionCallExpr->endPosition()
                );

                // And we steal the arguments
                functionCallExpr->arguments.clear();
                // Delete the old function call and replace it with the new one
                delete functionCallExpr;
                functionCallExpr = newExpr;
                functionCallExpr->valueType = callOperatorDecl->returnType->deepCopy();
                functionCallExpr->valueType->setIsLValue(true);
                break;
            }
            case Decl::Kind::Constructor: {
                auto foundConstructor = llvm::dyn_cast<ConstructorDecl>(foundDecl.functorDecl);

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

                switch (foundDecl.functorDecl->container->getDeclKind()) {
                    case Decl::Kind::TemplateStructInst:
                    case Decl::Kind::Struct:
                        functionCallExpr->valueType = new StructType(Type::Qualifier::Mut,
                                                                     llvm::dyn_cast<StructDecl>(foundDecl.functorDecl->container),
                                                                     {}, {});
                        functionCallExpr->valueType->setIsLValue(true);
                        break;
                    default:
                        printError("unknown constructor type found in `CodeProcessor::processFunctionCallExpr`!",
                                   functionCallExpr->startPosition(), functionCallExpr->endPosition());
                }

                return;
            }
            case Decl::Kind::TemplateFunctionInst:
            case Decl::Kind::Function: {
                auto functionDecl = llvm::dyn_cast<FunctionDecl>(foundDecl.functorDecl);

                // We handle argument casting and conversion no matter what. The below function will handle
                // converting from lvalue to rvalue, casting, and other rules for us.
                handleArgumentCasting(functionDecl->parameters(), functionCallExpr->arguments);

                if (functionDecl->isMemberFunction()) {
                    // TODO: Support `vtable` calls?
                    auto selfRef = getCurrentSelfRef(identifierExpr->startPosition(), identifierExpr->endPosition());
                    auto functionReference = new FunctionReferenceExpr(
                            identifierExpr->startPosition(),
                            identifierExpr->endPosition(),
                            functionDecl
                    );
                    processFunctionReferenceExpr(functionReference);
                    auto newExpr = new MemberFunctionCallExpr(
                            functionReference,
                            selfRef,
                            functionCallExpr->arguments,
                            functionCallExpr->startPosition(),
                            functionCallExpr->endPosition()
                    );

                    // And we steal the arguments
                    functionCallExpr->arguments.clear();
                    // Delete the old function call and replace it with the new one
                    delete functionCallExpr;
                    functionCallExpr = newExpr;
                } else {
                    // Delete the old function reference and replace it with the new one.
                    delete functionCallExpr->functionReference;
                    functionCallExpr->functionReference = createStaticFunctionReference(functionCallExpr, functionDecl);
                }

                functionCallExpr->valueType = functionDecl->returnType->deepCopy();
                functionCallExpr->valueType->setIsLValue(true);

                break;
            }
            case Decl::Kind::Property:
            case Decl::Kind::Variable:
                // TODO: Type must be `FunctionPointerType`
                printError("function pointer calls not yet supported!",
                           functionCallExpr->startPosition(), functionCallExpr->endPosition());
                break;
            default:
                printError("[INTERNAL] unknown functor type found in `CodeProcessor::processFunctionCallExpr`!",
                           functionCallExpr->startPosition(), functionCallExpr->endPosition());
                break;
        }
    } else if (llvm::isa<MemberAccessCallExpr>(functionCallExpr->functionReference)) {
        // TODO: Process the object reference normally, search the object for the function
        auto memberAccessCallExpr = llvm::dyn_cast<MemberAccessCallExpr>(functionCallExpr->functionReference);

        processExpr(memberAccessCallExpr->objectRef);

        // We must immediately handle any getters for the `objectRef`. A `PropertyRefExpr` or `SubscriptOperatorRefExpr`
        // cannot be used on its own. It must be converted to their respective `get` call expressions.
        // TODO: Should this be `handleRefGetter`? See `processMemberAccessCallExpr` for more info
        memberAccessCallExpr->objectRef = handleGetter(memberAccessCallExpr->objectRef);

        // NOTE: If `matchingTemplates` isn't empty it will have to be dealt with first as it may fill `matchingDecls`
        // NOTE: We make this a `vector<vector<>>` because of how we search for a matching template. We use the same
        //       code that searches for a matching template on `IdentifierExpr` which requires "depth"
        std::vector<std::vector<MatchingTemplateDecl>> templateMatches;
        templateMatches.push_back({});
        std::vector<MatchingFunctorDecl> matchingDecls;
        std::string const& findName = memberAccessCallExpr->member->identifier().name();

        if (memberAccessCallExpr->member->hasTemplateArguments()) {
            for (Expr*& templateArgument : memberAccessCallExpr->member->templateArguments()) {
                processExpr(templateArgument);
            }
        }

        // TODO: Support `NamespaceRefExpr` first. If it is not `NamespaceRefExpr` then check the others.
        if (llvm::isa<TypeExpr>(memberAccessCallExpr->objectRef)) {
            auto searchTypeExpr = llvm::dyn_cast<TypeExpr>(memberAccessCallExpr->objectRef);

            // TODO: Support extensions
            switch (searchTypeExpr->type->getTypeKind()) {
                case Type::Kind::Enum: {
                    auto enumType = llvm::dyn_cast<EnumType>(memberAccessCallExpr->objectRef->valueType);

                    if (memberAccessCallExpr->member->hasTemplateArguments()) {
                        // Check `memberAccessCallExpr->objectRef` for templates ONLY
                        fillListOfMatchingTemplates(enumType->decl()->ownedMembers(), findName, true,
                                                    memberAccessCallExpr->member->templateArguments(),
                                                    templateMatches[0]);
                    } else {
                        // Check `memberAccessCallExpr->objectRef`
                        fillListOfMatchingFunctors(enumType->decl()->ownedMembers(), findName, true,
                                                   functionCallExpr->arguments,
                                                   matchingDecls);
                    }

                    break;
                }
                case Type::Kind::Struct: {
                    auto structType = llvm::dyn_cast<StructType>(memberAccessCallExpr->objectRef->valueType);

                    if (memberAccessCallExpr->member->hasTemplateArguments()) {
                        // Check `memberAccessCallExpr->objectRef` for templates ONLY
                        fillListOfMatchingTemplates(structType->decl()->allMembers, findName, true,
                                                    memberAccessCallExpr->member->templateArguments(),
                                                    templateMatches[0]);
                    } else {
                        // Check `memberAccessCallExpr->objectRef`
                        fillListOfMatchingFunctors(structType->decl()->allMembers, findName, true,
                                                   functionCallExpr->arguments,
                                                   matchingDecls);
                    }

                    break;
                }
                case Type::Kind::Trait: {
                    auto traitType = llvm::dyn_cast<TraitType>(memberAccessCallExpr->objectRef->valueType);

                    if (memberAccessCallExpr->member->hasTemplateArguments()) {
                        // Check `memberAccessCallExpr->objectRef` for templates ONLY
                        fillListOfMatchingTemplates(traitType->decl()->allMembers, findName, true,
                                                    memberAccessCallExpr->member->templateArguments(),
                                                    templateMatches[0]);
                    } else {
                        // Check `memberAccessCallExpr->objectRef`
                        fillListOfMatchingFunctors(traitType->decl()->allMembers, findName, true,
                                                   functionCallExpr->arguments,
                                                   matchingDecls);
                    }

                    break;
                }
                default:
                    printError("type `" + memberAccessCallExpr->objectRef->valueType->toString() + "` does not contain "
                               "a function matching the call `" + functionCallExpr->toString() + "`!",
                               functionCallExpr->startPosition(), functionCallExpr->endPosition());
                    break;
            }
        } else {
            // TODO: Handle any smart references.
            Type* checkType = memberAccessCallExpr->objectRef->valueType;

            if (llvm::isa<ReferenceType>(checkType)) {
                checkType = llvm::dyn_cast<ReferenceType>(checkType)->nestedType;
            }

            // TODO: Support extensions...
            switch (memberAccessCallExpr->objectRef->valueType->getTypeKind()) {
                case Type::Kind::Enum: {
                    auto enumType = llvm::dyn_cast<EnumType>(memberAccessCallExpr->objectRef->valueType);

                    if (memberAccessCallExpr->member->hasTemplateArguments()) {
                        // Check `memberAccessCallExpr->objectRef` for templates ONLY
                        fillListOfMatchingTemplates(enumType->decl()->ownedMembers(), findName, false,
                                                    memberAccessCallExpr->member->templateArguments(),
                                                    templateMatches[0]);
                    } else {
                        // Check `memberAccessCallExpr->objectRef`
                        fillListOfMatchingFunctors(enumType->decl()->ownedMembers(), findName, false,
                                                   functionCallExpr->arguments,
                                                   matchingDecls);
                    }

                    break;
                }
                case Type::Kind::Struct: {
                    auto structType = llvm::dyn_cast<StructType>(memberAccessCallExpr->objectRef->valueType);

                    if (memberAccessCallExpr->member->hasTemplateArguments()) {
                        // Check `memberAccessCallExpr->objectRef` for templates ONLY
                        fillListOfMatchingTemplates(structType->decl()->allMembers, findName, false,
                                                    memberAccessCallExpr->member->templateArguments(),
                                                    templateMatches[0]);
                    } else {
                        // Check `memberAccessCallExpr->objectRef`
                        fillListOfMatchingFunctors(structType->decl()->allMembers, findName, false,
                                                   functionCallExpr->arguments,
                                                   matchingDecls);
                    }

                    break;
                }
                case Type::Kind::Trait: {
                    auto traitType = llvm::dyn_cast<TraitType>(memberAccessCallExpr->objectRef->valueType);

                    if (memberAccessCallExpr->member->hasTemplateArguments()) {
                        // Check `memberAccessCallExpr->objectRef` for templates ONLY
                        fillListOfMatchingTemplates(traitType->decl()->allMembers, findName, false,
                                                    memberAccessCallExpr->member->templateArguments(),
                                                    templateMatches[0]);
                    } else {
                        // Check `memberAccessCallExpr->objectRef`
                        fillListOfMatchingFunctors(traitType->decl()->allMembers, findName, false,
                                                   functionCallExpr->arguments,
                                                   matchingDecls);
                    }

                    break;
                }
                default:
                    printError("type `" + memberAccessCallExpr->objectRef->valueType->toString() + "` does not contain "
                               "a function matching the call `" + functionCallExpr->toString() + "`!",
                               functionCallExpr->startPosition(), functionCallExpr->endPosition());
                    break;
            }
        }

        // If the `member` is a template call we search `templateMatches` else we search `matchingDecls`
        if (memberAccessCallExpr->member->hasTemplateArguments()) {
            bool isAmbiguous = false;
            MatchingTemplateDecl* foundTemplate = findMatchingTemplateDecl(templateMatches,
                                                                           memberAccessCallExpr->member->templateArguments(),
                                                                           functionCallExpr->arguments,
                                                                           functionCallExpr->startPosition(),
                                                                           functionCallExpr->endPosition(),
                                                                           &isAmbiguous);

            if (foundTemplate == nullptr) {
                printError("template `" + memberAccessCallExpr->toString() + "` was not found!",
                           memberAccessCallExpr->startPosition(), memberAccessCallExpr->endPosition());
            }

            if (isAmbiguous) {
                printError("template `" + memberAccessCallExpr->toString() + "` is ambiguous!",
                           memberAccessCallExpr->startPosition(), memberAccessCallExpr->endPosition());
            }

            foundDecl = getTemplateFunctorInstantiation(foundTemplate,
                                                        memberAccessCallExpr->member->templateArguments(),
                                                        functionCallExpr->arguments, memberAccessCallExpr->toString(),
                                                        memberAccessCallExpr->startPosition(),
                                                        memberAccessCallExpr->endPosition());
        } else {
            if (matchingDecls.empty()) {
                printError("function `" + functionCallExpr->toString() + "` was not found!",
                           functionCallExpr->startPosition(), functionCallExpr->endPosition());
            }

            // TODO: Search for the best matching declaration
            if (matchingDecls.size() == 1) {
                foundDecl = matchingDecls[0];
            } else {
                bool isAmbiguous = false;

                for (MatchingFunctorDecl& checkMatch : matchingDecls) {
                    if (foundDecl.functorDecl == nullptr) {
                        foundDecl = checkMatch;
                        isAmbiguous = false;
                    } else {
                        if (foundDecl.kind == checkMatch.kind) {
                            isAmbiguous = true;
                        } else if (foundDecl.kind == MatchingDecl::Kind::Match) {
                            // Do nothing, we only handle ambiguity for this above if `check` is also `Match`
                        } else if (foundDecl.kind == MatchingDecl::Kind::DefaultValues) {
                            if (checkMatch.kind == MatchingDecl::Kind::Match) {
                                // Match is always better than `DefaultValues`
                                foundDecl = checkMatch;
                                isAmbiguous = false;
                            }
                        } else {
                            // In this scenario `found` is `Castable` and `check` is either `Match` or `DefaultValues`
                            foundDecl = checkMatch;
                            isAmbiguous = false;
                        }
                    }
                }
            }
        }

        if (foundDecl.functorDecl == nullptr) {
            printError("function `" + functionCallExpr->toString() + "` was not found!",
                       functionCallExpr->startPosition(), functionCallExpr->endPosition());
        }

        // TODO: Could we combine this ending with `IdentifierExpr`?
        switch (foundDecl.functorDecl->getDeclKind()) {
            case Decl::Kind::CallOperator: {
                auto callOperatorDecl = llvm::dyn_cast<CallOperatorDecl>(foundDecl.functorDecl);

                // We handle argument casting and conversion no matter what. The below function will handle
                // converting from lvalue to rvalue, casting, and other rules for us.
                handleArgumentCasting(callOperatorDecl->parameters(), functionCallExpr->arguments);

                auto callOperatorReference = new CallOperatorReferenceExpr(
                        memberAccessCallExpr->startPosition(),
                        memberAccessCallExpr->endPosition(),
                        callOperatorDecl
                );
                processCallOperatorReferenceExpr(callOperatorReference);

                Expr* selfRef = nullptr;

                // The `contextDecl` is either a `VariableDecl` or `PropertyDecl` contained in the `objectRef`
                if (llvm::isa<PropertyDecl>(foundDecl.contextDecl)) {
                    auto contextProperty = llvm::dyn_cast<PropertyDecl>(foundDecl.contextDecl);

                    auto memberRef = new MemberPropertyRefExpr(
                            memberAccessCallExpr->startPosition(),
                            memberAccessCallExpr->endPosition(),
                            memberAccessCallExpr->objectRef,
                            contextProperty
                    );

                    selfRef = handleGetter(memberRef);
                } else if (llvm::isa<VariableDecl>(foundDecl.contextDecl)) {
                    auto contextVariable = llvm::dyn_cast<VariableDecl>(foundDecl.contextDecl);

                    auto objectStructTypeQualifier =
                            callOperatorDecl->isMutable() ? Type::Qualifier::Mut : Type::Qualifier::Immut;
                    auto objectStructType = new StructType(
                            objectStructTypeQualifier,
                            // `VariableDecl` can only be contained in a `StructDecl` as a member so this is safe.
                            llvm::dyn_cast<StructDecl>(foundDecl.functorDecl->container),
                            {}, {}
                    );
                    auto memberRef = new MemberVariableRefExpr(
                            memberAccessCallExpr->startPosition(),
                            memberAccessCallExpr->endPosition(),
                            memberAccessCallExpr->objectRef,
                            objectStructType,
                            contextVariable
                    );

                    selfRef = memberRef;
                } else {
                    printError("unknown `context` declaration found for call operator!",
                               functionCallExpr->startPosition(), functionCallExpr->endPosition());
                }

                auto newExpr = new MemberFunctionCallExpr(
                        callOperatorReference,
                        selfRef,
                        functionCallExpr->arguments,
                        functionCallExpr->startPosition(),
                        functionCallExpr->endPosition()
                );

                // We steal these
                functionCallExpr->arguments.clear();
                memberAccessCallExpr->objectRef = nullptr;
                // Replace the old function with the new member function call
                delete functionCallExpr;
                functionCallExpr = newExpr;

                functionCallExpr->valueType = callOperatorDecl->returnType->deepCopy();
                functionCallExpr->valueType->setIsLValue(true);

                return;
            }
            case Decl::Kind::Constructor: {
                auto foundConstructor = llvm::dyn_cast<ConstructorDecl>(foundDecl.functorDecl);

                auto constructorReferenceExpr = new ConstructorReferenceExpr(
                        memberAccessCallExpr->startPosition(),
                        memberAccessCallExpr->member->endPosition(),
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

                switch (foundDecl.functorDecl->container->getDeclKind()) {
                    case Decl::Kind::TemplateStructInst:
                    case Decl::Kind::Struct:
                        functionCallExpr->valueType = new StructType(Type::Qualifier::Mut,
                                                                     llvm::dyn_cast<StructDecl>(foundDecl.functorDecl->container),
                                                                     {}, {});
                        functionCallExpr->valueType->setIsLValue(true);
                        break;
                    default:
                        printError("unknown constructor type found in `CodeProcessor::processFunctionCallExpr`!",
                                   functionCallExpr->startPosition(), functionCallExpr->endPosition());
                }

                return;
            }
            case Decl::Kind::TemplateFunctionInst:
            case Decl::Kind::Function: {
                auto functionDecl = llvm::dyn_cast<FunctionDecl>(foundDecl.functorDecl);

                // We handle argument casting and conversion no matter what. The below function will handle
                // converting from lvalue to rvalue, casting, and other rules for us.
                handleArgumentCasting(functionDecl->parameters(), functionCallExpr->arguments);

                if (functionDecl->isMemberFunction()) {
                    // TODO: Support `vtable` calls?
                    auto functionReference = new FunctionReferenceExpr(
                            memberAccessCallExpr->startPosition(),
                            memberAccessCallExpr->member->endPosition(),
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

                    // We steal the object reference
                    memberAccessCallExpr->objectRef = nullptr;
                    // And we steal the arguments
                    functionCallExpr->arguments.clear();
                    // Delete the old function call and replace it with the new one
                    delete functionCallExpr;
                    functionCallExpr = newExpr;
                } else {
                    // Delete the old function reference and replace it with the new one.
                    delete functionCallExpr->functionReference;
                    functionCallExpr->functionReference = createStaticFunctionReference(functionCallExpr, functionDecl);
                }

                functionCallExpr->valueType = functionDecl->returnType->deepCopy();
                functionCallExpr->valueType->setIsLValue(true);

                break;
            }
            case Decl::Kind::Property:
            case Decl::Kind::Variable:
                // TODO: Type must be `FunctionPointerType`
                printError("function pointer calls not yet supported!",
                           functionCallExpr->startPosition(), functionCallExpr->endPosition());
                break;
            default:
                printError("unknown functor type!",
                           functionCallExpr->startPosition(), functionCallExpr->endPosition());
                break;
        }
    } else {
        // TODO: Make sure the value type is a function pointer or a type containing `call`
        processExpr(functionCallExpr->functionReference);
    }
}

void gulc::CodeProcessor::fillListOfMatchingTemplatesInContainer(gulc::Decl* container, std::string const& findName,
                                                                 bool findStaticOnly,
                                                                 std::vector<Expr*> const& templateArguments,
                                                                 std::vector<MatchingTemplateDecl>& matchingTemplateDecls) {
    switch (container->getDeclKind()) {
        case Decl::Kind::Enum:
            fillListOfMatchingTemplates(llvm::dyn_cast<EnumDecl>(container)->ownedMembers(), findName,
                                        findStaticOnly, templateArguments, matchingTemplateDecls);
            break;
        case Decl::Kind::Extension:
            fillListOfMatchingTemplates(llvm::dyn_cast<ExtensionDecl>(container)->ownedMembers(), findName,
                                        findStaticOnly, templateArguments, matchingTemplateDecls);
            break;
        case Decl::Kind::Namespace:
            fillListOfMatchingTemplates(llvm::dyn_cast<NamespaceDecl>(container)->nestedDecls(), findName,
                                        findStaticOnly, templateArguments, matchingTemplateDecls);
            break;
        case Decl::Kind::Struct:
        case Decl::Kind::TemplateStructInst:
            fillListOfMatchingTemplates(llvm::dyn_cast<StructDecl>(container)->allMembers, findName,
                                        findStaticOnly, templateArguments, matchingTemplateDecls);
            break;
        case Decl::Kind::Trait:
        case Decl::Kind::TemplateTraitInst:
            fillListOfMatchingTemplates(llvm::dyn_cast<TraitDecl>(container)->allMembers, findName,
                                        findStaticOnly, templateArguments, matchingTemplateDecls);
            break;
        default:
            printError("unknown container type found in `CodeProcessor::fillListOfMatchingTemplatesInContainer`!",
                       container->startPosition(), container->endPosition());
            return;
    }
}

void gulc::CodeProcessor::fillListOfMatchingTemplates(std::vector<Decl*>& searchDecls, std::string const& findName,
                                                      bool findStaticOnly, std::vector<Expr*> const& templateArguments,
                                                      std::vector<MatchingTemplateDecl>& matchingTemplateDecls) {
    for (Decl* checkDecl : searchDecls) {
        std::vector<TemplateParameterDecl*>* checkTemplateParameters;

        if (llvm::isa<TemplateFunctionDecl>(checkDecl)) {
            if (findStaticOnly && !checkDecl->isStatic()) {
                // Template functions can be members and so we must skip non-static functions when `findStaticOnly` is
                // true
                continue;
            }

            auto checkFunction = llvm::dyn_cast<TemplateFunctionDecl>(checkDecl);

            checkTemplateParameters = &checkFunction->templateParameters();
        } else if (llvm::isa<TemplateStructDecl>(checkDecl)) {
            auto checkStruct = llvm::dyn_cast<TemplateStructDecl>(checkDecl);

            checkTemplateParameters = &checkStruct->templateParameters();
        } else if (llvm::isa<TemplateTraitDecl>(checkDecl)) {
            auto checkTrait = llvm::dyn_cast<TemplateTraitDecl>(checkDecl);

            checkTemplateParameters = &checkTrait->templateParameters();
        } else {
            // Skip non-templates
            checkTemplateParameters = nullptr;
            continue;
        }

        std::vector<std::size_t> argMatchStrengths;
        argMatchStrengths.resize(templateArguments.size());

        SignatureComparer::ArgMatchResult argMatchResult =
                SignatureComparer::compareTemplateArgumentsToParameters(
                        *checkTemplateParameters, templateArguments, argMatchStrengths);

        if (argMatchResult != SignatureComparer::ArgMatchResult::Fail) {
            MatchingDecl::Kind templateMatchKind = MatchingDecl::Kind::Unknown;

            switch (argMatchResult) {
                case SignatureComparer::ArgMatchResult::Match:
                    templateMatchKind = MatchingDecl::Kind::Match;
                    break;
                case SignatureComparer::ArgMatchResult::DefaultValues:
                    templateMatchKind = MatchingDecl::Kind::DefaultValues;
                    break;
                case SignatureComparer::ArgMatchResult::Castable:
                    templateMatchKind = MatchingDecl::Kind::Castable;
                    break;
            }

            MatchingTemplateDecl templateDeclMatch(templateMatchKind, checkDecl, argMatchStrengths);

            matchingTemplateDecls.push_back(templateDeclMatch);
        }
    }
}

void gulc::CodeProcessor::fillListOfMatchingConstructors(gulc::StructDecl* structDecl,
                                                         std::vector<LabeledArgumentExpr*> const& arguments,
                                                         std::vector<MatchingFunctorDecl>& matchingDecls) {
    // TODO: Handle type inference on template constructors
    for (ConstructorDecl* constructor : structDecl->constructors()) {
        SignatureComparer::ArgMatchResult argMatchResult =
                SignatureComparer::compareArgumentsToParameters(constructor->parameters(), arguments);

        if (argMatchResult != SignatureComparer::ArgMatchResult::Fail) {
            MatchingDecl::Kind matchKind =
                    argMatchResult == SignatureComparer::ArgMatchResult::Match
                    ? MatchingDecl::Kind::Match
                    : MatchingDecl::Kind::Castable;

            // For `ConstructorDecl` the `contextDecl` is the `StructDecl` that contains it.
            matchingDecls.emplace_back(MatchingFunctorDecl(matchKind, constructor, structDecl));
        }
    }
}

bool gulc::CodeProcessor::findBestTemplateConstructor(gulc::TemplateStructDecl* templateStructDecl,
                                                      std::vector<Expr*> const& templateArguments,
                                                      std::vector<LabeledArgumentExpr*> const& arguments,
                                                      ConstructorDecl** outMatchingConstructor,
                                                      SignatureComparer::ArgMatchResult* outArgMatchResult,
                                                      bool* outIsAmbiguous) {
    ConstructorDecl* foundConstructorDecl = nullptr;
    SignatureComparer::ArgMatchResult foundArgMatchResult = SignatureComparer::ArgMatchResult::Fail;
    bool isAmbiguous = false;

    // TODO: Handle type inference on template constructors
    for (ConstructorDecl* checkConstructor : templateStructDecl->constructors()) {
        SignatureComparer::ArgMatchResult checkArgMatchResult =
                SignatureComparer::compareArgumentsToParameters(
                        checkConstructor->parameters(), arguments,
                        templateStructDecl->templateParameters(), templateArguments);

        if (checkArgMatchResult == SignatureComparer::ArgMatchResult::Fail) {
            continue;
        }

        if (foundConstructorDecl == nullptr) {
            foundConstructorDecl = checkConstructor;
            foundArgMatchResult = checkArgMatchResult;
            isAmbiguous = false;
        } else if (foundArgMatchResult == SignatureComparer::ArgMatchResult::Match) {
            if (checkArgMatchResult == SignatureComparer::ArgMatchResult::Match) {
                // If the found constructor is ambiguous on `Match` we immediately break to let the calling function
                // error, nothing is greater than `Match`
                isAmbiguous = true;
                break;
            }
        } else if (foundArgMatchResult == SignatureComparer::ArgMatchResult::DefaultValues) {
            if (checkArgMatchResult == SignatureComparer::ArgMatchResult::Match) {
                // If the `found` is `DefaultValues` and `check` is `Match` then we replace the `found` with `check`
                foundConstructorDecl = checkConstructor;
                foundArgMatchResult = checkArgMatchResult;
                isAmbiguous = false;
                continue;
            } else if (checkArgMatchResult == SignatureComparer::ArgMatchResult::DefaultValues) {
                // The constructor is ambiguous but can be saved by an exact `Match`
                isAmbiguous = true;
            }
        } else if (foundArgMatchResult == SignatureComparer::ArgMatchResult::Castable) {
            if (checkArgMatchResult == SignatureComparer::ArgMatchResult::Match ||
                    checkArgMatchResult == SignatureComparer::ArgMatchResult::DefaultValues) {
                // If the `found` is `Castable` and `check` is `Match` or `DefaultValues` then we replace the
                // `found` with `check`
                foundConstructorDecl = checkConstructor;
                foundArgMatchResult = checkArgMatchResult;
                isAmbiguous = false;
                continue;
            } else if (checkArgMatchResult == SignatureComparer::ArgMatchResult::Castable) {
                // The constructor is ambiguous but can be saved by an exact `Match` or `DefaultValues`
                isAmbiguous = true;
            }
        }
    }

    if (foundConstructorDecl == nullptr) {
        return false;
    }

    // We always return `true` even for `ambiguous`. We return `true` for found but `isAmbiguous` still errors.
    *outMatchingConstructor = foundConstructorDecl;
    *outArgMatchResult = foundArgMatchResult;
    *outIsAmbiguous = isAmbiguous;
    return true;
}

void gulc::CodeProcessor::fillListOfMatchingFunctorsInContainer(gulc::Decl* container, std::string const& findName,
                                                                bool findStaticOnly,
                                                                std::vector<LabeledArgumentExpr*> const& arguments,
                                                                std::vector<MatchingFunctorDecl>& outMatchingDecls) {
    switch (container->getDeclKind()) {
        case Decl::Kind::Enum:
            fillListOfMatchingFunctors(llvm::dyn_cast<EnumDecl>(container)->ownedMembers(), findName,
                                       findStaticOnly, arguments, outMatchingDecls);
            break;
        case Decl::Kind::Extension:
            fillListOfMatchingFunctors(llvm::dyn_cast<ExtensionDecl>(container)->ownedMembers(), findName,
                                       findStaticOnly, arguments, outMatchingDecls);
            break;
        case Decl::Kind::Namespace:
            fillListOfMatchingFunctors(llvm::dyn_cast<NamespaceDecl>(container)->nestedDecls(), findName,
                                       findStaticOnly, arguments, outMatchingDecls);
            break;
        case Decl::Kind::Struct:
        case Decl::Kind::TemplateStructInst:
            fillListOfMatchingFunctors(llvm::dyn_cast<StructDecl>(container)->allMembers, findName,
                                       findStaticOnly, arguments, outMatchingDecls);
            break;
        case Decl::Kind::Trait:
        case Decl::Kind::TemplateTraitInst:
            fillListOfMatchingFunctors(llvm::dyn_cast<TraitDecl>(container)->allMembers, findName,
                                       findStaticOnly, arguments, outMatchingDecls);
            break;
        default:
            printError("unknown container type found in `CodeProcessor::fillListOfMatchingFunctorsInContainer`!",
                       container->startPosition(), container->endPosition());
            return;
    }
}

void gulc::CodeProcessor::fillListOfMatchingFunctors(std::vector<Decl*>& searchDecls, std::string const& findName,
                                                     bool findStaticOnly,
                                                     std::vector<LabeledArgumentExpr*> const& arguments,
                                                     std::vector<MatchingFunctorDecl>& outMatchingDecls) {
    // TODO: Support template type inference. Both `FunctionDecl` and `StructDecl` can be allowed to be templated as
    //       long as the following is true:
    //        1. All template arguments are optional
    //        2. The template arguments can be inferred by either the function arguments or constructor arguments.
    for (Decl* checkDecl : searchDecls) {
        if (findName == checkDecl->identifier().name()) {
            if (llvm::isa<VariableDecl>(checkDecl)) {
                // `VariableDecl` can be a "member"
                if (findStaticOnly && !checkDecl->isStatic()) continue;

                auto checkVariable = llvm::dyn_cast<VariableDecl>(checkDecl);
                Type* checkType = checkVariable->type;

                // TODO: Support smart references
                if (llvm::isa<ReferenceType>(checkType)) {
                    checkType = llvm::dyn_cast<ReferenceType>(checkType)->nestedType;
                }

                fillListOfMatchingFunctorsInType(checkType, checkDecl, arguments, outMatchingDecls);
            } else if (llvm::isa<PropertyDecl>(checkDecl)) {
                // `PropertyDecl` can be a "member"
                if (findStaticOnly && !checkDecl->isStatic()) continue;

                auto checkProperty = llvm::dyn_cast<PropertyDecl>(checkDecl);
                Type* checkType = checkProperty->type;

                // TODO: Support smart references
                if (llvm::isa<ReferenceType>(checkType)) {
                    checkType = llvm::dyn_cast<ReferenceType>(checkType)->nestedType;
                }

                // We can only find `mut` getters when the current function is a member function and `mut`
                bool findMut = _currentFunction != nullptr && _currentFunction->isMemberFunction() &&
                        _currentFunction->isMutable();
                // We only check for `functors` on `checkType` if the property has a proper `get` declaration
                bool getterFound = false;

                for (PropertyGetDecl* checkGetter : checkProperty->getters()) {
                    // If we can't `findMut` we have to skip mutable getters
                    if (!findMut && checkGetter->isMutable()) continue;

                    if (checkGetter->getResultType() == PropertyGetDecl::GetResult::Normal) {
                        getterFound = true;
                        break;
                    }
                }

                fillListOfMatchingFunctorsInType(checkType, checkDecl, arguments, outMatchingDecls);
            } else if (llvm::isa<FunctionDecl>(checkDecl)) {
                // `FunctionDecl` can be a "member"
                if (findStaticOnly && !checkDecl->isStatic()) continue;

                auto checkFunction = llvm::dyn_cast<FunctionDecl>(checkDecl);

                // We can only find `mut` functions when the current function is a member function and `mut`
                bool findMut = _currentFunction != nullptr && _currentFunction->isMemberFunction() &&
                        _currentFunction->isMutable();

                // Function must either be `static` or `findMut == checkFunction->isMutable()`
                if (!checkFunction->isStatic() && findMut != checkFunction->isMutable()) {
                    continue;
                }

                SignatureComparer::ArgMatchResult checkArgMatchResult =
                        SignatureComparer::compareArgumentsToParameters(checkFunction->parameters(), arguments);

                MatchingDecl::Kind matchKind = MatchingDecl::Kind::Unknown;

                switch (checkArgMatchResult) {
                    case SignatureComparer::ArgMatchResult::Match:
                        matchKind = MatchingDecl::Kind::Match;
                        break;
                    case SignatureComparer::ArgMatchResult::DefaultValues:
                        matchKind = MatchingDecl::Kind::DefaultValues;
                        break;
                    case SignatureComparer::ArgMatchResult::Castable:
                        matchKind = MatchingDecl::Kind::Castable;
                        break;
                    case SignatureComparer::ArgMatchResult::Fail:
                        return;
                }

                // For `FunctionDecl` we don't have a `contextDecl`
                MatchingFunctorDecl newMatch(matchKind, checkFunction, nullptr);

                outMatchingDecls.push_back(newMatch);
            } else if (llvm::isa<StructDecl>(checkDecl)) {
                // `StructDecl` CANNOT be a "member", we ignore `findStaticOnly` as struct cannot be non-static
                auto checkStruct = llvm::dyn_cast<StructDecl>(checkDecl);

                fillListOfMatchingConstructors(checkStruct, arguments, outMatchingDecls);
            }
        }
    }
}

void gulc::CodeProcessor::fillListOfMatchingFunctorsInType(gulc::Type* checkType, Decl* contextDecl,
                                                           std::vector<LabeledArgumentExpr*> const& arguments,
                                                           std::vector<MatchingFunctorDecl>& outMatchingDecls) {
    switch (checkType->getTypeKind()) {
        case Type::Kind::FunctionPointer: {
            auto checkFuncPointer = llvm::dyn_cast<FunctionPointerType>(checkType);

            SignatureComparer::ArgMatchResult checkArgMatchResult =
                    SignatureComparer::compareArgumentsToParameters(checkFuncPointer->parameters, arguments);

            MatchingDecl::Kind matchKind = MatchingDecl::Kind::Unknown;

            switch (checkArgMatchResult) {
                case SignatureComparer::ArgMatchResult::Match:
                    matchKind = MatchingDecl::Kind::Match;
                    break;
                case SignatureComparer::ArgMatchResult::DefaultValues:
                    matchKind = MatchingDecl::Kind::DefaultValues;
                    break;
                case SignatureComparer::ArgMatchResult::Castable:
                    matchKind = MatchingDecl::Kind::Castable;
                    break;
                case SignatureComparer::ArgMatchResult::Fail:
                    return;
            }

            // For `FunctionPointerType` the `contextDecl` is the `functor` instead of the `context`
            MatchingFunctorDecl newMatch(matchKind, contextDecl, nullptr);

            outMatchingDecls.push_back(newMatch);

            break;
        }
        case Type::Kind::Struct: {
            auto checkStruct = llvm::dyn_cast<StructType>(checkType);

            fillListOfMatchingCallOperators(checkStruct->decl()->allMembers,
                                            checkStruct->qualifier() == Type::Qualifier::Mut, contextDecl,
                                            arguments, outMatchingDecls);

            break;
        }
        case Type::Kind::Trait: {
            auto checkTrait = llvm::dyn_cast<TraitType>(checkType);

            fillListOfMatchingCallOperators(checkTrait->decl()->allMembers,
                                            checkTrait->qualifier() == Type::Qualifier::Mut, contextDecl,
                                            arguments, outMatchingDecls);

            break;
        }
        default:
            // Skip the variable if it isn't a type known to support `operator ()`
            return;
    }
}

void gulc::CodeProcessor::fillListOfMatchingCallOperators(std::vector<Decl*>& searchDecls, bool findMut,
                                                          Decl* contextDecl,
                                                          std::vector<LabeledArgumentExpr*> const& arguments,
                                                          std::vector<MatchingFunctorDecl>& outMatchingDecls) {
    for (Decl* checkDecl : searchDecls) {
        // The struct/trait must be `mut` to call `mut` call operators.
        if (!findMut && checkDecl->isMutable()) continue;

        if (llvm::isa<CallOperatorDecl>(checkDecl)) {
            auto checkCallOperator = llvm::dyn_cast<CallOperatorDecl>(checkDecl);

            SignatureComparer::ArgMatchResult checkArgMatchResult =
                    SignatureComparer::compareArgumentsToParameters(checkCallOperator->parameters(), arguments);

            MatchingDecl::Kind matchKind = MatchingDecl::Kind::Unknown;

            switch (checkArgMatchResult) {
                case SignatureComparer::ArgMatchResult::Match:
                    matchKind = MatchingDecl::Kind::Match;
                    break;
                case SignatureComparer::ArgMatchResult::DefaultValues:
                    matchKind = MatchingDecl::Kind::DefaultValues;
                    break;
                case SignatureComparer::ArgMatchResult::Castable:
                    matchKind = MatchingDecl::Kind::Castable;
                    break;
                case SignatureComparer::ArgMatchResult::Fail:
                    return;
            }

            // For the `CallOperatorDecl` we make the call operator the `functor` and the `contextDecl` container
            // the `context`
            MatchingFunctorDecl newMatch(matchKind, checkCallOperator, contextDecl);

            outMatchingDecls.push_back(newMatch);
        }
    }
}

gulc::CodeProcessor::MatchingTemplateDecl* gulc::CodeProcessor::findMatchingTemplateDecl(
        std::vector<std::vector<MatchingTemplateDecl>>& allMatchingTemplateDecls,
        std::vector<Expr*> const& templateArguments, std::vector<LabeledArgumentExpr*> const& arguments,
        TextPosition errorStartPosition, TextPosition errorEndPosition, bool* outIsAmbiguous) {
    MatchingTemplateDecl* foundTemplate = nullptr;
    SignatureComparer::ArgMatchResult foundArgMatchResult = SignatureComparer::ArgMatchResult::Fail;
    std::size_t foundMatchDepth = std::numeric_limits<std::size_t>::max();
    bool isAmbiguous = false;

    for (std::size_t matchDepth = 0; matchDepth < allMatchingTemplateDecls.size(); ++matchDepth) {
        // NOTE: A "perfect" match would mean the following:
        //        1. Template arguments all have strength of "0"
        //        2. Template arguments do NOT use default values
        //        3. Normal arguments all match the parameter types exactly
        //        4. Normal arguments do NOT use default values
        //       The following will make a match imperfect:
        //        * Requires cast
        //        * Requires default values
        //        * Template specialization strength > 0
        // If a match is `perfect` we can skip everything after the current depth. We MUST continue searching
        // the current depth for ambiguity but past that there is no more searching required.
        // TODO: Actually detect and set this...
        bool matchIsPerfect = false;

        for (MatchingTemplateDecl& checkMatch : allMatchingTemplateDecls[matchDepth]) {
            if (foundTemplate == nullptr) {
                switch (checkMatch.matchingDecl->getDeclKind()) {
                    case Decl::Kind::TemplateFunction: {
                        auto checkTemplateFunction =
                                llvm::dyn_cast<TemplateFunctionDecl>(checkMatch.matchingDecl);

                        SignatureComparer::ArgMatchResult checkArgMatchResult =
                                SignatureComparer::compareArgumentsToParameters(
                                        checkTemplateFunction->parameters(),
                                        arguments,
                                        checkTemplateFunction->templateParameters(),
                                        templateArguments);

                        if (checkArgMatchResult == SignatureComparer::ArgMatchResult::Fail) {
                            // A failure means there is nothing left to check. The templates arguments were
                            // correct but the normal arguments were not.
                            continue;
                        }

                        foundTemplate = &checkMatch;
                        foundArgMatchResult = checkArgMatchResult;
                        foundMatchDepth = matchDepth;
                        isAmbiguous = false;
                        break;
                    }
                    case Decl::Kind::TemplateStruct: {
                        auto checkTemplateStruct = llvm::dyn_cast<TemplateStructDecl>(checkMatch.matchingDecl);

                        ConstructorDecl* matchingConstructor = nullptr;
                        SignatureComparer::ArgMatchResult checkArgMatchResult;
                        bool checkIsAmbiguous = false;

                        if (findBestTemplateConstructor(checkTemplateStruct, templateArguments, arguments,
                                                        &matchingConstructor, &checkArgMatchResult,
                                                        &checkIsAmbiguous)) {
                            if (checkArgMatchResult == SignatureComparer::ArgMatchResult::Fail) {
                                // A failure means there is nothing left to check. The templates arguments were
                                // correct but the normal arguments were not.
                                continue;
                            }

                            foundTemplate = &checkMatch;
                            foundArgMatchResult = checkArgMatchResult;
                            foundMatchDepth = matchDepth;
                            isAmbiguous = checkIsAmbiguous;
                            break;
                        }
                    }
                    case Decl::Kind::TemplateTrait: {
                        // `init` declarations within a template only refer to constructors that a `struct`
                        // must define. They cannot be used to construct a trait (as a trait cannot be
                        // constructed at all)
                        continue;
                    }
                    default:
                        printError("unknown template found in `CodeGen::findMatchingTemplateDecl`!",
                                   errorStartPosition, errorEndPosition);
                        break;
                }
            } else {
                bool shouldReplaceDecl = false;
                bool exactSame = false;

                // Regardless of what kind the matches are if they are both the same we go through checking
                // strengths from left to right.
                if (foundTemplate->kind == checkMatch.kind) {
                    // TODO: `exactSame` can only be true if the matches are at the exact same depth?
                    exactSame = foundMatchDepth == matchDepth;
                    bool foundIsBetter = false;


                    // We break on first difference. If the first different argument is closer to zero then we
                    // replace `foundMatch`, if it was worse then we continue searching. If the arguments have the
                    // exact same strength then we set `isAmbiguous` but keep searching in case something better
                    // comes along.
                    for (std::size_t i = 0; i < foundTemplate->argMatchStrengths.size(); ++i) {
                        std::size_t foundStrength = foundTemplate->argMatchStrengths[i];
                        std::size_t checkStrength = checkMatch.argMatchStrengths[i];

                        if (checkStrength < foundStrength) {
                            shouldReplaceDecl = true;
                            exactSame = false;
                            break;
                        } else if (checkStrength > foundStrength) {
                            foundIsBetter = true;
                            exactSame = false;
                            break;
                        }
                    }

                    // If `foundIsBetter` is true then we have to skip the rest of the checks.
                    if (foundIsBetter) {
                        continue;
                    }
                } else if (foundTemplate->kind == MatchingDecl::Kind::Match) {
                    // We continue the loop unless `checkMatch` is also exact, in that scenario the above code
                    // will execute instead.
                    continue;
                } else if (foundTemplate->kind == MatchingDecl::Kind::DefaultValues) {
                    // If we found a `DefaultValues` match then we follow these rules:
                    //  1. If the `checkMatch` is `Exact` we replace without question
                    //  2. If the `checkMatch` is also `DefaultValues` then we compare strengths
                    //  3. If the `checkMatch` is `Castable` we skip it
                    if (checkMatch.kind == MatchingDecl::Kind::Match) {
                        shouldReplaceDecl = true;
                    } else if (checkMatch.kind == MatchingDecl::Kind::DefaultValues) {
                        // In this scenario the matches are the same kind so this will never execute.
                        continue;
                    }
                }

                // We do nothing for `Castable` at this point.
                // Castable only makes it past this loop on one of two conditions:
                //  1. It is the only match in the list
                //  2. There are multiple matches in the list and we choose the strongest match.

                // NOTE: If `exactSame` is false then we should always replace if the arguments match at all.
                //       If `exactSame` is true then we should replace only if the arguments are a better match,
                //       if arguments are the same as well then it is a true `exactSame` and we must mark for
                //       ambiguity.
                if (shouldReplaceDecl) {
                    // We create a new `mustReplace` variable because if `exactSame` is true then it will be
                    // modified based on the results of our checks below.
                    bool mustReplace = !exactSame;

                    switch (checkMatch.matchingDecl->getDeclKind()) {
                        case Decl::Kind::TemplateFunction: {
                            auto checkTemplateFunction =
                                    llvm::dyn_cast<TemplateFunctionDecl>(checkMatch.matchingDecl);

                            SignatureComparer::ArgMatchResult checkArgMatchResult =
                                    SignatureComparer::compareArgumentsToParameters(
                                            checkTemplateFunction->parameters(),
                                            arguments,
                                            checkTemplateFunction->templateParameters(),
                                            templateArguments);

                            if (checkArgMatchResult == SignatureComparer::ArgMatchResult::Fail) {
                                // A failure means there is nothing left to check. The templates arguments were
                                // correct but the normal arguments were not.
                                continue;
                            }

                            // If `mustReplace` is true then we must do what the name says. It means the
                            // template arguments matched better for the new template.
                            if (mustReplace) {
                                foundTemplate = &checkMatch;
                                foundMatchDepth = matchDepth;
                                foundArgMatchResult = checkArgMatchResult;
                                isAmbiguous = false;
                            } else {
                                if (foundArgMatchResult == checkArgMatchResult) {
                                    // If the normal argument checks are exactly the same then we compare based
                                    // on the `exactSame` variable.
                                    if (exactSame) {
                                        isAmbiguous = true;
                                        matchIsPerfect = false;
                                    }
                                } else {
                                    // Choose which is better
                                    if (foundArgMatchResult == SignatureComparer::ArgMatchResult::Match) {
                                        // Nothing to do. If `check` is the same it would be handeled above.
                                        // Everything else is ignored.
                                    } else if (foundArgMatchResult ==
                                               SignatureComparer::ArgMatchResult::DefaultValues) {
                                        // If `found` is `DefaultValues` and `check` is `Match` then `check` is
                                        // better.
                                        // If `found` is `DefaultValues` and `check` is too then `check` is
                                        // better as `check` passed the `strength` test better.
                                        // If `found` is `DefaultValues` and `check` is `Castable` we do
                                        // nothing.
                                        if (checkArgMatchResult == SignatureComparer::ArgMatchResult::Match ||
                                            checkArgMatchResult ==
                                            SignatureComparer::ArgMatchResult::DefaultValues) {
                                            foundTemplate = &checkMatch;
                                            foundMatchDepth = matchDepth;
                                            foundArgMatchResult = checkArgMatchResult;
                                            isAmbiguous = false;
                                        }
                                    } else if (foundArgMatchResult ==
                                               SignatureComparer::ArgMatchResult::Castable) {
                                        // Regardless of what `check` is it is better because it passed the
                                        // strength test better. If `check` failed the strength test it would
                                        // be skipped and had it been the same we would be checking in the
                                        // prior `if` that checks `exactSame`.
                                        foundTemplate = &checkMatch;
                                        foundMatchDepth = matchDepth;
                                        foundArgMatchResult = checkArgMatchResult;
                                        isAmbiguous = false;
                                    }
                                }
                            }

                            break;
                        }
                        case Decl::Kind::TemplateStruct: {
                            auto checkTemplateStruct = llvm::dyn_cast<TemplateStructDecl>(
                                    checkMatch.matchingDecl);

                            ConstructorDecl* checkConstructor;
                            SignatureComparer::ArgMatchResult checkArgMatchResult =
                                    SignatureComparer::ArgMatchResult::Fail;
                            bool checkIsAmbiguous = false;

                            if (findBestTemplateConstructor(checkTemplateStruct,
                                                            templateArguments,
                                                            arguments,
                                                            &checkConstructor, &checkArgMatchResult,
                                                            &checkIsAmbiguous)) {
                                // If `mustReplace` is true then we must do what the name says. It means the
                                // template arguments matched better for the new template.
                                if (mustReplace) {
                                    foundTemplate = &checkMatch;
                                    foundMatchDepth = matchDepth;
                                    foundArgMatchResult = checkArgMatchResult;
                                    // We set to `checkIsAmbiguous` as there could be multiple matching
                                    // `init` declarations.
                                    isAmbiguous = checkIsAmbiguous;
                                } else {
                                    // If both are same we potentially have an ambiguity. That can only be true
                                    // if the `exactSame` variable has already been evaluated to `true`
                                    if (foundArgMatchResult == checkArgMatchResult) {
                                        // There can only be an ambiguity if `exactSame` is true
                                        if (exactSame) {
                                            isAmbiguous = true;
                                            matchIsPerfect = false;
                                        }
                                    } else if (foundArgMatchResult ==
                                               SignatureComparer::ArgMatchResult::Match) {
                                        // Nothing to do here, we only account for if both are `Match` above...
                                    } else if (foundArgMatchResult ==
                                               SignatureComparer::ArgMatchResult::DefaultValues) {
                                        // `DefaultValues` is worse than `Match`, `Match` is exact.
                                        if (checkArgMatchResult == SignatureComparer::ArgMatchResult::Match) {
                                            foundTemplate = &checkMatch;
                                            foundMatchDepth = matchDepth;
                                            foundArgMatchResult = checkArgMatchResult;
                                            // We set to `checkIsAmbiguous` as there could be multiple matching
                                            // `init` declarations.
                                            isAmbiguous = checkIsAmbiguous;
                                        }

                                        // `DefaultValues` is handled above, `Castable` is ignored.
                                    } else if (foundArgMatchResult ==
                                               SignatureComparer::ArgMatchResult::Castable) {
                                        // Regardless of what `check` is it is better because it passed the
                                        // strength test better. If `check` failed the strength test it would
                                        // be skipped and had it been the same we would be checking in the
                                        // prior `if` that checks `exactSame`.
                                        foundTemplate = &checkMatch;
                                        foundMatchDepth = matchDepth;
                                        foundArgMatchResult = checkArgMatchResult;
                                        // We set to `checkIsAmbiguous` as there could be multiple matching
                                        // `init` declarations.
                                        isAmbiguous = checkIsAmbiguous;
                                    }
                                }
                            }

                            break;
                        }
                        case Decl::Kind::TemplateTrait: {
                            // `init` declarations within a template only refer to constructors that a `struct`
                            // must define. They cannot be used to construct a trait (as a trait cannot be
                            // constructed at all)
                            continue;
                        }
                        default:
                            printError("unknown template found in `CodeGen::findMatchingTemplateDecl`!",
                                       errorStartPosition, errorEndPosition);
                            break;
                    }
                }
            }
        }

        // A perfect match means we don't have to search any further depths
        if (matchIsPerfect) {
            break;
        }
    }

    *outIsAmbiguous = isAmbiguous;
    return foundTemplate;
}

gulc::CodeProcessor::MatchingFunctorDecl* gulc::CodeProcessor::findMatchingFunctorDecl(
        std::vector<MatchingFunctorDecl>& allMatchingFunctorDecls, bool findExactOnly, bool* outIsAmbiguous) {
    MatchingFunctorDecl* result = nullptr;
    bool isAmbiguous = false;

    for (MatchingFunctorDecl& checkMatchingFunctorDecl : allMatchingFunctorDecls) {
        if (result == nullptr) {
            if (findExactOnly) {
                if (checkMatchingFunctorDecl.kind == MatchingDecl::Kind::Match) {
                    result = &checkMatchingFunctorDecl;
                }
            } else {
                result = &checkMatchingFunctorDecl;
            }
        } else {
            if (findExactOnly) {
                if (checkMatchingFunctorDecl.kind == MatchingDecl::Kind::Match) {
                    // `result` will always be filled in this scenario. As such it is always ambiguous here so we
                    // break.
                    isAmbiguous = true;
                    break;
                }
            } else {
                if (result->kind == checkMatchingFunctorDecl.kind) {
                    // Keep searching but it is currently considered ambiguous.
                    isAmbiguous = true;
                } else if (result->kind == MatchingDecl::Kind::Match) {
                    // Do nothing. If `result` is already `match` it will be handled above.
                } else if (result->kind == MatchingDecl::Kind::DefaultValues) {
                    // Match is better than DefaultValues
                    if (checkMatchingFunctorDecl.kind == MatchingDecl::Kind::Match) {
                        result = &checkMatchingFunctorDecl;
                        isAmbiguous = false;
                    }
                } else {
                    // It is impossible for `check` to be `Castable` here so it must be better. As a result we set
                    // `result` to `check` and set `isAmbiguous` to false.
                    result = &checkMatchingFunctorDecl;
                    isAmbiguous = false;
                }
            }
        }
    }

    *outIsAmbiguous = isAmbiguous;
    return result;
}

gulc::CodeProcessor::MatchingFunctorDecl gulc::CodeProcessor::getTemplateFunctorInstantiation(
        gulc::CodeProcessor::MatchingTemplateDecl* matchingTemplateDecl, std::vector<Expr*>& templateArguments,
        std::vector<LabeledArgumentExpr*> const& arguments,
        std::string const& errorString, gulc::TextPosition errorStartPosition, gulc::TextPosition errorEndPosition) {
    MatchingFunctorDecl foundDecl;
    DeclInstantiator declInstantiator(_target, _filePaths);

    switch (matchingTemplateDecl->matchingDecl->getDeclKind()) {
        case Decl::Kind::TemplateFunction: {
            auto templateFunctionDecl = llvm::dyn_cast<TemplateFunctionDecl>(matchingTemplateDecl->matchingDecl);

            TemplateFunctionInstDecl* checkTemplateFunctionInst =
                    declInstantiator.instantiateTemplateFunction(_currentFile, templateFunctionDecl,
                                                                 templateArguments,
                                                                 errorString,
                                                                 errorStartPosition, errorEndPosition);

            foundDecl = MatchingFunctorDecl(matchingTemplateDecl->kind, checkTemplateFunctionInst, nullptr);

            break;
        }
        case Decl::Kind::TemplateStruct: {
            auto templateStructDecl = llvm::dyn_cast<TemplateStructDecl>(matchingTemplateDecl->matchingDecl);

            TemplateStructInstDecl* checkTemplateStructInst =
                    declInstantiator.instantiateTemplateStruct(_currentFile, templateStructDecl,
                                                               templateArguments,
                                                               errorString,
                                                               errorStartPosition, errorEndPosition);

            // Find the matching constructor again...
            std::vector<MatchingFunctorDecl> constructorMatches;

            fillListOfMatchingConstructors(checkTemplateStructInst, arguments, constructorMatches);

            if (constructorMatches.empty()) {
                printError("[INTERNAL] template search found matching constructor that cannot be found in "
                           "instantiation!",
                           errorStartPosition, errorEndPosition);
            }

            if (constructorMatches.size() == 1) {
                foundDecl = constructorMatches[0];
            } else {
                // Search for an exact match or default values match...
                bool constructorIsAmbiguous = false;

                for (MatchingFunctorDecl& checkMatch : constructorMatches) {
                    if (foundDecl.functorDecl == nullptr) {
                        foundDecl = checkMatch;
                        constructorIsAmbiguous = false;
                    } else {
                        if (foundDecl.kind == checkMatch.kind) {
                            constructorIsAmbiguous = true;
                        } else if (foundDecl.kind == MatchingDecl::Kind::Match) {
                            // Nothing to do. If `check` is the same it will be handled above.
                        } else if (foundDecl.kind == MatchingDecl::Kind::DefaultValues) {
                            // We replace `DefaultValues` with `Match`
                            if (checkMatch.kind == MatchingDecl::Kind::Match) {
                                foundDecl = checkMatch;
                                constructorIsAmbiguous = false;
                            }
                        } else if (foundDecl.kind == MatchingDecl::Kind::Castable) {
                            // We set it no matter what here. `checkMatch` can't be `Castable` here it can only
                            // be better.
                            foundDecl = checkMatch;
                            constructorIsAmbiguous = false;
                        }
                    }
                }

                if (constructorIsAmbiguous) {
                    printError("constructor call is ambiguous!",
                               errorStartPosition, errorEndPosition);
                }
            }

            break;
        }
        case Decl::Kind::TemplateTrait:
            // `init` declarations within a template only refer to constructors that a `struct`
            // must define. They cannot be used to construct a trait (as a trait cannot be
            // constructed at all)
            break;
        default:
            printError("unknown template found in `CodeGen::getTemplateFunctorInstantiation`!",
                       errorStartPosition, errorEndPosition);
            break;
    }

    return foundDecl;
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
    processExpr(hasExpr->expr);
    processPrototypeDecl(hasExpr->decl);

    switch (hasExpr->expr->getExprKind()) {
        case Expr::Kind::Type:
            // TODO: Do we do anything yet? We could pre-solve the `has` expression here...
            break;
        default:
            printError("unknown expression as left value of `has` operator, expected a type!",
                       hasExpr->startPosition(), hasExpr->endPosition());
            break;
    }

    hasExpr->valueType = new BoolType(Type::Qualifier::Immut, hasExpr->startPosition(), hasExpr->endPosition());
    hasExpr->valueType->setIsLValue(false);
}

void gulc::CodeProcessor::processIdentifierExpr(gulc::Expr*& expr) {
    // NOTE: The generic `processIdentifierExpr` should ONLY look for normal variables and types, it should NOT look
    //       for functions. `processFunctionCallExpr` and `processSubscriptCallExpr` should handle `IdentifierExpr` as
    //       special cases in their own functions. They should ALSO handle `MemberAccessCallExpr` as a special case.
    //       Once `CodeProcessor` is done `IdentifierExpr` should NEVER appear in the AST again (except in
    //       uninstantiated templates)
    auto identifierExpr = llvm::dyn_cast<IdentifierExpr>(expr);
    std::string const& findName = identifierExpr->identifier().name();
    Decl* foundDecl = nullptr;

    // TODO: I think we should rewrite this to use the newer more abstract functions we wrote for
    //       `processFunctionCallExpr`

    if (identifierExpr->hasTemplateArguments()) {
        for (Expr*& templateArgument : identifierExpr->templateArguments()) {
            processExpr(templateArgument);
        }

        // TODO: To account for both overloading and template specialization we have to search everything to properly
        //       find any templates
        // TODO: We shouldn't have to. We just need to constantly check if we found a perfect match.
        std::vector<std::vector<MatchingTemplateDecl>> foundTemplates;

        if (_currentContainer != nullptr) {
            foundTemplates.push_back({});

            fillListOfMatchingTemplatesInContainer(_currentContainer, findName, false,
                                                   identifierExpr->templateArguments(),
                                                   foundTemplates[foundTemplates.size() - 1]);
        }

        // TODO: Search current namespace?

        if (_currentFile != nullptr) {
            foundTemplates.push_back({});

            fillListOfMatchingTemplates(_currentFile->declarations, findName, false,
                                        identifierExpr->templateArguments(),
                                        foundTemplates[foundTemplates.size() - 1]);
        }

        if (_currentFile != nullptr && !_currentFile->imports.empty()) {
            foundTemplates.push_back({});

            for (ImportDecl* checkImport : _currentFile->imports) {
                fillListOfMatchingTemplates(checkImport->pointToNamespace->nestedDecls(), findName, false,
                                            identifierExpr->templateArguments(),
                                            foundTemplates[foundTemplates.size() - 1]);
            }
        }

        // TODO: Find the best matching template, this should be easy since we don't have normal parameters to deal
        //       with like we did with `processFunctionCallExpr`
        MatchingTemplateDecl* foundTemplate = nullptr;
        std::size_t foundDepth = std::numeric_limits<std::size_t>::max();
        bool isAmbiguous = false;

        for (std::size_t matchDepth = 0; matchDepth < foundTemplates.size(); ++matchDepth) {
            // TODO: Account for strengths
            for (MatchingTemplateDecl& checkTemplate : foundTemplates[matchDepth]) {
                if (foundTemplate == nullptr) {
                    foundTemplate = &checkTemplate;
                    foundDepth = matchDepth;
                    isAmbiguous = false;
                } else {
                    if (foundTemplate->kind == checkTemplate.kind) {
                        // The templates can only be ambiguous when at the same depth
                        bool exactSame = foundDepth == matchDepth;
                        bool foundIsBetter = false;
                        // If this is true then we replace `found` with `check`
                        bool shouldReplaceFound = false;

                        // We break on first difference. If the first different argument is closer to zero then we
                        // replace `foundMatch`, if it was worse then we continue searching. If the arguments have the
                        // exact same strength then we set `isAmbiguous` but keep searching in case something better
                        // comes along.
                        for (std::size_t i = 0; i < foundTemplate->argMatchStrengths.size(); ++i) {
                            std::size_t foundStrength = foundTemplate->argMatchStrengths[i];
                            std::size_t checkStrength = checkTemplate.argMatchStrengths[i];

                            if (checkStrength < foundStrength) {
                                shouldReplaceFound = true;
                                exactSame = false;
                                break;
                            } else if (checkStrength > foundStrength) {
                                foundIsBetter = true;
                                exactSame = false;
                                break;
                            }
                        }

                        // If `foundIsBetter` is true then we have to skip the rest of the checks.
                        if (foundIsBetter) {
                            continue;
                        } else if (exactSame) {
                            isAmbiguous = true;
                        } else if (shouldReplaceFound) {
                            foundTemplate = &checkTemplate;
                            foundDepth = matchDepth;
                            isAmbiguous = false;
                        }
                    } else if (foundTemplate->kind == MatchingDecl::Kind::Match) {
                        // Nothing to do, we only check for ambiguity above when `check` is also `Match`
                    } else if (foundTemplate->kind == MatchingDecl::Kind::DefaultValues) {
                        // `Match` is always better than `DefaultValues`
                        if (checkTemplate.kind == MatchingDecl::Kind::Match) {
                            foundTemplate = &checkTemplate;
                            foundDepth = matchDepth;
                            isAmbiguous = false;
                        }
                    } else {
                        // `found` is `Castable` while `check` is either `Match` or `DefaultValues`, making `check`
                        // better
                        foundTemplate = &checkTemplate;
                        foundDepth = matchDepth;
                        isAmbiguous = false;
                    }
                }
            }
        }

        if (foundTemplate == nullptr) {
            printError("template `" + identifierExpr->toString() + "` was not found!",
                       identifierExpr->startPosition(), identifierExpr->endPosition());
        }

        if (isAmbiguous) {
            printError("template `" + identifierExpr->toString() + "` was ambiguous!",
                       identifierExpr->startPosition(), identifierExpr->endPosition());
        }

        DeclInstantiator declInstantiator(_target, _filePaths);

        switch (foundTemplate->matchingDecl->getDeclKind()) {
            case Decl::Kind::TemplateStruct: {
                auto templateStructDecl = llvm::dyn_cast<TemplateStructDecl>(foundTemplate->matchingDecl);

                TemplateStructInstDecl* checkTemplateStructInst =
                        declInstantiator.instantiateTemplateStruct(_currentFile, templateStructDecl,
                                                                   identifierExpr->templateArguments(),
                                                                   identifierExpr->toString(),
                                                                   identifierExpr->startPosition(),
                                                                   identifierExpr->endPosition());

                foundDecl = checkTemplateStructInst;

                break;
            }
            case Decl::Kind::TemplateTrait: {
                auto templateTraitDecl = llvm::dyn_cast<TemplateTraitDecl>(foundTemplate->matchingDecl);

                TemplateTraitInstDecl* checkTemplateTraitInst =
                        declInstantiator.instantiateTemplateTrait(_currentFile, templateTraitDecl,
                                                                  identifierExpr->templateArguments(),
                                                                  identifierExpr->toString(),
                                                                  identifierExpr->startPosition(),
                                                                  identifierExpr->endPosition());

                foundDecl = checkTemplateTraitInst;

                break;
            }
            default:
                printError("unknown template declaration reference!",
                           identifierExpr->startPosition(), identifierExpr->endPosition());
                break;
        }
    } else {
        // First we check if it is a built in type
        if (BuiltInType::isBuiltInType(identifierExpr->identifier().name())) {
            auto builtInType = BuiltInType::get(Type::Qualifier::Unassigned,
                                                identifierExpr->identifier().name(),
                                                expr->startPosition(), expr->endPosition());
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

                    // `in` and `out` parameters are references.
                    if (parameter->parameterKind() != ParameterDecl::ParameterKind::Val) {
                        newExpr->valueType = new ReferenceType(Type::Qualifier::Unassigned, newExpr->valueType);
                    }

                    newExpr->valueType->setIsLValue(true);
                    // Delete the old identifier
                    delete expr;
                    // Set it to the local variable reference and exit our function.
                    expr = newExpr;
                    return;
                }
            }
        }

        bool isAmbiguous = false;

        // Check our current container
        if (_currentContainer != nullptr) {
            findMatchingDeclInContainer(_currentContainer, findName, &foundDecl, &isAmbiguous);
        }

        // TODO: Search current namespace?

        // Check our current file
        if (foundDecl == nullptr) {
            findMatchingDecl(_currentFile->declarations, findName, &foundDecl, &isAmbiguous);
        }

        // Check the imports with an ambiguity check
        if (foundDecl == nullptr) {
            // NOTE: We continue searching even after `foundDecl` is set for the imports, if `foundDecl` isn't null on
            //       a match then there is an ambiguity and we need to error out saying such.
            for (ImportDecl* checkImport : _currentFile->imports) {
                Decl* tmpFoundDecl = nullptr;
                bool tmpIsAmbiguous = false;

                if (findMatchingDecl(checkImport->pointToNamespace->nestedDecls(), findName, &tmpFoundDecl,
                                     &tmpIsAmbiguous)) {
                    if (foundDecl != nullptr || tmpIsAmbiguous) {
                        // TODO: Use `foundDecl` and `tmpFoundDecl` to show the two ambiguous identifier paths
                        isAmbiguous = true;
                        // Break from the entire loop to trigger the normal ambiguous error message
                        break;
                    }

                    foundDecl = tmpFoundDecl;
                }
            }
        }

        if (isAmbiguous) {
            printError("identifier `" + identifierExpr->toString() + "` is ambiguous!",
                       identifierExpr->startPosition(), identifierExpr->endPosition());
        }
    }

    if (foundDecl == nullptr) {
        printError("identifier `" + identifierExpr->toString() + "` was not found!",
                   identifierExpr->startPosition(), identifierExpr->endPosition());
    }

    switch (foundDecl->getDeclKind()) {
        case Decl::Kind::CallOperator:
        case Decl::Kind::Function:
            // TODO: I'm thinking for function pointers we will do `Type::function(_: i32)`. We will have to do that
            //       since we allow both argument label overloading as well as argument type overloading.
            printError("function calls cannot be used without arguments, you must provide type arguments to grab a "
                       "function pointer!",
                       identifierExpr->startPosition(), identifierExpr->endPosition());
            break;
        case Decl::Kind::EnumConst: {
            auto enumConst = llvm::dyn_cast<EnumConstDecl>(foundDecl);
            auto enumDecl = llvm::dyn_cast<EnumDecl>(enumConst->container);
            auto newExpr = new EnumConstRefExpr(expr->startPosition(), expr->endPosition(), enumConst);
            processEnumConstRefExpr(newExpr);
            // Delete the old identifier
            delete expr;
            expr = newExpr;
            return;
        }
        case Decl::Kind::Enum: {
            auto enumDecl = llvm::dyn_cast<EnumDecl>(foundDecl);
            auto newExpr = new TypeExpr(new EnumType(Type::Qualifier::Unassigned, enumDecl,
                                                     expr->startPosition(), expr->endPosition()));
            // Delete the old identifier
            delete expr;
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
            // NOTE: This will be handled upstream through `handle*Getter` and `handleSetter`
            auto propertyDecl = llvm::dyn_cast<PropertyDecl>(foundDecl);
            auto newExpr = new PropertyRefExpr(expr->startPosition(), expr->endPosition(), propertyDecl);
            // Delete the old identifier
            delete expr;
            expr = newExpr;
            break;
        }
        case Decl::Kind::Struct: {
            auto structDecl = llvm::dyn_cast<StructDecl>(foundDecl);
            auto structType = new StructType(Type::Qualifier::Unassigned, structDecl,
                                             expr->startPosition(), expr->endPosition());
            auto newExpr = new TypeExpr(structType);
            // Delete the old identifier
            delete expr;
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
            if (llvm::isa<StructDecl>(foundDecl->container) && foundDecl->container == _currentContainer) {
                // TODO: We need to account for whether or not the `currentFunction` is `static` or not.
                // If the variable is a member of a struct we have to create a reference to `self` for accessing it.
                Expr* selfRef = getCurrentSelfRef(expr->startPosition(), expr->endPosition());
                // Self mustn't be an lvalue, that would make it logically a double reference.
                selfRef = convertLValueToRValue(selfRef);
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

bool gulc::CodeProcessor::findMatchingDeclInContainer(gulc::Decl* container, std::string const& findName,
                                                      gulc::Decl** outFoundDecl, bool* outIsAmbiguous) {
    if (llvm::isa<NamespaceDecl>(container)) {
        auto checkNamespace = llvm::dyn_cast<NamespaceDecl>(container);

        return findMatchingDecl(checkNamespace->nestedDecls(), findName, outFoundDecl, outIsAmbiguous);
    } else if (llvm::isa<StructDecl>(container)) {
        auto checkStruct = llvm::dyn_cast<StructDecl>(container);

        return findMatchingDecl(checkStruct->allMembers, findName, outFoundDecl, outIsAmbiguous);
    } else if (llvm::isa<TraitDecl>(container)) {
        auto checkTrait = llvm::dyn_cast<TraitDecl>(container);

        return findMatchingDecl(checkTrait->allMembers, findName, outFoundDecl, outIsAmbiguous);
    } else {
        printError("[INTERNAL] unsupported container found in `CodeProcessor::findMatchingDeclInContainer`!",
                   container->startPosition(), container->endPosition());
        return false;
    }
}

bool gulc::CodeProcessor::findMatchingDecl(std::vector<Decl*> const& searchDecls, std::string const& findName,
                                           gulc::Decl** outFoundDecl, bool* outIsAmbiguous) {
    Decl* foundDecl = nullptr;
    bool isAmbiguous = false;

    for (Decl* checkDecl : searchDecls) {
        if (findName == checkDecl->identifier().name()) {
            if (foundDecl == nullptr) {
                foundDecl = checkDecl;
                isAmbiguous = false;
            } else {
                isAmbiguous = true;
                break;
            }
        }
    }

    if (foundDecl == nullptr) {
        return false;
    } else {
        *outFoundDecl = foundDecl;
        *outIsAmbiguous = isAmbiguous;
        return true;
    }
}

void gulc::CodeProcessor::processInfixOperatorExpr(gulc::InfixOperatorExpr*& infixOperatorExpr) {
    // NOTE: We need to make sure we check extensions as well. We will also need to check for any ambiguities on
    //       extensions. Two namespaces could define the same extension operator with the same signature causing
    //       ambiguities
    processExpr(infixOperatorExpr->leftValue);
    processExpr(infixOperatorExpr->rightValue);

    // Before we do anything else we need to handle getters. Not doing so will cause the application to crash as we
    // don't have type information until getters are applied.
    // TODO: Immediately after getters handle smart references. Smart references CANNOT have their own operator
    //       overloading.
    infixOperatorExpr->leftValue = handleGetter(infixOperatorExpr->leftValue);
    infixOperatorExpr->rightValue = handleGetter(infixOperatorExpr->rightValue);

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
            // If `left` is `mut` then find the `mut` operator or `immut` operator (if no `mut` operator exists)
            // If `left` is `immut` then only find `immut` operator
            OperatorDecl* foundOperator = nullptr;
            bool foundIsExact = false;
            bool isAmbiguous = false;

            if (matchingDecls.size() == 1) {
                foundOperator = llvm::dyn_cast<OperatorDecl>(matchingDecls[0].matchingDecl);
                foundIsExact = matchingDecls[0].kind == MatchingDecl::Kind::Match;
            } else {
                for (MatchingDecl& checkMatch : matchingDecls) {
                    auto checkOperator = llvm::dyn_cast<OperatorDecl>(checkMatch.matchingDecl);

                    // We skip any castables if we already found an exact match.
                    if (foundIsExact && checkMatch.kind == MatchingDecl::Kind::Castable) {
                        continue;
                    }

                    // If `foundOperator` is `mut` and `checkMatch` is `mut` we error for ambiguity
                    // (we immediately error since `mut` can never be upgraded)
                    // (NOTE FOR ABOVE: if we are `castable` we only mark for ambiguity, castable can upgrade to exact)
                    // If `foundOperator` is `mut` and `checkMatch` is `immut` we skip
                    // If `foundOperator` is `immut` and `checkMatch` is `mut` we replace `foundOperator`
                    // If `foundOperator` is `immut` and `checkMatch` is `immut` we mark for ambiguity
                    // (we only mark for ambiguity as `immut` can be upgraded to `mut` potentially)

                    if (foundOperator == nullptr) {
                        foundOperator = checkOperator;
                        foundIsExact = checkMatch.kind == MatchingDecl::Kind::Match;
                        continue;
                    }

                    if (foundOperator->isMutable()) {
                        if (checkOperator->isMutable()) {
                            isAmbiguous = true;

                            // If the `checkOperator` is an exact match we immediately break out of the loop for the
                            // error.
                            // If `checkOperator` is "Castable" we keep searching, there can be multiple "Castable" as
                            // long as there is a single exact match later...
                            if (checkMatch.kind == MatchingDecl::Kind::Match) {
                                foundOperator = checkOperator;
                                foundIsExact = true;
                                break;
                            }
                        }

                        // Skip. Check is `immut` so we don't care.
                    } else {
                        // `immut` can be replaced by `mut`
                        if (checkOperator->isMutable()) {
                            foundOperator = checkOperator;
                            foundIsExact = checkMatch.kind == MatchingDecl::Kind::Match;
                            // If there were multiple `immut` that doesn't matter, `mut` overrides that until we find
                            // multiple `mut`
                            isAmbiguous = false;
                            continue;
                        } else {
                            // Both are `immut` so we mark for ambiguity...
                            isAmbiguous = true;
                        }
                    }
                }
            }

            if (isAmbiguous) {
                printError("overloaded operator is ambiguous!",
                           infixOperatorExpr->startPosition(), infixOperatorExpr->endPosition());
            }

            if (foundOperator == nullptr) {
                printError("no overloaded operator found for the provided types!",
                           infixOperatorExpr->startPosition(), infixOperatorExpr->endPosition());
            }

            // TODO: Treat the rest like a normal function call (pass to the argument caster, etc.)

            // We handle argument casting and conversion no matter what. The below function will handle
            // converting from lvalue to rvalue, casting, and other rules for us.
            handleArgumentCasting(foundOperator->parameters()[0], infixOperatorExpr->rightValue);

            auto newMemberInfixOperatorCall = new MemberInfixOperatorCallExpr(
                    foundOperator,
                    infixOperatorExpr->infixOperator(),
                    infixOperatorExpr->leftValue,
                    infixOperatorExpr->rightValue
            );
            newMemberInfixOperatorCall->valueType = foundOperator->returnType->deepCopy();
            newMemberInfixOperatorCall->valueType->setIsLValue(true);
            // We steal these
            infixOperatorExpr->leftValue = nullptr;
            infixOperatorExpr->rightValue = nullptr;
            delete infixOperatorExpr;
            infixOperatorExpr = newMemberInfixOperatorCall;
            return;
        }
    }

    // Once we've reached this point we need to convert any lvalues to rvalues and dereference any implicit references
    infixOperatorExpr->leftValue = convertLValueToRValue(infixOperatorExpr->leftValue);
    infixOperatorExpr->rightValue = convertLValueToRValue(infixOperatorExpr->rightValue);
    // Dereference the references (if there are any references)
    infixOperatorExpr->leftValue = dereferenceReference(infixOperatorExpr->leftValue);
    infixOperatorExpr->rightValue = dereferenceReference(infixOperatorExpr->rightValue);

    auto leftType = infixOperatorExpr->leftValue->valueType;
    auto rightType = infixOperatorExpr->rightValue->valueType;

    switch (infixOperatorExpr->infixOperator()) {
        case InfixOperators::LogicalAnd: // &&
        case InfixOperators::LogicalOr: // ||
            // Once we've reached this point we need to convert any lvalues to rvalues and dereference any implicit references
            infixOperatorExpr->leftValue = handleGetter(infixOperatorExpr->leftValue);
            infixOperatorExpr->rightValue = handleGetter(infixOperatorExpr->rightValue);
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

    // TODO: Should this be `handleRefGetter` instead? Or even a special case to try
    //       `handleRefMutGetter -> handleRefGetter -> handleGetter`? My assumption would be that anyone using `.` on
    //       a property probably wants to use the actual reference to the value coming from the property rather than a
    //       copy of it.
    memberAccessCallExpr->objectRef = handleGetter(memberAccessCallExpr->objectRef);

    std::string const& findName = memberAccessCallExpr->member->identifier().name();

    if (memberAccessCallExpr->member->hasTemplateArguments()) {
        for (Expr*& templateArgument : memberAccessCallExpr->member->templateArguments()) {
            processExpr(templateArgument);
        }
    }

    // TODO: We will have to account for extensions
    // TODO: Support `NamespaceReferenceExpr` once it is created.
    if (llvm::isa<TypeExpr>(memberAccessCallExpr->objectRef)) {
        if (memberAccessCallExpr->isArrowCall()) {
            printError("operator `->` cannot be used on types!",
                       memberAccessCallExpr->startPosition(), memberAccessCallExpr->endPosition());
        }

        auto checkTypeExpr = llvm::dyn_cast<TypeExpr>(memberAccessCallExpr->objectRef);

        std::vector<MatchingTemplateDecl> matchingTemplates;
        // TODO: Search extensions
        bool isAmbiguous = false;
        Decl* foundDecl = nullptr;

        switch (checkTypeExpr->type->getTypeKind()) {
            case Type::Kind::Enum: {
                auto checkEnum = llvm::dyn_cast<EnumType>(checkTypeExpr->type)->decl();

                if (memberAccessCallExpr->member->hasTemplateArguments()) {
                    // NOTE: Enum `case` declarations cannot be templated so we only have to check `ownedMembers`
                    fillListOfMatchingTemplates(checkEnum->ownedMembers(), findName, true,
                                                memberAccessCallExpr->member->templateArguments(),
                                                matchingTemplates);
                } else {
                    for (EnumConstDecl* checkCase : checkEnum->enumConsts()) {
                        if (findName == checkCase->identifier().name()) {
                            foundDecl = checkCase;
                            break;
                        }
                    }

                    if (foundDecl == nullptr) {
                        foundDecl = findMatchingMemberDecl(checkEnum->ownedMembers(), findName,
                                                           true, &isAmbiguous);
                    }
                }
                break;
            }
            case Type::Kind::Struct: {
                auto checkStruct = llvm::dyn_cast<StructType>(checkTypeExpr->type)->decl();

                if (memberAccessCallExpr->member->hasTemplateArguments()) {
                    fillListOfMatchingTemplates(checkStruct->allMembers, findName, true,
                                                memberAccessCallExpr->member->templateArguments(),
                                                matchingTemplates);
                } else {
                    foundDecl = findMatchingMemberDecl(checkStruct->allMembers, findName,
                                                       true, &isAmbiguous);
                }

                break;
            }
            case Type::Kind::Trait: {
                auto checkTrait = llvm::dyn_cast<TraitType>(checkTypeExpr->type)->decl();

                if (memberAccessCallExpr->member->hasTemplateArguments()) {
                    fillListOfMatchingTemplates(checkTrait->allMembers, findName, true,
                                                memberAccessCallExpr->member->templateArguments(),
                                                matchingTemplates);
                } else {
                    foundDecl = findMatchingMemberDecl(checkTrait->allMembers, findName,
                                                       true, &isAmbiguous);
                }

                break;
            }
            default:
                break;
        }

        if (!matchingTemplates.empty()) {
            MatchingTemplateDecl* foundTemplate = nullptr;

            for (MatchingTemplateDecl& checkTemplate : matchingTemplates) {
                if (foundTemplate == nullptr) {
                    foundTemplate = &checkTemplate;
                    isAmbiguous = false;
                } else {
                    if (foundTemplate->kind == checkTemplate.kind) {
                        bool exactSame = true;
                        bool foundIsBetter = false;
                        // If this is true then we replace `found` with `check`
                        bool shouldReplaceFound = false;

                        // We break on first difference. If the first different argument is closer to zero then we
                        // replace `foundMatch`, if it was worse then we continue searching. If the arguments have the
                        // exact same strength then we set `isAmbiguous` but keep searching in case something better
                        // comes along.
                        for (std::size_t i = 0; i < foundTemplate->argMatchStrengths.size(); ++i) {
                            std::size_t foundStrength = foundTemplate->argMatchStrengths[i];
                            std::size_t checkStrength = checkTemplate.argMatchStrengths[i];

                            if (checkStrength < foundStrength) {
                                shouldReplaceFound = true;
                                exactSame = false;
                                break;
                            } else if (checkStrength > foundStrength) {
                                foundIsBetter = true;
                                exactSame = false;
                                break;
                            }
                        }

                        // If `foundIsBetter` is true then we have to skip the rest of the checks.
                        if (foundIsBetter) {
                            continue;
                        } else if (exactSame) {
                            isAmbiguous = true;
                        } else if (shouldReplaceFound) {
                            foundTemplate = &checkTemplate;
                            isAmbiguous = false;
                        }
                    } else if (foundTemplate->kind == MatchingDecl::Kind::Match) {
                        // Nothing is better than match, we only error above if check is also match
                    } else if (foundTemplate->kind == MatchingDecl::Kind::DefaultValues) {
                        // Match is always better than DefaultValues
                        if (checkTemplate.kind == MatchingDecl::Kind::Match) {
                            foundTemplate = &checkTemplate;
                            isAmbiguous = false;
                        }
                    } else {
                        // At this point `found` is `Castable` and `check` is `Match` or `DefaultValues`, both of which
                        // are better than `Castable`
                        foundTemplate = &checkTemplate;
                        isAmbiguous = false;
                    }
                }
            }

            if (foundTemplate == nullptr) {
                printError("template `" + memberAccessCallExpr->toString() + "` was not found!",
                           memberAccessCallExpr->startPosition(), memberAccessCallExpr->endPosition());
            }

            if (isAmbiguous) {
                printError("template `" + memberAccessCallExpr->toString() + "` is ambiguous!",
                           memberAccessCallExpr->startPosition(), memberAccessCallExpr->endPosition());
            }

            DeclInstantiator declInstantiator(_target, _filePaths);

            switch (foundTemplate->matchingDecl->getDeclKind()) {
                case Decl::Kind::TemplateStruct: {
                    auto templateStructDecl = llvm::dyn_cast<TemplateStructDecl>(foundTemplate->matchingDecl);

                    TemplateStructInstDecl* checkTemplateStructInst =
                            declInstantiator.instantiateTemplateStruct(_currentFile, templateStructDecl,
                                                                       memberAccessCallExpr->member->templateArguments(),
                                                                       memberAccessCallExpr->toString(),
                                                                       memberAccessCallExpr->startPosition(),
                                                                       memberAccessCallExpr->endPosition());

                    foundDecl = checkTemplateStructInst;

                    break;
                }
                case Decl::Kind::TemplateTrait: {
                    auto templateTraitDecl = llvm::dyn_cast<TemplateTraitDecl>(foundTemplate->matchingDecl);

                    TemplateTraitInstDecl* checkTemplateTraitInst =
                            declInstantiator.instantiateTemplateTrait(_currentFile, templateTraitDecl,
                                                                      memberAccessCallExpr->member->templateArguments(),
                                                                      memberAccessCallExpr->toString(),
                                                                      memberAccessCallExpr->startPosition(),
                                                                      memberAccessCallExpr->endPosition());

                    foundDecl = checkTemplateTraitInst;

                    break;
                }
                default:
                    printError("unknown template declaration reference!",
                               memberAccessCallExpr->startPosition(), memberAccessCallExpr->endPosition());
                    break;
            }
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

        if (memberAccessCallExpr->member->hasTemplateArguments()) {
            // Doing `var.Example<T>` is not allowed, you have to do `Type.Example<T>`. There is nothing that
            // can be templated on instance variables
            printError("cannot access templates from instance variables! (use direct type instead)",
                       memberAccessCallExpr->startPosition(), memberAccessCallExpr->endPosition());
        }

        // TODO: Search extensions
        bool isAmbiguous = false;
        Decl* foundDecl = nullptr;

        switch (checkType->getTypeKind()) {
            case Type::Kind::Enum: {
                auto checkEnum = llvm::dyn_cast<EnumType>(checkType)->decl();

                for (EnumConstDecl* checkCase : checkEnum->enumConsts()) {
                    if (findName == checkCase->identifier().name()) {
                        foundDecl = checkCase;
                        break;
                    }
                }

                if (foundDecl == nullptr) {
                    foundDecl = findMatchingMemberDecl(checkEnum->ownedMembers(), findName,
                                                       false, &isAmbiguous);
                }

                break;
            }
            case Type::Kind::Struct: {
                auto checkStruct = llvm::dyn_cast<StructType>(checkType)->decl();

                foundDecl = findMatchingMemberDecl(checkStruct->allMembers, findName,
                                                   false, &isAmbiguous);

                break;
            }
            case Type::Kind::Trait: {
                auto checkTrait = llvm::dyn_cast<TraitType>(checkType)->decl();

                foundDecl = findMatchingMemberDecl(checkTrait->allMembers, findName,
                                                   false, &isAmbiguous);

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
            printError("type `" + checkType->toString() + "` does not contain a member `" + memberAccessCallExpr->member->toString() + "`!",
                       memberAccessCallExpr->startPosition(), memberAccessCallExpr->endPosition());
        }

        memberAccessCallExpr->objectRef = handleGetter(memberAccessCallExpr->objectRef);

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
                                                        std::string const& findName,
                                                        bool searchForStatic, bool* outIsAmbiguous) {
    Decl* foundDecl = nullptr;

    // These will ONLY be templates, if a non-template matches then we set `foundDecl` directly
    std::vector<MatchingDecl> potentialMatches;

    for (Decl* checkDecl : searchDecls) {
        if (checkDecl->isStatic() == searchForStatic && checkDecl->identifier().name() == findName) {
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

void gulc::CodeProcessor::processMemberSubscriptOperatorRefExpr(
        gulc::MemberSubscriptOperatorRefExpr* memberSubscriptOperatorRefExpr) {
    // NOTE: Ditto to the above `processMemberPropertyRefExpr`.
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
        // We steal this
        postfixOperatorExpr->nestedExpr = nullptr;
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
    postfixOperatorExpr->nestedExpr = handleGetter(postfixOperatorExpr->nestedExpr);
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
        // We steal this
        prefixOperatorExpr->nestedExpr = nullptr;
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
            prefixOperatorExpr->nestedExpr = handleGetter(prefixOperatorExpr->nestedExpr);
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
            prefixOperatorExpr->nestedExpr = handleGetter(prefixOperatorExpr->nestedExpr);
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
            prefixOperatorExpr->nestedExpr = handleGetter(prefixOperatorExpr->nestedExpr);
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
            prefixOperatorExpr->nestedExpr = handleGetter(prefixOperatorExpr->nestedExpr);
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

    if (llvm::isa<PropertyRefExpr>(refExpr->nestedExpr)) {
        refExpr->nestedExpr = handleRefGetter(llvm::dyn_cast<PropertyRefExpr>(refExpr->nestedExpr),
                                              refExpr->isMutable);
        refExpr->valueType = refExpr->nestedExpr->valueType->deepCopy();
    } else if (llvm::isa<SubscriptOperatorRefExpr>(refExpr->nestedExpr)) {
        refExpr->nestedExpr = handleRefGetter(llvm::dyn_cast<SubscriptOperatorRefExpr>(refExpr->nestedExpr),
                                              refExpr->isMutable);
        refExpr->valueType = refExpr->nestedExpr->valueType->deepCopy();
    } else {
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
}

void gulc::CodeProcessor::processSubscriptCallExpr(gulc::Expr*& expr) {
    auto subscriptCallExpr = llvm::dyn_cast<SubscriptCallExpr>(expr);

    processExpr(subscriptCallExpr->subscriptReference);

    for (LabeledArgumentExpr* argument : subscriptCallExpr->arguments) {
        processLabeledArgumentExpr(argument);
    }

    // TODO: Support searching extensions
    // TODO: Support flat array and pointers
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

        auto newSubscriptOperatorRef = new SubscriptOperatorRefExpr(
                subscriptCallExpr->startPosition(),
                subscriptCallExpr->endPosition(),
                foundSubscriptOperator,
                subscriptCallExpr->arguments
        );
        processSubscriptOperatorRefExpr(newSubscriptOperatorRef);
        // We steal these.
        subscriptCallExpr->arguments.clear();
        // Replace the old `subscriptCallExpr` with the overloaded `SubscriptOperatorRefExpr` call
        delete subscriptCallExpr;
        expr = newSubscriptOperatorRef;
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
                                                                       false, &isAmbiguous);
                break;
            }
            case Type::Kind::Trait: {
                auto traitType = llvm::dyn_cast<TraitType>(searchType);
                auto searchTrait = traitType->decl();

                foundSubscriptOperator = findMatchingSubscriptOperator(searchTrait->allMembers,
                                                                       subscriptCallExpr->arguments,
                                                                       false, &isAmbiguous);
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

        auto newMemberSubscriptOperatorRef = new MemberSubscriptOperatorRefExpr(
                subscriptCallExpr->startPosition(),
                subscriptCallExpr->endPosition(),
                foundSubscriptOperator,
                subscriptCallExpr->subscriptReference,
                subscriptCallExpr->arguments
        );
        processMemberSubscriptOperatorRefExpr(newMemberSubscriptOperatorRef);
        // We steal these
        subscriptCallExpr->subscriptReference = nullptr;
        subscriptCallExpr->arguments.clear();
        // Replace the old `subscriptCallExpr` with the overloaded `SubscriptOperatorRefExpr` call
        delete subscriptCallExpr;
        expr = newMemberSubscriptOperatorRef;
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

void gulc::CodeProcessor::processSubscriptOperatorRefExpr(gulc::SubscriptOperatorRefExpr* subscriptOperatorRefExpr) {
    // NOTE: See `processMemberSubscriptOperatorRefExpr` for notes on why this is empty.
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

    ternaryExpr->trueExpr = handleGetter(ternaryExpr->trueExpr);
    ternaryExpr->falseExpr = handleGetter(ternaryExpr->falseExpr);

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

        variableDeclExpr->initialValue = handleGetter(variableDeclExpr->initialValue);

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
            variableDeclExpr->initialValue = handleGetter(variableDeclExpr->initialValue);
            variableDeclExpr->initialValue = convertLValueToRValue(variableDeclExpr->initialValue);
        }

        if (variableDeclExpr->type == nullptr) {
            // If the type wasn't explicitly set then we infer the type from the value type of the initial value
            variableDeclExpr->type = variableDeclExpr->initialValue->valueType->deepCopy();
        }
    }

    // If the variable is `let mut` we also set the type to `mut`
    if (variableDeclExpr->isAssignable()) {
        variableDeclExpr->type->setQualifier(Type::Qualifier::Mut);
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
            if (handleArgumentCasting(parameters[i], arguments[i]->argument)) {
                delete arguments[i]->valueType;
                arguments[i]->valueType = arguments[i]->argument->valueType->deepCopy();
            }
        }
    }
}

bool gulc::CodeProcessor::handleArgumentCasting(gulc::ParameterDecl* parameter, gulc::Expr*& argument) {
    if (llvm::isa<ReferenceType>(parameter->type)) {
        if (argument->valueType->isLValue()) {
            // If the argument is an lvalue to a reference then we convert the lvalue to an rvalue, keeping the
            // implicit reference.
            argument = convertLValueToRValue(argument);
            return true;
        }
    } else if (parameter->parameterKind() == ParameterDecl::ParameterKind::In) {
        // TODO: We should make it possible to pass `in` parameters by value when the
        //       `sizeof(param.type) <= sizeof(ptr)`
        //       Passing by reference is only more efficient when structs are large.
        //       There should be a bool part of `ParameterDecl` that says `passInByRef` or something that when
        //       false will treat the `in` as by-value instead of by-ref (useful for `in i32`, `in bool`, etc.)
        if (llvm::isa<ReferenceType>(parameter->type)) {
            // If the parameter is a reference then the argument must either be a double reference or an lvalue
            // + reference (NOTE: I don't know why we would allow `in ref <type>`? But I'll try to support it
            // nonetheless.
            if (argument->valueType->isLValue()) {
                // If it is a `ref ref` we convert lvalue to rvalue, else we keep the lvalue as we need the
                // double reference to allow `in`...
                if (llvm::isa<ReferenceType>(argument->valueType)) {
                    auto checkNested =
                            llvm::dyn_cast<ReferenceType>(argument->valueType)->nestedType;

                    if (llvm::isa<ReferenceType>(argument->valueType)) {
                        argument = convertLValueToRValue(argument);
                        return true;
                    }
                }
            }
        } else {
            // If argument is a reference we need to convert lvalue to rvalue
            // If argument is not a reference then we need to make sure it is an lvalue
            // If argument is not an lvalue then we need to create a temporary value to store it so we can have
            // a reference for it.
            if (llvm::isa<ReferenceType>(argument->valueType)) {
                argument = convertLValueToRValue(argument);
                return true;
            } else if (!argument->valueType->isLValue()) {
                if (llvm::isa<StructType>(argument->valueType)) {
                    // To prevent potential memory leaks I'm putting this here. If we ever encounter a scenario
                    // where a struct is being converted from rvalue to `in` ref then I'll handle it.
                    printError("[INTERNAL] struct rvalue being passed to `in` parameter not supported!",
                               argument->startPosition(), argument->endPosition());
                }

                // Here we use `RValueToInRefExpr` to force the code generator to create a reference from the
                // rvalue.
                // TODO: Originally I was thinking we would do this to prevent a temporary value cleanup
                //       But now I'm thinking we actually SHOULD either `copy` or `move` the rvalue to a
                //       temporary and have it get cleaned up...
                auto rvalueToInRef = new RValueToInRefExpr(argument);
                rvalueToInRef->valueType = argument->valueType->deepCopy();
                rvalueToInRef->valueType->setIsLValue(true);
                argument = rvalueToInRef;
                return true;
            }
        }
    } else if (parameter->parameterKind() == ParameterDecl::ParameterKind::Out) {
        // If argument is a reference we need to convert lvalue to rvalue
        // If argument is not a reference then we need to make sure it is an lvalue
        // The argument MUST be `mut`
        if (llvm::isa<ReferenceType>(argument->valueType)) {
            if (llvm::dyn_cast<ReferenceType>(argument->valueType)->nestedType->qualifier()
                != Type::Qualifier::Mut) {
                printError("`out` parameters require their arguments to be `mut`!",
                           argument->startPosition(), argument->endPosition());
            }

            argument = convertLValueToRValue(argument);
            return true;
        } else if (!argument->valueType->isLValue()) {
            printError("`out` parameters require their argument to either be an lvalue or `ref mut`!",
                       argument->startPosition(), argument->endPosition());
        } else if (argument->valueType->qualifier() != Type::Qualifier::Mut) {
            printError("`out` parameters require their arguments to be `mut`!",
                       argument->startPosition(), argument->endPosition());
        }
    } else {
        // The parameter is a value type so we remove any `lvalue` and dereference any implicit references.
        argument = convertLValueToRValue(argument);
        // TODO: Dereference any references as well.
        return true;
    }

    return false;
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

gulc::Expr* gulc::CodeProcessor::handleGetter(gulc::Expr* potentialGetSet) const {
    if (llvm::isa<MemberPropertyRefExpr>(potentialGetSet)) {
        auto memberPropertyRefExpr = llvm::dyn_cast<MemberPropertyRefExpr>(potentialGetSet);

        // For the `MemberPropertyRefExpr` we have to account for mutability.
        // If the `object` is `mut` then we can either grab a `mut` getter OR an `immut` getter if the `mut` getter
        // doesn't exist.
        // If the `object` is `immut` then we can ONLY grab the `immut` getter
        PropertyGetDecl* foundGetter = nullptr;
        bool findMutGetter = memberPropertyRefExpr->object->valueType->qualifier() == Type::Qualifier::Mut;

        for (PropertyGetDecl* checkGetter : memberPropertyRefExpr->propertyDecl->getters()) {
            // We're only looking for normal getters, not `ref` getters.
            if (checkGetter->getResultType() != PropertyGetDecl::GetResult::Normal) continue;

            if (checkGetter->isMutable()) {
                // If we can't use `mut` getters we skip this one.
                if (!findMutGetter) continue;

                // If we're searching for the `mut` getter and this getter is `mut` we need to check if the existing
                // getter is `mut` or not. If the existing found getter IS `mut` we have to error for ambiguity. Else
                // we just replace it.
                // This should never happen as a previous pass should detect this. But just in case.
                if (foundGetter != nullptr && foundGetter->isMutable()) {
                    printError("property call is ambiguous, there are multiple `mut get` declarations!",
                               potentialGetSet->startPosition(), potentialGetSet->endPosition());
                } else {
                    // At this point the found getter is either null or immut. Either way we set it to the "stronger"
                    // `mut` getter
                    foundGetter = checkGetter;
                }
            } else {
                // If the found getter is `mut` then we skip. A `mut` getter cannot be replaced by an `immut` getter.
                if (foundGetter != nullptr && foundGetter->isMutable()) continue;

                // If the found getter is `immut` and not null then we have to error
                if (foundGetter != nullptr) {
                    printError("property call is ambiguous, there are multiple `get` declarations!",
                               potentialGetSet->startPosition(), potentialGetSet->endPosition());
                }

                foundGetter = checkGetter;
            }
        }

        if (foundGetter == nullptr) {
            printError("property does not have a valid getter!",
                       potentialGetSet->startPosition(), potentialGetSet->endPosition());
        }

        auto result = new PropertyGetCallExpr(memberPropertyRefExpr, foundGetter);
        result->valueType = foundGetter->returnType->deepCopy();
        result->valueType->setIsLValue(true);
        return result;
    } else if (llvm::isa<PropertyRefExpr>(potentialGetSet)) {
        auto propertyRefExpr = llvm::dyn_cast<PropertyRefExpr>(potentialGetSet);

        // We can use the non-ref getter. If there is more than one we error (this should never actually trigger, a
        // previous pass should handle this)
        PropertyGetDecl* foundGetter = nullptr;

        for (PropertyGetDecl* checkGetter : propertyRefExpr->propertyDecl->getters()) {
            if (checkGetter->getResultType() == PropertyGetDecl::GetResult::Normal) {
                if (foundGetter == nullptr) {
                    foundGetter = checkGetter;
                } else {
                    printError("property call is ambiguous, there are multiple valid `get` blocks!",
                               potentialGetSet->startPosition(), potentialGetSet->endPosition());
                }
            }
        }

        if (foundGetter == nullptr) {
            printError("property does not have a valid getter!",
                       potentialGetSet->startPosition(), potentialGetSet->endPosition());
        }

        auto result = new PropertyGetCallExpr(propertyRefExpr, foundGetter);
        result->valueType = foundGetter->returnType->deepCopy();
        result->valueType->setIsLValue(true);
        return result;
    } else if (llvm::isa<MemberSubscriptOperatorRefExpr>(potentialGetSet)) {
        auto memberSubscriptOperatorRef = llvm::dyn_cast<MemberSubscriptOperatorRefExpr>(potentialGetSet);

        // For the `MemberSubscriptOperatorRefExpr` we have to account for mutability.
        // If the `object` is `mut` then we can either grab a `mut` getter OR an `immut` getter if the `mut` getter
        // doesn't exist.
        // If the `object` is `immut` then we can ONLY grab the `immut` getter
        SubscriptOperatorGetDecl* foundGetter = nullptr;
        bool findMutGetter = memberSubscriptOperatorRef->object->valueType->qualifier() == Type::Qualifier::Mut;

        for (SubscriptOperatorGetDecl* checkGetter : memberSubscriptOperatorRef->subscriptOperatorDecl->getters()) {
            // We're only looking for normal getters, not `ref` getters.
            if (checkGetter->getResultType() != SubscriptOperatorGetDecl::GetResult::Normal) continue;

            if (checkGetter->isMutable()) {
                // If we can't use `mut` getters we skip this one.
                if (!findMutGetter) continue;

                // If we're searching for the `mut` getter and this getter is `mut` we need to check if the existing
                // getter is `mut` or not. If the existing found getter IS `mut` we have to error for ambiguity. Else
                // we just replace it.
                // This should never happen as a previous pass should detect this. But just in case.
                if (foundGetter != nullptr && foundGetter->isMutable()) {
                    printError("subscript call is ambiguous, there are multiple `mut get` declarations!",
                               potentialGetSet->startPosition(), potentialGetSet->endPosition());
                } else {
                    // At this point the found getter is either null or immut. Either way we set it to the "stronger"
                    // `mut` getter
                    foundGetter = checkGetter;
                }
            } else {
                // If the found getter is `mut` then we skip. A `mut` getter cannot be replaced by an `immut` getter.
                if (foundGetter != nullptr && foundGetter->isMutable()) continue;

                // If the found getter is `immut` and not null then we have to error
                if (foundGetter != nullptr) {
                    printError("subscript call is ambiguous, there are multiple `get` declarations!",
                               potentialGetSet->startPosition(), potentialGetSet->endPosition());
                }

                foundGetter = checkGetter;
            }
        }

        if (foundGetter == nullptr) {
            printError("subscript does not have a valid getter!",
                       potentialGetSet->startPosition(), potentialGetSet->endPosition());
        }

        auto result = new SubscriptOperatorGetCallExpr(memberSubscriptOperatorRef, foundGetter);
        result->valueType = foundGetter->returnType->deepCopy();
        result->valueType->setIsLValue(true);
        return result;
    } else if (llvm::isa<SubscriptOperatorRefExpr>(potentialGetSet)) {
        auto subscriptOperatorRefExpr = llvm::dyn_cast<SubscriptOperatorRefExpr>(potentialGetSet);

        // We can use the non-ref getter. If there is more than one we error (this should never actually trigger, a
        // previous pass should handle this)
        SubscriptOperatorGetDecl* foundGetter = nullptr;

        for (SubscriptOperatorGetDecl* checkGetter : subscriptOperatorRefExpr->subscriptOperatorDecl->getters()) {
            if (checkGetter->getResultType() == SubscriptOperatorGetDecl::GetResult::Normal) {
                if (foundGetter == nullptr) {
                    foundGetter = checkGetter;
                } else {
                    printError("subscript call is ambiguous, there are multiple valid `get` blocks!",
                               potentialGetSet->startPosition(), potentialGetSet->endPosition());
                }
            }
        }

        if (foundGetter == nullptr) {
            printError("subscript does not have a valid getter!",
                       potentialGetSet->startPosition(), potentialGetSet->endPosition());
        }

        auto result = new SubscriptOperatorGetCallExpr(subscriptOperatorRefExpr, foundGetter);
        result->valueType = foundGetter->returnType->deepCopy();
        result->valueType->setIsLValue(true);
        return result;
    } else {
        return potentialGetSet;
    }
}

gulc::Expr* gulc::CodeProcessor::handleRefGetter(gulc::PropertyRefExpr* propertyRefExpr, bool isRefMut) const {
    if (llvm::isa<MemberPropertyRefExpr>(propertyRefExpr)) {
        auto memberPropertyRefExpr = llvm::dyn_cast<MemberPropertyRefExpr>(propertyRefExpr);

        // For the `MemberPropertyRefExpr` we have to account for mutability.
        // If the `object` is `mut` then we can either grab a `mut` getter OR an `immut` getter if the `mut` getter
        // doesn't exist.
        // If the `object` is `immut` then we can ONLY grab the `immut` getter
        PropertyGetDecl* foundGetter = nullptr;
        bool findMutGetter = memberPropertyRefExpr->object->valueType->qualifier() == Type::Qualifier::Mut;

        for (PropertyGetDecl* checkGetter : memberPropertyRefExpr->propertyDecl->getters()) {
            auto checkResultType = checkGetter->getResultType();

            if (isRefMut) {
                // We can ONLY support `ref mut` getters
                if (checkResultType != PropertyGetDecl::GetResult::RefMut) continue;

                if (checkGetter->isMutable()) {
                    // TODO: Is it possible for an immut `ref mut`? I think so...
                    if (!findMutGetter) continue;

                    // If we're searching for the `mut` getter and this getter is `mut` we need to check if the existing
                    // getter is `mut` or not. If the existing found getter IS `mut` we have to error for ambiguity.
                    // Else we just replace it.
                    // This should never happen as a previous pass should detect this. But just in case.
                    if (foundGetter != nullptr && foundGetter->isMutable()) {
                        printError("property call is ambiguous, there are multiple `mut get` declarations!",
                                   propertyRefExpr->startPosition(), propertyRefExpr->endPosition());
                    } else {
                        // At this point the found getter is either null or immut. Either way we set it to the "stronger"
                        // `mut` getter
                        foundGetter = checkGetter;
                    }
                } else {
                    // If the found getter is `mut` then we skip. A `mut` getter cannot be replaced by an `immut`
                    // getter.
                    if (foundGetter != nullptr && foundGetter->isMutable()) continue;

                    // If the found getter is `immut` and not null then we have to error
                    if (foundGetter != nullptr) {
                        printError("property call is ambiguous, there are multiple `get` declarations!",
                                   propertyRefExpr->startPosition(), propertyRefExpr->endPosition());
                    }

                    foundGetter = checkGetter;
                }
            } else {
                // We can support either `ref mut` or `ref`
                if (checkResultType == PropertyGetDecl::GetResult::Normal) continue;

                // If the object is `immut` but the getter is `mut` then we skip the getter.
                if (!findMutGetter && checkGetter->isMutable()) continue;

                // For `ref` we favor `get ref` BUT if `get ref` doesn't exist and `get ref mut` does we will still
                // allow `get ref mut` as a backup.
                if (checkResultType == PropertyGetDecl::GetResult::Ref) {
                    if (foundGetter != nullptr) {
                        // If the found is `ref mut` we replace it with `ref`
                        if (foundGetter->getResultType() == PropertyGetDecl::GetResult::RefMut) {
                            foundGetter = checkGetter;
                            continue;
                        }

                        // If the found is `immut` but check is `mut` we replace found, if check is `mut` at this point
                        // that means we're looking for a `mut` where possible.
                        if (!foundGetter->isMutable() && checkGetter->isMutable()) {
                            foundGetter = checkGetter;
                            continue;
                        }

                        // At this point both `resultType` and `mutability` is identical, this is an ambiguous call
                        // (NOTE: This should never happen, a previous `prop` validator should detect this and error)
                        printError("property call is ambiguous, there are multiple `get ref` declarations!",
                                   propertyRefExpr->startPosition(), propertyRefExpr->endPosition());
                    } else {
                        // This is the first found getter, we immediately set but keep looking
                        foundGetter = checkGetter;
                        continue;
                    }
                } else if (checkResultType == PropertyGetDecl::GetResult::RefMut) {
                    if (foundGetter != nullptr) {
                        // If the getter we already found is a `ref` we skip (as it is exactly what the programmer wants,
                        // `ref mut` would be a fallback here)
                        if (foundGetter->getResultType() == PropertyGetDecl::GetResult::Ref) {
                            continue;
                        }

                        // If the found is `immut` but check is `mut` we replace found, if check is `mut` at this point
                        // that means we're looking for a `mut` where possible.
                        if (!foundGetter->isMutable() && checkGetter->isMutable()) {
                            foundGetter = checkGetter;
                            continue;
                        }

                        // At this point both `resultType` and `mutability` is identical, this is an ambiguous call
                        // (NOTE: This should never happen, a previous `prop` validator should detect this and error)
                        printError("property call is ambiguous, there are multiple `get ref` declarations!",
                                   propertyRefExpr->startPosition(), propertyRefExpr->endPosition());
                    } else {
                        // This is the first found getter, we immediately set but keep looking
                        foundGetter = checkGetter;
                        continue;
                    }
                }
            }
        }

        if (foundGetter == nullptr) {
            printError("property does not have a valid `ref` getter!",
                       propertyRefExpr->startPosition(), propertyRefExpr->endPosition());
        }

        auto result = new PropertyGetCallExpr(memberPropertyRefExpr, foundGetter);
        result->valueType = foundGetter->returnType->deepCopy();
        result->valueType->setIsLValue(true);
        return result;
    } else {
        PropertyGetDecl* foundGetter = nullptr;

        for (PropertyGetDecl* checkGetter : propertyRefExpr->propertyDecl->getters()) {
            auto checkResultType = checkGetter->getResultType();

            if (isRefMut) {
                // We can ONLY support `ref mut` getters
                if (checkResultType != PropertyGetDecl::GetResult::RefMut) continue;

                // For global properties we don't have to check if the getter is `mut get` as `mut get` doesn't exist
                // outside of objects.
                if (foundGetter != nullptr) {
                    printError("property call is ambiguous, there are multiple `get ref mut` declarations!",
                               propertyRefExpr->startPosition(), propertyRefExpr->endPosition());
                }

                foundGetter = checkGetter;
            } else {
                // We can support either `ref mut` or `ref`
                if (checkResultType == PropertyGetDecl::GetResult::Normal) continue;

                // First found result, it can be either `ref` or `ref mut` so we set it and continue searching just in
                // case there are multiple.
                if (foundGetter == nullptr) {
                    foundGetter = checkGetter;
                    continue;
                }

                // If the found is `ref` but check is `ref mut` then we skip the check.
                if (foundGetter->getResultType() == PropertyGetDecl::GetResult::Ref) {
                    // If `checkGetter` is also `ref` then we error on ambiguity, if `checkGetter` is `ref mut` we skip
                    if (checkResultType == PropertyGetDecl::GetResult::Ref) {
                        printError("property call is ambiguous, there are multiple `get ref` declarations!",
                                   propertyRefExpr->startPosition(), propertyRefExpr->endPosition());
                    }
                } else {
                    // If `foundGetter` is `ref mut` we can replace it if `checkGetter` is only `ref`. Else we error on
                    // ambiguity
                    if (checkResultType == PropertyGetDecl::GetResult::RefMut) {
                        printError("property call is ambiguous, there are multiple `get ref mut` declarations!",
                                   propertyRefExpr->startPosition(), propertyRefExpr->endPosition());
                    }

                    foundGetter = checkGetter;
                    continue;
                }
            }
        }

        if (foundGetter == nullptr) {
            printError("property does not have a valid `ref` getter!",
                       propertyRefExpr->startPosition(), propertyRefExpr->endPosition());
        }

        auto result = new PropertyGetCallExpr(propertyRefExpr, foundGetter);
        result->valueType = foundGetter->returnType->deepCopy();
        result->valueType->setIsLValue(true);
        return result;
    }
}

gulc::Expr* gulc::CodeProcessor::handleRefGetter(gulc::SubscriptOperatorRefExpr* subscriptOperatorRefExpr,
                                                 bool isRefMut) const {
    if (llvm::isa<MemberSubscriptOperatorRefExpr>(subscriptOperatorRefExpr)) {
        auto memberSubscriptOperatorRefExpr = llvm::dyn_cast<MemberSubscriptOperatorRefExpr>(subscriptOperatorRefExpr);

        // For the `MemberSubscriptOperatorRefExpr` we have to account for mutability.
        // If the `object` is `mut` then we can either grab a `mut` getter OR an `immut` getter if the `mut` getter
        // doesn't exist.
        // If the `object` is `immut` then we can ONLY grab the `immut` getter
        SubscriptOperatorGetDecl* foundGetter = nullptr;
        bool findMutGetter = memberSubscriptOperatorRefExpr->object->valueType->qualifier() == Type::Qualifier::Mut;

        for (SubscriptOperatorGetDecl* checkGetter : memberSubscriptOperatorRefExpr->subscriptOperatorDecl->getters()) {
            auto checkResultType = checkGetter->getResultType();

            if (isRefMut) {
                // We can ONLY support `ref mut` getters
                if (checkResultType != SubscriptOperatorGetDecl::GetResult::RefMut) continue;

                if (checkGetter->isMutable()) {
                    // TODO: Is it possible for an immut `ref mut`? I think so...
                    if (!findMutGetter) continue;

                    // If we're searching for the `mut` getter and this getter is `mut` we need to check if the existing
                    // getter is `mut` or not. If the existing found getter IS `mut` we have to error for ambiguity.
                    // Else we just replace it.
                    // This should never happen as a previous pass should detect this. But just in case.
                    if (foundGetter != nullptr && foundGetter->isMutable()) {
                        printError("subscript call is ambiguous, there are multiple `mut get` declarations!",
                                   subscriptOperatorRefExpr->startPosition(), subscriptOperatorRefExpr->endPosition());
                    } else {
                        // At this point the found getter is either null or immut. Either way we set it to the "stronger"
                        // `mut` getter
                        foundGetter = checkGetter;
                    }
                } else {
                    // If the found getter is `mut` then we skip. A `mut` getter cannot be replaced by an `immut`
                    // getter.
                    if (foundGetter != nullptr && foundGetter->isMutable()) continue;

                    // If the found getter is `immut` and not null then we have to error
                    if (foundGetter != nullptr) {
                        printError("subscript call is ambiguous, there are multiple `get` declarations!",
                                   subscriptOperatorRefExpr->startPosition(), subscriptOperatorRefExpr->endPosition());
                    }

                    foundGetter = checkGetter;
                }
            } else {
                // We can support either `ref mut` or `ref`
                if (checkResultType == SubscriptOperatorGetDecl::GetResult::Normal) continue;

                // If the object is `immut` but the getter is `mut` then we skip the getter.
                if (!findMutGetter && checkGetter->isMutable()) continue;

                // For `ref` we favor `get ref` BUT if `get ref` doesn't exist and `get ref mut` does we will still
                // allow `get ref mut` as a backup.
                if (checkResultType == SubscriptOperatorGetDecl::GetResult::Ref) {
                    if (foundGetter != nullptr) {
                        // If the found is `ref mut` we replace it with `ref`
                        if (foundGetter->getResultType() == SubscriptOperatorGetDecl::GetResult::RefMut) {
                            foundGetter = checkGetter;
                            continue;
                        }

                        // If the found is `immut` but check is `mut` we replace found, if check is `mut` at this point
                        // that means we're looking for a `mut` where possible.
                        if (!foundGetter->isMutable() && checkGetter->isMutable()) {
                            foundGetter = checkGetter;
                            continue;
                        }

                        // At this point both `resultType` and `mutability` is identical, this is an ambiguous call
                        // (NOTE: This should never happen, a previous `prop` validator should detect this and error)
                        printError("subscript call is ambiguous, there are multiple `get ref` declarations!",
                                   subscriptOperatorRefExpr->startPosition(), subscriptOperatorRefExpr->endPosition());
                    } else {
                        // This is the first found getter, we immediately set but keep looking
                        foundGetter = checkGetter;
                        continue;
                    }
                } else if (checkResultType == SubscriptOperatorGetDecl::GetResult::RefMut) {
                    if (foundGetter != nullptr) {
                        // If the getter we already found is a `ref` we skip (as it is exactly what the programmer wants,
                        // `ref mut` would be a fallback here)
                        if (foundGetter->getResultType() == SubscriptOperatorGetDecl::GetResult::Ref) {
                            continue;
                        }

                        // If the found is `immut` but check is `mut` we replace found, if check is `mut` at this point
                        // that means we're looking for a `mut` where possible.
                        if (!foundGetter->isMutable() && checkGetter->isMutable()) {
                            foundGetter = checkGetter;
                            continue;
                        }

                        // At this point both `resultType` and `mutability` is identical, this is an ambiguous call
                        // (NOTE: This should never happen, a previous `prop` validator should detect this and error)
                        printError("subscript call is ambiguous, there are multiple `get ref` declarations!",
                                   subscriptOperatorRefExpr->startPosition(), subscriptOperatorRefExpr->endPosition());
                    } else {
                        // This is the first found getter, we immediately set but keep looking
                        foundGetter = checkGetter;
                        continue;
                    }
                }
            }
        }

        if (foundGetter == nullptr) {
            printError("subscript does not have a valid `ref` getter!",
                       subscriptOperatorRefExpr->startPosition(), subscriptOperatorRefExpr->endPosition());
        }

        auto result = new SubscriptOperatorGetCallExpr(memberSubscriptOperatorRefExpr, foundGetter);
        result->valueType = foundGetter->returnType->deepCopy();
        result->valueType->setIsLValue(true);
        return result;
    } else {
        SubscriptOperatorGetDecl* foundGetter = nullptr;

        for (SubscriptOperatorGetDecl* checkGetter : subscriptOperatorRefExpr->subscriptOperatorDecl->getters()) {
            auto checkResultType = checkGetter->getResultType();

            if (isRefMut) {
                // We can ONLY support `ref mut` getters
                if (checkResultType != SubscriptOperatorGetDecl::GetResult::RefMut) continue;

                // For global properties we don't have to check if the getter is `mut get` as `mut get` doesn't exist
                // outside of objects.
                if (foundGetter != nullptr) {
                    printError("subscript call is ambiguous, there are multiple `get ref mut` declarations!",
                               subscriptOperatorRefExpr->startPosition(), subscriptOperatorRefExpr->endPosition());
                }

                foundGetter = checkGetter;
            } else {
                // We can support either `ref mut` or `ref`
                if (checkResultType == SubscriptOperatorGetDecl::GetResult::Normal) continue;

                // First found result, it can be either `ref` or `ref mut` so we set it and continue searching just in
                // case there are multiple.
                if (foundGetter == nullptr) {
                    foundGetter = checkGetter;
                    continue;
                }

                // If the found is `ref` but check is `ref mut` then we skip the check.
                if (foundGetter->getResultType() == SubscriptOperatorGetDecl::GetResult::Ref) {
                    // If `checkGetter` is also `ref` then we error on ambiguity, if `checkGetter` is `ref mut` we skip
                    if (checkResultType == SubscriptOperatorGetDecl::GetResult::Ref) {
                        printError("subscript call is ambiguous, there are multiple `get ref` declarations!",
                                   subscriptOperatorRefExpr->startPosition(), subscriptOperatorRefExpr->endPosition());
                    }
                } else {
                    // If `foundGetter` is `ref mut` we can replace it if `checkGetter` is only `ref`. Else we error on
                    // ambiguity
                    if (checkResultType == SubscriptOperatorGetDecl::GetResult::RefMut) {
                        printError("subscript call is ambiguous, there are multiple `get ref mut` declarations!",
                                   subscriptOperatorRefExpr->startPosition(), subscriptOperatorRefExpr->endPosition());
                    }

                    foundGetter = checkGetter;
                    continue;
                }
            }
        }

        if (foundGetter == nullptr) {
            printError("subscript does not have a valid `ref` getter!",
                       subscriptOperatorRefExpr->startPosition(), subscriptOperatorRefExpr->endPosition());
        }

        auto result = new SubscriptOperatorGetCallExpr(subscriptOperatorRefExpr, foundGetter);
        result->valueType = foundGetter->returnType->deepCopy();
        result->valueType->setIsLValue(true);
        return result;
    }
}
