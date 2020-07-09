#include <iostream>
#include <ast/types/DependentType.hpp>
#include <ast/types/StructType.hpp>
#include <ast/types/TemplateStructType.hpp>
#include "CodeProcessor.hpp"
#include <utilities/SignatureComparer.hpp>
#include <ast/exprs/ConstructorCallExpr.hpp>
#include <ast/types/TraitType.hpp>
#include <ast/types/TemplateTraitType.hpp>

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
    for (ParameterDecl* parameter : functionDecl->parameters()) {
        processParameterDecl(parameter);
    }

    // Prototypes don't have bodies
    if (!functionDecl->isPrototype()) {
        processCompoundStmt(functionDecl->body());
    }
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

    for (ConstructorDecl* constructor : structDecl->constructors()) {
        processFunctionDecl(constructor);
    }

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

//=====================================================================================================================
// STATEMENTS
//=====================================================================================================================
void gulc::CodeProcessor::processStmt(gulc::Stmt* stmt) {
    switch (stmt->getStmtKind()) {
        case Stmt::Kind::Break:
            processBreakStmt(llvm::dyn_cast<BreakStmt>(stmt));
            break;
        case Stmt::Kind::Case:
            // TODO: I don't think this should be included in the general area...
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
        case Stmt::Kind::Do:
            processDoStmt(llvm::dyn_cast<DoStmt>(stmt));
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
            printError("[INTERNAL ERROR] unsupported `Stmt` found in `CodeProcessor::processStmt`!",
                       stmt->startPosition(), stmt->endPosition());
            break;
    }
}

void gulc::CodeProcessor::processBreakStmt(gulc::BreakStmt* breakStmt) {
    // TODO: We will need to keep a list of all current labels and the current containing loop
}

void gulc::CodeProcessor::processCaseStmt(gulc::CaseStmt* caseStmt) {

}

void gulc::CodeProcessor::processCatchStmt(gulc::CatchStmt* catchStmt) {

}

void gulc::CodeProcessor::processCompoundStmt(gulc::CompoundStmt* compoundStmt) {
    for (Stmt* statement : compoundStmt->statements) {
        processStmt(statement);
    }
}

void gulc::CodeProcessor::processContinueStmt(gulc::ContinueStmt* continueStmt) {
    // TODO: We will need to keep a list of all current labels and the current containing loop
}

void gulc::CodeProcessor::processDoStmt(gulc::DoStmt* doStmt) {

}

void gulc::CodeProcessor::processForStmt(gulc::ForStmt* forStmt) {

}

void gulc::CodeProcessor::processGotoStmt(gulc::GotoStmt* gotoStmt) {
    // TODO: Keep track of all labels... I think we could do this through a lookup? Maybe?
}

void gulc::CodeProcessor::processIfStmt(gulc::IfStmt* ifStmt) {

}

void gulc::CodeProcessor::processLabeledStmt(gulc::LabeledStmt* labeledStmt) {

}

void gulc::CodeProcessor::processReturnStmt(gulc::ReturnStmt* returnStmt) {
    if (returnStmt->returnValue != nullptr) {
        processExpr(returnStmt->returnValue);
    }
}

void gulc::CodeProcessor::processSwitchStmt(gulc::SwitchStmt* switchStmt) {

}

void gulc::CodeProcessor::processTryStmt(gulc::TryStmt* tryStmt) {
    processStmt(tryStmt->body());

    for (CatchStmt* catchStmt : tryStmt->catchStatements()) {
        processCatchStmt(catchStmt);
    }

    if (tryStmt->hasFinallyStatement()) {
        processStmt(tryStmt->finallyStatement());
    }
}

void gulc::CodeProcessor::processWhileStmt(gulc::WhileStmt* whileStmt) {

}

//=====================================================================================================================
// EXPRESSIONS
//=====================================================================================================================
void gulc::CodeProcessor::processExpr(gulc::Expr* expr) {
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
            printError("argument label found outside of function call, this is not supported! (how did you even do that?!)",
                       expr->startPosition(), expr->endPosition());
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
        case Expr::Kind::TemplateConstRefExpr:
            processTemplateConstRefExpr(llvm::dyn_cast<TemplateConstRefExpr>(expr));
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
    for (Expr* index : arrayLiteralExpr->indexes) {
        processExpr(index);
    }
}

void gulc::CodeProcessor::processAsExpr(gulc::AsExpr* asExpr) {
    processExpr(asExpr->expr);

    // TODO: How will we handle this? Check the types for cast support? And look for overloads?
}

void gulc::CodeProcessor::processAssignmentOperatorExpr(gulc::AssignmentOperatorExpr* assignmentOperatorExpr) {
    processExpr(assignmentOperatorExpr->leftValue);
    processExpr(assignmentOperatorExpr->rightValue);
}

void gulc::CodeProcessor::processCheckExtendsTypeExpr(gulc::CheckExtendsTypeExpr* checkExtendsTypeExpr) {
    // TODO: Should this be support at this point? Should we do `const` solving here?
}

void gulc::CodeProcessor::processFunctionCallExpr(gulc::FunctionCallExpr* functionCallExpr) {
    Expr* resolvedFunctionReference = nullptr;

    if (llvm::isa<IdentifierExpr>(functionCallExpr->functionReference)) {
        auto identifierExpr = llvm::dyn_cast<IdentifierExpr>(functionCallExpr->functionReference);
        std::string const& findName = identifierExpr->identifier().name();

        if (!identifierExpr->hasTemplateArguments()) {
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
                            printError("template type `" + checkTemplateParameter->identifier().name() + "` does not have a constructor matching the provided arguments!",
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
                                printError("using the `()` operator on template const arguments not yet supported!",
                                           functionCallExpr->startPosition(), functionCallExpr->endPosition());
                            }
                        }
                        break;
                    }
                }
            }
        }

        // TODO: Search members of our current containers


        // TODO: Search the current file if we make it this far
        // TODO: Search the imports if we haven't found anything yet, double check to make sure there aren't any
        //       ambiguous calls
    } else if (llvm::isa<MemberAccessCallExpr>(functionCallExpr->functionReference)) {
        // TODO: Process the object reference normally, search the object for the function
        auto memberAccessCallExpr = llvm::dyn_cast<MemberAccessCallExpr>(functionCallExpr->functionReference);
    } else {
        // TODO: Make sure the value type is a function pointer
        processExpr(functionCallExpr->functionReference);
    }

functionReferenceFound:
    for (LabeledArgumentExpr* argument : functionCallExpr->arguments) {
        processLabeledArgumentExpr(argument);
    }
}

bool gulc::CodeProcessor::fillListOfMatchingConstructors(gulc::Type* fromType,
                                                         const std::vector<LabeledArgumentExpr*>& arguments,
                                                         std::vector<MatchingDecl>& matchingDecls) {
    Type* processType = fromType;

    if (llvm::isa<DependentType>(processType)) {
        processType = llvm::dyn_cast<DependentType>(processType)->dependent;
    }

    bool matchFound = false;

    // TODO: We should also look for `T has init(...)` at some point (once supported)
    // TODO: For `StructType` and `TemplateStructType` we should also account for any extensions these have within our
    //       current context
    if (llvm::isa<StructType>(processType)) {
        auto structType = llvm::dyn_cast<StructType>(processType);

        for (ConstructorDecl* checkConstructor : structType->decl()->constructors()) {
            SignatureComparer::ArgMatchResult result =
                    SignatureComparer::compareArgumentsToParameters(checkConstructor->parameters(), arguments);

            if (result != SignatureComparer::ArgMatchResult::Fail) {
                matchFound = true;

                matchingDecls.emplace_back(MatchingDecl(result == SignatureComparer::ArgMatchResult::Match ?
                                                        MatchingDecl::Kind::Match : MatchingDecl::Kind::Castable,
                                                        checkConstructor));
            }
        }
    } else if (llvm::isa<TemplateStructType>(processType)) {
        auto templateStructType = llvm::dyn_cast<TemplateStructType>(processType);

        for (ConstructorDecl* checkConstructor : templateStructType->decl()->constructors()) {
            SignatureComparer::ArgMatchResult result =
                    SignatureComparer::compareArgumentsToParameters(checkConstructor->parameters(), arguments,
                                                                    templateStructType->decl()->templateParameters(),
                                                                    templateStructType->templateArguments());

            if (result != SignatureComparer::ArgMatchResult::Fail) {
                matchFound = true;

                matchingDecls.emplace_back(MatchingDecl(result == SignatureComparer::ArgMatchResult::Match ?
                                                        MatchingDecl::Kind::Match : MatchingDecl::Kind::Castable,
                                                        checkConstructor));
            }
        }
    }

    return matchFound;
}

bool gulc::CodeProcessor::fillListOfMatchingCallOperators(gulc::Type* fromType,
                                                          const std::vector<LabeledArgumentExpr*>& arguments,
                                                          std::vector<MatchingDecl>& matchingDecls) {
    Type* processType = fromType;

    if (llvm::isa<DependentType>(processType)) {
        processType = llvm::dyn_cast<DependentType>(processType)->dependent;
    }

    std::vector<Decl*>* checkDecls = nullptr;
    std::vector<Decl*>* checkInheritedDecls = nullptr;
    std::vector<TemplateParameterDecl*>* templateParameters = nullptr;
    std::vector<Expr*>* templateArguments = nullptr;

    if (llvm::isa<StructType>(processType)) {
        auto structType = llvm::dyn_cast<StructType>(processType);

        checkDecls = &structType->decl()->ownedMembers();
        // TODO: This
        //checkInheritedDecls = &structType->decl()->inheritedMembers();
    } else if (llvm::isa<TemplateStructType>(processType)) {
        auto templateStructType = llvm::dyn_cast<TemplateStructType>(processType);

        checkDecls = &templateStructType->decl()->ownedMembers();
        // TODO: This
        //checkInheritedDecls = &structType->decl()->inheritedMembers();
        templateParameters = &templateStructType->decl()->templateParameters();
        templateArguments = &templateStructType->templateArguments();
    } else if (llvm::isa<TraitType>(processType)) {
        auto traitType = llvm::dyn_cast<TraitType>(processType);

        checkDecls = &traitType->decl()->ownedMembers();
        // TODO: This
        //checkInheritedDecls = &structType->decl()->inheritedMembers();
    } else if (llvm::isa<TemplateTraitType>(processType)) {
        auto templateTraitType = llvm::dyn_cast<TemplateTraitType>(processType);

        checkDecls = &templateTraitType->decl()->ownedMembers();
        // TODO: This
        //checkInheritedDecls = &structType->decl()->inheritedMembers();
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

    // TODO: Check inherited members...

    return matchFound;
}

bool gulc::CodeProcessor::fillListOfMatchingFunctors(gulc::Decl* fromContainer, IdentifierExpr* identifierExpr,
                                                     const std::vector<LabeledArgumentExpr*>& arguments,
                                                     std::vector<MatchingDecl> &matchingDecls) {
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

        // TODO: Support `NamespaceDecl`
    } else if (llvm::isa<StructDecl>(fromContainer)) {
        auto structContainer = llvm::dyn_cast<StructDecl>(fromContainer);

        // TODO: Support `StructDecl`
        for (Decl* checkDecl : structContainer->ownedMembers()) {

        }

        // TODO: Add support for an `inheritedMembers` member and search that
    } else if (llvm::isa<TemplateStructDecl>(fromContainer)) {
        // TODO: Support `TemplateStructDecl`
        printError("[INTERNAL] searching `TemplateStructDecl` as a container is not yet supported!",
                   fromContainer->startPosition(), fromContainer->endPosition());
    } else if (llvm::isa<TemplateTraitDecl>(fromContainer)) {
        // TODO: Support `TemplateTraitDecl`
        printError("[INTERNAL] searching `TemplateTraitDecl` as a container is not yet supported!",
                   fromContainer->startPosition(), fromContainer->endPosition());
    } else if (llvm::isa<TraitDecl>(fromContainer)) {
        auto traitContainer = llvm::dyn_cast<TraitDecl>(fromContainer);

        // TODO: Support `TraitDecl`
    } else {
        printError("[INTERNAL] unsupported container Decl found in `CodeProcessor::fillListOfMatchingFunctors`!",
                   fromContainer->startPosition(), fromContainer->endPosition());
    }

    return false;
}

void gulc::CodeProcessor::processHasExpr(gulc::HasExpr* hasExpr) {
    // TODO: We need to fix this. I want to support `T has func test(_: i32)`
}

void gulc::CodeProcessor::processIdentifierExpr(gulc::IdentifierExpr* identifierExpr) {
    // TODO: The generic `processIdentifierExpr` should ONLY look for normal variables and types, it should NOT look
    //       for functions. `processFunctionCallExpr` and `processIndexerCallExpr` should handle `IdentifierExpr` as
    //       special cases in their own functions. They should ALSO handle `MemberAccessCallExpr` as a special case.
}

void gulc::CodeProcessor::processIndexerCallExpr(gulc::IndexerCallExpr* indexerCallExpr) {
    processExpr(indexerCallExpr->indexerReference);

    for (LabeledArgumentExpr* argument : indexerCallExpr->arguments) {
        processLabeledArgumentExpr(argument);
    }
}

void gulc::CodeProcessor::processInfixOperatorExpr(gulc::InfixOperatorExpr* infixOperatorExpr) {
    processExpr(infixOperatorExpr->leftValue);
    processExpr(infixOperatorExpr->rightValue);
}

void gulc::CodeProcessor::processIsExpr(gulc::IsExpr* isExpr) {
    processExpr(isExpr->expr);

    // TODO: Should we just do `const` solving here again?
}

void gulc::CodeProcessor::processLabeledArgumentExpr(gulc::LabeledArgumentExpr* labeledArgumentExpr) {
    processExpr(labeledArgumentExpr->argument);
}

void gulc::CodeProcessor::processMemberAccessCallExpr(gulc::MemberAccessCallExpr* memberAccessCallExpr) {
    // TODO: We will have to account for extensions here once we actually check the type for members...
    processExpr(memberAccessCallExpr->objectRef);
}

void gulc::CodeProcessor::processParenExpr(gulc::ParenExpr* parenExpr) {
    processExpr(parenExpr);
}

void gulc::CodeProcessor::processPostfixOperatorExpr(gulc::PostfixOperatorExpr* postfixOperatorExpr) {
    processExpr(postfixOperatorExpr->nestedExpr);
}

void gulc::CodeProcessor::processPrefixOperatorExpr(gulc::PrefixOperatorExpr* prefixOperatorExpr) {
    processExpr(prefixOperatorExpr->nestedExpr);
}

void gulc::CodeProcessor::processTemplateConstRefExpr(gulc::TemplateConstRefExpr* templateConstRefExpr) {
    // TODO: Should we error here? Or return the const value? Or do nothing until a const solving stage?
}

void gulc::CodeProcessor::processTernaryExpr(gulc::TernaryExpr* ternaryExpr) {
    // TODO: We will need to check if `condition` returns a bool or not
    processExpr(ternaryExpr->condition);
    processExpr(ternaryExpr->trueExpr);
    processExpr(ternaryExpr->falseExpr);
}

void gulc::CodeProcessor::processTypeExpr(gulc::TypeExpr* typeExpr) {

}

void gulc::CodeProcessor::processValueLiteralExpr(gulc::ValueLiteralExpr* valueLiteralExpr) {

}

void gulc::CodeProcessor::processVariableDeclExpr(gulc::VariableDeclExpr* variableDeclExpr) {
    // TODO: Add the variable to a local variables list, detect shadowing (should we allow redefinition like Rust/Swift?)
    if (variableDeclExpr->initialValue != nullptr) {
        processExpr(variableDeclExpr->initialValue);
        // TODO: Detect variable type from `initialValue` if the type isn't already set...
    }
}
