// Copyright (C) 2019 Michael Brandon Huddle
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <AST/Types/BuiltInType.hpp>
#include <AST/Types/UnresolvedType.hpp>
#include <AST/Types/FunctionTemplateTypenameRefType.hpp>
#include <AST/Types/ReferenceType.hpp>
#include <AST/Types/ConstType.hpp>
#include <AST/Types/MutType.hpp>
#include <AST/Types/ImmutType.hpp>
#include <AST/Types/PointerType.hpp>
#include <AST/Types/FunctionPointerType.hpp>
#include <AST/Types/TemplateTypenameType.hpp>
#include <AST/Types/RValueReferenceType.hpp>
#include <AST/Types/EnumType.hpp>
#include <AST/Exprs/UnresolvedTypeRefExpr.hpp>
#include <AST/Exprs/LocalVariableDeclOrPrefixOperatorCallExpr.hpp>
#include <AST/Exprs/CustomPrefixOperatorExpr.hpp>
#include <AST/Exprs/PotentialExplicitCastExpr.hpp>
#include "TypeResolver.hpp"

using namespace gulc;

void TypeResolver::processFile(FileAST &fileAst) {
    currentFileAst = &fileAst;

    // TODO: We need to do two passes. One for assigning types to all declarations and then one for assigning them to expressions
    for (Decl* decl : fileAst.topLevelDecls()) {
        processDecl(decl);
    }
}

void TypeResolver::printError(const std::string &message, TextPosition startPosition, TextPosition endPosition) {
    std::cout << "gulc resolver error[" << currentFileAst->filePath() << ", "
                                     "{" << startPosition.line << ", " << startPosition.column << "} "
                                     "to {" << endPosition.line << ", " << endPosition.column << "}]: "
              << message
              << std::endl;
    std::exit(1);
}

void TypeResolver::printWarning(const std::string &message, TextPosition startPosition, TextPosition endPosition) {
    std::cout << "gulc warning[" << currentFileAst->filePath() << ", "
                              "{" << startPosition.line << ", " << startPosition.column << "} "
                              "to {" << endPosition.line << ", " << endPosition.column << "}]: "
              << message
              << std::endl;
}

void TypeResolver::printDebugWarning(const std::string &message) {
#ifndef NDEBUG
    std::cout << "gulc resolver [DEBUG WARNING](" << currentFileAst->filePath() << "): " << message << std::endl;
#endif
}

bool TypeResolver::resolveType(Type *&type) {
    if (type->getTypeKind() == Type::Kind::Unresolved) {
        // TODO: Take 'namespacePath' into consideration. If there is a namespace path we obviously don't have to worry about the template params
        auto unresolvedType = llvm::dyn_cast<UnresolvedType>(type);

        if (BuiltInType::isBuiltInType(unresolvedType->name())) {
            // TODO: Support template overloading. Allow someone to implement `struct int<T> {}` that will be found if there are template arguments
            if (unresolvedType->hasTemplateArguments()) {
                printError("built in types do not support templating!",
                           unresolvedType->startPosition(), unresolvedType->endPosition());
            }

            Type *oldType = type;
            type = new BuiltInType(oldType->startPosition(), oldType->endPosition(), unresolvedType->name());
            delete oldType;
            return true;
        }

        // We check the function templates first...
        // Function template params can't be templated themselves?
        if (!unresolvedType->hasTemplateArguments() && functionTemplateParams) {
            for (TemplateParameterDecl *templateParameterDecl : *functionTemplateParams) {
                if (templateParameterDecl->type->getTypeKind() == Type::Kind::TemplateTypename) {
                    if (templateParameterDecl->name() == unresolvedType->name()) {
                        Type *oldType = type;
                        type = new FunctionTemplateTypenameRefType(oldType->startPosition(), oldType->endPosition(),
                                                                   templateParameterDecl->name());
                        delete oldType;
                        return true;
                    }
                }
            }
        }

        // Check the file decls...
        for (Decl* checkDecl : currentFileAst->topLevelDecls()) {
            if (checkDecl->name() == unresolvedType->name()) {
                // TODO: Take templates into consideration
                //  `class Example<T> where T : Widget` and `class Example<T> where T : Window` should be supported
                if (llvm::isa<EnumDecl>(checkDecl)) {
                    auto enumDecl = llvm::dyn_cast<EnumDecl>(checkDecl);
                    Type* baseType;

                    if (enumDecl->hasBaseType()) {
                        baseType = deepCopy(enumDecl->baseType);
                    } else {
                        // Default type for enum is uint32
                        baseType = new BuiltInType(type->startPosition(), type->endPosition(), "uint32");
                    }

                    Type* oldType = type;
                    type = new EnumType(oldType->startPosition(), oldType->endPosition(), enumDecl->name(), baseType);

                    delete oldType;
                    return true;
                }
            }
        }
    } else if (type->getTypeKind() == Type::Kind::Const) {
        auto constType = llvm::dyn_cast<ConstType>(type);

        if (llvm::isa<ConstType>(constType->pointToType)) {
            printWarning("duplicate `const` qualifier not needed!",
                         constType->startPosition(), constType->endPosition());
        } else if (llvm::isa<MutType>(constType->pointToType)) {
            printError("`const` and `mut` qualifiers are not mixable!",
                       constType->startPosition(), constType->endPosition());
            return false;
        } else if (llvm::isa<ImmutType>(constType->pointToType)) {
            printError("`const` and `immut` qualifiers are not mixable!",
                       constType->startPosition(), constType->endPosition());
            return false;
        }

        return resolveType(constType->pointToType);
    } else if (type->getTypeKind() == Type::Kind::Mut) {
        auto mutType = llvm::dyn_cast<MutType>(type);

        if (llvm::isa<ConstType>(mutType->pointToType)) {
            printError("`mut` and `const` qualifiers are not mixable!",
                       mutType->startPosition(), mutType->endPosition());
            return false;
        } else if (llvm::isa<MutType>(mutType->pointToType)) {
            printWarning("duplicate `mut` qualifier not needed!",
                         mutType->startPosition(), mutType->endPosition());
        } else if (llvm::isa<ImmutType>(mutType->pointToType)) {
            printError("`mut` and `immut` qualifiers are not mixable!",
                       mutType->startPosition(), mutType->endPosition());
            return false;
        }

        return resolveType(mutType->pointToType);
    } else if (type->getTypeKind() == Type::Kind::Immut) {
        auto immutType = llvm::dyn_cast<ImmutType>(type);

        if (llvm::isa<ConstType>(immutType->pointToType)) {
            printError("`immut` and `const` qualifiers are not mixable!",
                       immutType->startPosition(), immutType->endPosition());
            return false;
        } else if (llvm::isa<MutType>(immutType->pointToType)) {
            printError("`immut` and `mut` qualifiers are not mixable!",
                       immutType->startPosition(), immutType->endPosition());
            return false;
        } else if (llvm::isa<ImmutType>(immutType->pointToType)) {
            printWarning("duplicate `immut` qualifier not needed!",
                         immutType->startPosition(), immutType->endPosition());
        }

        return resolveType(immutType->pointToType);
    } else if (type->getTypeKind() == Type::Kind::Pointer) {
        auto pointerType = llvm::dyn_cast<PointerType>(type);
        return resolveType(pointerType->pointToType);
    } else if (type->getTypeKind() == Type::Kind::Reference) {
        auto referenceType = llvm::dyn_cast<ReferenceType>(type);
        return resolveType(referenceType->referenceToType);
    }

    return false;
}

Type *TypeResolver::deepCopy(const Type *type) {
    switch (type->getTypeKind()) {
        case Type::Kind::BuiltIn: {
            auto builtInType = llvm::dyn_cast<BuiltInType>(type);
            return new BuiltInType(builtInType->startPosition(), builtInType->endPosition(), builtInType->name());
        }
        case Type::Kind::FunctionTemplateTypenameRef: {
            auto functionTemplateTypenameRef = llvm::dyn_cast<FunctionTemplateTypenameRefType>(type);
            return new FunctionTemplateTypenameRefType(functionTemplateTypenameRef->startPosition(),
                                                       functionTemplateTypenameRef->endPosition(),
                                                       functionTemplateTypenameRef->name());
        }
        case Type::Kind::Const: {
            auto constType = llvm::dyn_cast<ConstType>(type);
            return new ConstType(constType->startPosition(), constType->endPosition(), deepCopy(constType->pointToType));
        }
        case Type::Kind::Mut: {
            auto mutType = llvm::dyn_cast<MutType>(type);
            return new MutType(mutType->startPosition(), mutType->endPosition(), deepCopy(mutType->pointToType));
        }
        case Type::Kind::Immut: {
            auto immutType = llvm::dyn_cast<ImmutType>(type);
            return new ImmutType(immutType->startPosition(), immutType->endPosition(), deepCopy(immutType->pointToType));
        }
        case Type::Kind::Pointer: {
            auto pointerType = llvm::dyn_cast<PointerType>(type);
            return new PointerType(pointerType->startPosition(), pointerType->endPosition(), deepCopy(pointerType->pointToType));
        }
        case Type::Kind::FunctionPointer: {
            auto functionPointer = llvm::dyn_cast<FunctionPointerType>(type);
            Type* resultType = deepCopy(functionPointer->resultType);
            std::vector<Type*> paramTypes{};

            if (!functionPointer->paramTypes.empty()) {
                paramTypes.reserve(functionPointer->paramTypes.size());

                for (const Type* paramType : functionPointer->paramTypes) {
                    paramTypes.emplace_back(deepCopy(paramType));
                }
            }

            return new FunctionPointerType(functionPointer->startPosition(), functionPointer->endPosition(), resultType, paramTypes);
        }
        case Type::Kind::TemplateTypename: {
            auto templateTypenameType = llvm::dyn_cast<TemplateTypenameType>(type);
            return new TemplateTypenameType(templateTypenameType->startPosition(), templateTypenameType->endPosition());
        }
        case Type::Kind::Reference: {
            auto referenceType = llvm::dyn_cast<ReferenceType>(type);
            return new ReferenceType(referenceType->startPosition(), referenceType->endPosition(), deepCopy(referenceType->referenceToType));
        }
        case Type::Kind::RValueReference: {
            auto rvalueReferenceType = llvm::dyn_cast<RValueReferenceType>(type);
            return new ReferenceType(rvalueReferenceType->startPosition(), rvalueReferenceType->endPosition(), deepCopy(rvalueReferenceType->referenceToType));
        }
        case Type::Kind::Unresolved: {
            std::cout << "gulc [INTERNAL] resolver error: attempted to deep copy unresolved type, operation not supported!" << std::endl;
            std::exit(1);
        }
        default:
            std::cout << "gulc [INTERNAL] resolver error: attempted to deep copy an unsupported type!" << std::endl;
            std::exit(1);
    }
    // MSVC apparently doesn't take 'std::exit' into account and thinks we don't return on all code paths...
    return nullptr;
}

void TypeResolver::processDecl(Decl *decl) {
    switch (decl->getDeclKind()) {
        case Decl::Kind::Function:
            processFunctionDecl(llvm::dyn_cast<FunctionDecl>(decl));
            break;
        case Decl::Kind::GlobalVariable:
            processGlobalVariableDecl(llvm::dyn_cast<GlobalVariableDecl>(decl));
            break;
        case Decl::Kind::Enum:
            processEnumDecl(llvm::dyn_cast<EnumDecl>(decl));
            break;
        case Decl::Kind::Parameter:
        case Decl::Kind::TemplateParameter:
        default:
            printDebugWarning("unhandled Decl in 'processDecl'!");
            break;
    }
}

void TypeResolver::processStmt(Stmt *&stmt) {
    switch (stmt->getStmtKind()) {
        case Stmt::Kind::Case:
            processCaseStmt(llvm::dyn_cast<CaseStmt>(stmt));
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
        case Stmt::Kind::TryCatch:
            processTryCatchStmt(llvm::dyn_cast<TryCatchStmt>(stmt));
            break;
        case Stmt::Kind::TryFinally:
            processTryFinallyStmt(llvm::dyn_cast<TryFinallyStmt>(stmt));
            break;
        case Stmt::Kind::While:
            processWhileStmt(llvm::dyn_cast<WhileStmt>(stmt));
            break;
        case Stmt::Kind::Expr: {
            auto expr = llvm::dyn_cast<Expr>(stmt);
            processExpr(expr);
            stmt = expr;
        }
    }
}

void TypeResolver::processExpr(Expr *&expr) {
    switch (expr->getExprKind()) {
        case Expr::Kind::BinaryOperator:
            processBinaryOperatorExpr(llvm::dyn_cast<BinaryOperatorExpr>(expr));
            break;
        case Expr::Kind::CharacterLiteral:
            processCharacterLiteralExpr(llvm::dyn_cast<CharacterLiteralExpr>(expr));
            break;
        case Expr::Kind::ExplicitCast:
            processExplicitCastExpr(llvm::dyn_cast<ExplicitCastExpr>(expr));
            break;
        case Expr::Kind::FloatLiteral:
            processFloatLiteralExpr(llvm::dyn_cast<FloatLiteralExpr>(expr));
            break;
        case Expr::Kind::FunctionCall:
            processFunctionCallExpr(llvm::dyn_cast<FunctionCallExpr>(expr));
            break;
        case Expr::Kind::Identifier:
            processIdentifierExpr(expr);
            break;
        case Expr::Kind::ImplicitCast:
            processImplicitCastExpr(llvm::dyn_cast<ImplicitCastExpr>(expr));
            break;
        case Expr::Kind::IndexerCall:
            processIndexerCallExpr(llvm::dyn_cast<IndexerCallExpr>(expr));
            break;
        case Expr::Kind::IntegerLiteral:
            processIntegerLiteralExpr(llvm::dyn_cast<IntegerLiteralExpr>(expr));
            break;
        case Expr::Kind::LocalVariableDecl:
            processLocalVariableDeclExpr(llvm::dyn_cast<LocalVariableDeclExpr>(expr));
            break;
        case Expr::Kind::LocalVariableDeclOrPrefixOperatorCallExpr:
            // Casting isn't required for this function. It will handle the casting for us since this is a type we will be completely removing from the AST in this function
            processLocalVariableDeclOrPrefixOperatorCallExpr(expr);
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
        case Expr::Kind::PotentialExplicitCast:
            processPotentialExplicitCastExpr(expr);
            break;
        case Expr::Kind::PrefixOperator:
            processPrefixOperatorExpr(llvm::dyn_cast<PrefixOperatorExpr>(expr));
            break;
        case Expr::Kind::ResolvedTypeRef:
            processResolvedTypeRefExpr(llvm::dyn_cast<ResolvedTypeRefExpr>(expr));
            break;
        case Expr::Kind::StringLiteral:
            processStringLiteralExpr(llvm::dyn_cast<StringLiteralExpr>(expr));
            break;
        case Expr::Kind::Ternary:
            processTernaryExpr(llvm::dyn_cast<TernaryExpr>(expr));
            break;
        case Expr::Kind::UnresolvedTypeRef:
            processUnresolvedTypeRefExpr(expr);
            break;
    }
}

// Decls
void TypeResolver::processEnumDecl(EnumDecl *enumDecl) {
    if (!enumDecl->hasBaseType()) {
        // If the enum doesn't have a base type we default to an unsigned 32-bit integer
        enumDecl->baseType = new BuiltInType(enumDecl->startPosition(), enumDecl->endPosition(), "uint32");
    } else if (!resolveType(enumDecl->baseType)) {
        printError("could not resolve enum `" + enumDecl->name() + "`s base type `" + enumDecl->baseType->getString() + "`!",
                   enumDecl->baseType->startPosition(), enumDecl->baseType->endPosition());
        return;
    }

    if (enumDecl->hasConstants()) {
        for (EnumConstantDecl* enumConstant : enumDecl->enumConstants()) {
            if (enumConstant->hasConstantValue()) {
                processExpr(enumConstant->constantValue);
            }
        }
    }
}

void TypeResolver::processFunctionDecl(FunctionDecl *functionDecl) {
    if (functionDecl->hasTemplateParameters()) {
        bool shouldHaveDefaultArgument = false;

        // We allow `void func<typename T, T value>()` so we have to set the functionTemplateParams here...
        functionTemplateParams = &functionDecl->templateParameters;

        for (TemplateParameterDecl* templateParameterDecl : functionDecl->templateParameters) {
            if (!resolveType(templateParameterDecl->type)) {
                printError("template parameter type `" + templateParameterDecl->type->getString() + "` was not found!",
                           templateParameterDecl->startPosition(), templateParameterDecl->endPosition());
            }

            // Also make sure all template parameters after the first optional template parameter are also optional...
            if (templateParameterDecl->hasDefaultArgument()) {
                if (!shouldHaveDefaultArgument) {
                    shouldHaveDefaultArgument = true;
                } else {
                    printError("all template parameters after the first optional template parameter must also be optional!",
                               templateParameterDecl->startPosition(), templateParameterDecl->endPosition());
                }
            }
        }
    }

    // Resolve function return type...
    if (!resolveType(functionDecl->resultType)) {
        printError("could not find function return type `" + functionDecl->resultType->getString() + "`!",
                   functionDecl->resultType->startPosition(), functionDecl->resultType->endPosition());
    }

    if (functionDecl->hasParameters()) {
        bool shouldHaveDefaultArgument = false;

        // Make sure all parameters after the first optional parameter are also optional
        for (ParameterDecl* parameterDecl : functionDecl->parameters) {
            if (!resolveType(parameterDecl->type)) {
                printError("could not find function parameter type!",
                           parameterDecl->startPosition(), parameterDecl->endPosition());
            }

            if (parameterDecl->hasDefaultArgument()) {
                if (!shouldHaveDefaultArgument) {
                    shouldHaveDefaultArgument = true;
                } else {
                    printError("all parameters after the first optional parameter must also be optional!",
                               parameterDecl->startPosition(), parameterDecl->endPosition());
                }
            }
        }
    }

    processCompoundStmt(functionDecl->body());
}

void TypeResolver::processGlobalVariableDecl(GlobalVariableDecl *globalVariableDecl) {
    // Resolve global variable type...
    if (!resolveType(globalVariableDecl->type)) {
        printError("could not find function return type `" + globalVariableDecl->type->getString() + "`!",
                   globalVariableDecl->type->startPosition(), globalVariableDecl->type->endPosition());
    }
}

// Stmts
void TypeResolver::processCaseStmt(CaseStmt *caseStmt) {
    if (caseStmt->hasCondition()) {
        processExpr(caseStmt->condition);
    }

    processStmt(caseStmt->trueStmt);
}

void TypeResolver::processCompoundStmt(CompoundStmt *compoundStmt) {
    for (Stmt*& stmt : compoundStmt->statements()) {
        processStmt(stmt);
    }
}

void TypeResolver::processDoStmt(DoStmt *doStmt) {
    processStmt(doStmt->loopStmt);
    processExpr(doStmt->condition);
}

void TypeResolver::processForStmt(ForStmt *forStmt) {
    processExpr(forStmt->preLoop);
    processExpr(forStmt->condition);
    processExpr(forStmt->iterationExpr);

    processStmt(forStmt->loopStmt);
}

void TypeResolver::processIfStmt(IfStmt *ifStmt) {
    processExpr(ifStmt->condition);
    processStmt(ifStmt->trueStmt);

    if (ifStmt->hasFalseStmt()) {
        processStmt(ifStmt->falseStmt);
    }
}

void TypeResolver::processLabeledStmt(LabeledStmt *labeledStmt) {
    processStmt(labeledStmt->labeledStmt);
}

void TypeResolver::processReturnStmt(ReturnStmt *returnStmt) {
    if (returnStmt->hasReturnValue()) {
        processExpr(returnStmt->returnValue);
    }
}

void TypeResolver::processSwitchStmt(SwitchStmt *switchStmt) {
    processExpr(switchStmt->condition);

    for (CaseStmt* caseStmt : switchStmt->cases()) {
        processCaseStmt(caseStmt);
    }
}

void TypeResolver::processTryStmt(TryStmt *tryStmt) {
    processCompoundStmt(tryStmt->encapsulatedStmt);

    if (tryStmt->hasCatchStmts()) {
        for (TryCatchStmt* catchStmt : tryStmt->catchStmts()) {
            processTryCatchStmt(catchStmt);
        }
    }

    if (tryStmt->hasFinallyStmt()) {
        processTryFinallyStmt(tryStmt->finallyStmt);
    }
}

void TypeResolver::processTryCatchStmt(TryCatchStmt *tryCatchStmt) {
    if (tryCatchStmt->hasExceptionDecl() && !resolveType(tryCatchStmt->exceptionType)) {
        printError("catch type `" + tryCatchStmt->exceptionType->getString() + "` was not found!",
                   tryCatchStmt->exceptionType->startPosition(), tryCatchStmt->exceptionType->endPosition());
    }

    processCompoundStmt(tryCatchStmt->handlerStmt);
}

void TypeResolver::processTryFinallyStmt(TryFinallyStmt *tryFinallyStmt) {
    processCompoundStmt(tryFinallyStmt->handlerStmt);
}

void TypeResolver::processWhileStmt(WhileStmt *whileStmt) {
    processExpr(whileStmt->condition);
    processStmt(whileStmt->loopStmt);
}

// Exprs
void TypeResolver::processBinaryOperatorExpr(BinaryOperatorExpr *binaryOperatorExpr) {
    processExpr(binaryOperatorExpr->leftValue);
    processExpr(binaryOperatorExpr->rightValue);
}

void TypeResolver::processCharacterLiteralExpr(CharacterLiteralExpr *characterLiteralExpr) {
    // TODO: Support custom type suffixes here...
}

void TypeResolver::processExplicitCastExpr(ExplicitCastExpr *explicitCastExpr) {
    if (!resolveType(explicitCastExpr->castType)) {
        printError("explicit cast type `" + explicitCastExpr->castType->getString() + "` was not found!",
                   explicitCastExpr->startPosition(), explicitCastExpr->endPosition());
    }

    processExpr(explicitCastExpr->castee);
}

void TypeResolver::processFloatLiteralExpr(FloatLiteralExpr *floatLiteralExpr) {
    // TODO: Support custom type suffixes here...
}

void TypeResolver::processFunctionCallExpr(FunctionCallExpr *functionCallExpr) {
    if (functionCallExpr->hasArguments()) {
        for (Expr*& arg : functionCallExpr->arguments) {
            processExpr(arg);
        }
    }

    processExpr(functionCallExpr->functionReference);
}

void TypeResolver::processIdentifierExpr(Expr*& expr) {
    auto identifierExpr = llvm::dyn_cast<IdentifierExpr>(expr);

    // First we check if the identifier is a built in type
    if (BuiltInType::isBuiltInType(identifierExpr->name())) {
        // TODO: Support template overloading. Allow someone to implement `struct int<T> {}` that will be found if there are template arguments
        if (identifierExpr->hasTemplateArguments()) {
            printError("built in types do not support templating!",
                       identifierExpr->startPosition(), identifierExpr->endPosition());
        }

        Type *resolvedType = new BuiltInType(expr->startPosition(), expr->endPosition(), identifierExpr->name());

        delete identifierExpr;

        expr = new ResolvedTypeRefExpr(expr->startPosition(), expr->endPosition(), resolvedType);
        expr->resultType = deepCopy(resolvedType);
        return;
    }

    if (identifierExpr->hasTemplateArguments()) {
        for (Expr*& templateArgument : identifierExpr->templateArguments) {
            processExpr(templateArgument);
        }
    }

    // Check the file for types...
    for (const Decl* decl : currentFileAst->topLevelDecls()) {
        if (decl->name() == identifierExpr->name()) {
            // TODO: Add all Decls that can be types as they're added...
            if (llvm::isa<EnumDecl>(decl)) {
                auto enumDecl = llvm::dyn_cast<EnumDecl>(decl);

                // TODO: We should support template overloading.
                if (identifierExpr->hasTemplateArguments()) {
                    printError("enum types cannot have template arguments!",
                               identifierExpr->startPosition(), identifierExpr->endPosition());
                }

                Type *resolvedType = new EnumType(expr->startPosition(), expr->endPosition(), identifierExpr->name(),
                                                  deepCopy(enumDecl->baseType));

                delete identifierExpr;

                expr = new ResolvedTypeRefExpr(expr->startPosition(), expr->endPosition(), resolvedType);
                expr->resultType = deepCopy(resolvedType);
                return;
            }
        }
    }

    // If we reach this point then it is assumed that `IdentifierExpr` is a variable name or function name.
    // If it isn't either of the above then that error will be caught in another pass...
}

void TypeResolver::processImplicitCastExpr(ImplicitCastExpr *implicitCastExpr) {
    // I don't think there will be any implicit casts processed here... but just in case...
    if (!resolveType(implicitCastExpr->castType)) {
        printError("implicit cast type `" + implicitCastExpr->castType->getString() + "` was not found!",
                   implicitCastExpr->startPosition(), implicitCastExpr->endPosition());
    }

    processExpr(implicitCastExpr->castee);
}

void TypeResolver::processIndexerCallExpr(IndexerCallExpr *indexerCallExpr) {
    processExpr(indexerCallExpr->indexerReference);

    if (indexerCallExpr->hasArguments()) {
        for (Expr*& arg : indexerCallExpr->arguments()) {
            processExpr(arg);
        }
    }
}

void TypeResolver::processIntegerLiteralExpr(IntegerLiteralExpr *integerLiteralExpr) {
    // TODO: Support custom type suffixes here...
}

void TypeResolver::processLocalVariableDeclExpr(LocalVariableDeclExpr *localVariableDeclExpr) {
    processExpr(localVariableDeclExpr->type);

    if (!llvm::isa<ResolvedTypeRefExpr>(localVariableDeclExpr->type)) {
        printError("unknown expression in type of variable declaration!",
                   localVariableDeclExpr->startPosition(), localVariableDeclExpr->endPosition());
    }
}

void TypeResolver::processLocalVariableDeclOrPrefixOperatorCallExpr(Expr *&expr) {
    auto localVariableDeclOrPrefixOperatorCall = llvm::dyn_cast<LocalVariableDeclOrPrefixOperatorCallExpr>(expr);

    processExpr(localVariableDeclOrPrefixOperatorCall->typeOrPrefixOperator);
    processExpr(localVariableDeclOrPrefixOperatorCall->nameOrExpr);

    // If the type or prefix operator is a resolved type ref then it is a local variable declaration
    if (llvm::isa<ResolvedTypeRefExpr>(localVariableDeclOrPrefixOperatorCall->typeOrPrefixOperator)) {
        if (!llvm::isa<IdentifierExpr>(localVariableDeclOrPrefixOperatorCall->nameOrExpr)) {
            printError("unexpected expression where local variable name was expected!",
                       localVariableDeclOrPrefixOperatorCall->nameOrExpr->startPosition(),
                       localVariableDeclOrPrefixOperatorCall->nameOrExpr->endPosition());
        }

        auto nameExpr = llvm::dyn_cast<IdentifierExpr>(localVariableDeclOrPrefixOperatorCall->nameOrExpr);

        auto newLocalVariableExpr = new LocalVariableDeclExpr(expr->startPosition(), expr->endPosition(),
                                                              localVariableDeclOrPrefixOperatorCall->typeOrPrefixOperator,
                                                              nameExpr->name());
        // We steal this pointer.
        localVariableDeclOrPrefixOperatorCall->typeOrPrefixOperator = nullptr;
        delete localVariableDeclOrPrefixOperatorCall;
        // This isn't really needed...but oh well
        processLocalVariableDeclExpr(newLocalVariableExpr);
        expr = newLocalVariableExpr;
        return;
    }

    if (llvm::isa<UnresolvedTypeRefExpr>(localVariableDeclOrPrefixOperatorCall->typeOrPrefixOperator)) {
        printDebugWarning("UNRESOLVED TYPE FOUND IN LOCAL VARIABLE DECL OR PREFIX OPERATOR CALL");
    }

    // If we reach this point then we assume the expression is a prefix operator call
    auto customPrefixOperator = new CustomPrefixOperatorExpr(expr->startPosition(), expr->endPosition(),
                                                             localVariableDeclOrPrefixOperatorCall->typeOrPrefixOperator,
                                                             localVariableDeclOrPrefixOperatorCall->nameOrExpr);
    localVariableDeclOrPrefixOperatorCall->typeOrPrefixOperator = nullptr;
    localVariableDeclOrPrefixOperatorCall->nameOrExpr = nullptr;
    delete localVariableDeclOrPrefixOperatorCall;
    expr = customPrefixOperator;
}

void TypeResolver::processMemberAccessCallExpr(MemberAccessCallExpr *memberAccessCallExpr) {
    // TODO: We need to account for if `objectRef` is converted to a Type ref or a Namespace ref...
    processExpr(memberAccessCallExpr->objectRef);

    gulc::Expr* workingExpr = memberAccessCallExpr->member;
    processIdentifierExpr(workingExpr);

    if (!llvm::dyn_cast<IdentifierExpr>(workingExpr)) {
        printError("[INTERNAL] member expression converted away from IdentifierExpr, cannot continue...",
                   memberAccessCallExpr->startPosition(), memberAccessCallExpr->endPosition());
    }

    memberAccessCallExpr->member = llvm::dyn_cast<IdentifierExpr>(workingExpr);
}

void TypeResolver::processParenExpr(ParenExpr *parenExpr) {
    processExpr(parenExpr->containedExpr);
}

void TypeResolver::processPostfixOperatorExpr(PostfixOperatorExpr *postfixOperatorExpr) {
    processExpr(postfixOperatorExpr->expr);
}

void TypeResolver::processPotentialExplicitCastExpr(Expr *&expr) {
    auto potentialExplicitCastExpr = llvm::dyn_cast<PotentialExplicitCastExpr>(expr);

    processExpr(potentialExplicitCastExpr->castType);
    processExpr(potentialExplicitCastExpr->castee);

    if (!llvm::isa<ResolvedTypeRefExpr>(potentialExplicitCastExpr->castType)) {
        printError("unknown type in explicit cast expression!",
                   potentialExplicitCastExpr->startPosition(), potentialExplicitCastExpr->endPosition());
    }

    auto resolvedTypeRef = llvm::dyn_cast<ResolvedTypeRefExpr>(potentialExplicitCastExpr->castType);
    Type* resolvedType = resolvedTypeRef->resolvedType;

    expr = new ExplicitCastExpr(potentialExplicitCastExpr->startPosition(), potentialExplicitCastExpr->endPosition(),
                                resolvedType, potentialExplicitCastExpr->castee);

    // Set the values we steal to nullptr then delete the old expression
    resolvedTypeRef->resolvedType = nullptr;
    potentialExplicitCastExpr->castee = nullptr;
    delete potentialExplicitCastExpr;
}

void TypeResolver::processPrefixOperatorExpr(PrefixOperatorExpr *prefixOperatorExpr) {
    processExpr(prefixOperatorExpr->expr);
}

void TypeResolver::processResolvedTypeRefExpr(ResolvedTypeRefExpr *resolvedTypeRefExpr) {
    // We don't have anything to do here...
}

void TypeResolver::processStringLiteralExpr(StringLiteralExpr *stringLiteralExpr) {
    // TODO: Support custom type suffixes here...
}

void TypeResolver::processTernaryExpr(TernaryExpr *ternaryExpr) {
    processExpr(ternaryExpr->condition);
    processExpr(ternaryExpr->trueExpr);
    processExpr(ternaryExpr->falseExpr);
}

void TypeResolver::processUnresolvedTypeRefExpr(Expr *&expr) {
    auto unresolvedTypeRef = llvm::dyn_cast<UnresolvedTypeRefExpr>(expr);

    if (!resolveType(unresolvedTypeRef->unresolvedType)) {
        printError("local variable type `" + unresolvedTypeRef->unresolvedType->getString() + "` was not found!",
                   unresolvedTypeRef->startPosition(), unresolvedTypeRef->endPosition());
    }

    auto resolvedType = new ResolvedTypeRefExpr(unresolvedTypeRef->startPosition(), unresolvedTypeRef->endPosition(),
                                                unresolvedTypeRef->unresolvedType);
    unresolvedTypeRef->unresolvedType = nullptr;
    delete unresolvedTypeRef;

    expr = resolvedType;
}