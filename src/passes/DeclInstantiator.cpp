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
#include <ast/types/StructType.hpp>
#include "DeclInstantiator.hpp"
#include <utilities/TemplateInstHelper.hpp>
#include <ast/types/BuiltInType.hpp>
#include <ast/types/DimensionType.hpp>
#include <ast/types/FlatArrayType.hpp>
#include <ast/types/PointerType.hpp>
#include <ast/types/ReferenceType.hpp>
#include <ast/types/TemplatedType.hpp>
#include <ast/types/UnresolvedType.hpp>
#include <utilities/SizeofUtil.hpp>
#include <ast/exprs/ValueLiteralExpr.hpp>
#include <utilities/SignatureComparer.hpp>
#include <utilities/TypeHelper.hpp>
#include <ast/types/VTableType.hpp>
#include <ast/types/TraitType.hpp>
#include <ast/types/AliasType.hpp>
#include <ast/types/UnresolvedNestedType.hpp>
#include <ast/types/TemplateStructType.hpp>
#include <ast/types/TemplateTraitType.hpp>
#include <algorithm>
#include <ast/conts/WhereCont.hpp>
#include <utilities/TypeCompareUtil.hpp>
#include <utilities/ContractUtil.hpp>
#include <ast/types/DependentType.hpp>
#include <ast/types/EnumType.hpp>
#include <make_reverse_iterator.hpp>
#include <utilities/InheritUtil.hpp>
#include <ast/types/ImaginaryType.hpp>
#include <ast/exprs/ImaginaryRefExpr.hpp>

void gulc::DeclInstantiator::processFiles(std::vector<ASTFile>& files) {
    _files = &files;

    for (ASTFile& file : files) {
        _currentFile = &file;

        // Preprocess extensions
        for (ExtensionDecl* extension : file.scopeExtensions) {
            processExtensionDecl(llvm::dyn_cast<ExtensionDecl>(extension));
        }

        for (Decl* decl : file.declarations) {
            processDecl(decl);
        }

        handleDelayedInstantiationDecls();
    }
}

gulc::TemplateStructInstDecl* gulc::DeclInstantiator::instantiateTemplateStruct(ASTFile* file,
                                                                                gulc::TemplateStructDecl* templateStructDecl,
                                                                                std::vector<Expr*>& templateArguments,
                                                                                std::string const& errorMessageName,
                                                                                TextPosition errorStartPosition,
                                                                                TextPosition errorEndPosition) {
    _currentFile = file;

    if (!templateStructDecl->contractsAreInstantiated) {
        processTemplateStructDeclContracts(templateStructDecl);
    }

    if (!templateStructDecl->isInstantiated) {
        printError("[INTERNAL] template `" + templateStructDecl->identifier().name() + "` is not instantiated, cannot create instantiations from it!",
                   errorStartPosition, errorEndPosition);
    }

    if (templateStructDecl->containedInTemplate) {
        printError("[INTERNAL] template `" + errorMessageName + "` is contained within a template, this cannot be instantiated!",
                   errorStartPosition, errorEndPosition);
    }

    if (!ConstExprHelper::templateArgumentsAreSolved(templateArguments)) {
        printError("[INTERNAL] template `" + errorMessageName + "` does NOT have solved template parameters!",
                   errorStartPosition, errorEndPosition);
    }

    // Output an easy to read error message for any `where...` failures
    errorOnWhereContractFailure(templateStructDecl->contracts(), templateArguments,
                                templateStructDecl->templateParameters(), errorMessageName,
                                errorStartPosition, errorEndPosition);

    TemplateStructInstDecl* templateStructInstDecl = nullptr;
    templateStructDecl->getInstantiation(templateArguments, &templateStructInstDecl);

    // NOTE: It is possible the template instantiation has already been instantiated.
    if (!templateStructInstDecl->isInstantiated) {
        processTemplateStructInstDecl(templateStructInstDecl);
    }

    handleDelayedInstantiationDecls();

    return templateStructInstDecl;
}

gulc::TemplateTraitInstDecl* gulc::DeclInstantiator::instantiateTemplateTrait(ASTFile* file,
                                                                              gulc::TemplateTraitDecl* templateTraitDecl,
                                                                              std::vector<Expr*>& templateArguments,
                                                                              std::string const& errorMessageName,
                                                                              TextPosition errorStartPosition,
                                                                              TextPosition errorEndPosition) {
    _currentFile = file;

    if (!templateTraitDecl->contractsAreInstantiated) {
        processTemplateTraitDeclContracts(templateTraitDecl);
    }

    if (!templateTraitDecl->isInstantiated) {
        printError("[INTERNAL] template `" + templateTraitDecl->identifier().name() + "` is not instantiated, cannot create instantiations from it!",
                   errorStartPosition, errorEndPosition);
    }

    if (templateTraitDecl->containedInTemplate) {
        printError("[INTERNAL] template `" + errorMessageName + "` is contained within a template, this cannot be instantiated!",
                   errorStartPosition, errorEndPosition);
    }

    if (!ConstExprHelper::templateArgumentsAreSolved(templateArguments)) {
        printError("[INTERNAL] template `" + errorMessageName + "` does NOT have solved template parameters!",
                   errorStartPosition, errorEndPosition);
    }

    // Output an easy to read error message for any `where...` failures
    errorOnWhereContractFailure(templateTraitDecl->contracts(), templateArguments,
                                templateTraitDecl->templateParameters(), errorMessageName,
                                errorStartPosition, errorEndPosition);

    TemplateTraitInstDecl* templateTraitInstDecl = nullptr;
    templateTraitDecl->getInstantiation(templateArguments, &templateTraitInstDecl);

    // NOTE: It is possible the template instantiation has already been instantiated.
    if (!templateTraitInstDecl->isInstantiated) {
        processTemplateTraitInstDecl(templateTraitInstDecl);
    }

    handleDelayedInstantiationDecls();

    return templateTraitInstDecl;
}

gulc::TemplateFunctionInstDecl* gulc::DeclInstantiator::instantiateTemplateFunction(ASTFile* file,
                                                                                    gulc::TemplateFunctionDecl* templateFunctionDecl,
                                                                                    std::vector<Expr*>& templateArguments,
                                                                                    std::string const& errorMessageName,
                                                                                    TextPosition errorStartPosition,
                                                                                    TextPosition errorEndPosition) {
    _currentFile = file;

    if (!templateFunctionDecl->contractsAreInstantiated) {
        processTemplateFunctionDeclContracts(templateFunctionDecl);
    }

    // TODO: Do we need to handle this? I don't think this function will ever be called in a scenario where the
    //       template hasn't already been instantiated since this function should only be called AFTER
    //       `DeclInstantiator` has already finished
//    if (!templateFunctionDecl->isInstantiated) {
//        printError("[INTERNAL] template `" + templateFunctionDecl->identifier().name() + "` is not instantiated, cannot create instantiations from it!",
//                   errorStartPosition, errorEndPosition);
//    }

    if (templateFunctionDecl->containedInTemplate) {
        printError("[INTERNAL] template `" + errorMessageName + "` is contained within a template, this cannot be instantiated!",
                   errorStartPosition, errorEndPosition);
    }

    if (!ConstExprHelper::templateArgumentsAreSolved(templateArguments)) {
        printError("[INTERNAL] template `" + errorMessageName + "` does NOT have solved template parameters!",
                   errorStartPosition, errorEndPosition);
    }

    // Output an easy to read error message for any `where...` failures
    errorOnWhereContractFailure(templateFunctionDecl->contracts(), templateArguments,
                                templateFunctionDecl->templateParameters(), errorMessageName,
                                errorStartPosition, errorEndPosition);

    TemplateFunctionInstDecl* templateFunctionInstDecl = nullptr;
    templateFunctionDecl->getInstantiation(templateArguments, &templateFunctionInstDecl);

    // NOTE: It is possible the template instantiation has already been instantiated.
    if (!templateFunctionInstDecl->isInstantiated) {
        processTemplateFunctionInstDecl(templateFunctionInstDecl);
    }

    handleDelayedInstantiationDecls();

    return templateFunctionInstDecl;
}

void gulc::DeclInstantiator::handleDelayedInstantiationDecls() {
    while (!_delayInstantiationDecls.empty()) {
        Decl* delayedInstantiation = _delayInstantiationDecls.front();
        _delayInstantiationDecls.pop();

        switch (delayedInstantiation->getDeclKind()) {
            case Decl::Kind::TemplateStructInst: {
                auto templateStructInstDecl = llvm::dyn_cast<TemplateStructInstDecl>(delayedInstantiation);

                if (!templateStructInstDecl->isInstantiated) {
                    processTemplateStructInstDecl(templateStructInstDecl);
                }

                break;
            }
            case Decl::Kind::TemplateTraitInst: {
                auto templateTraitInstDecl = llvm::dyn_cast<TemplateTraitInstDecl>(delayedInstantiation);

                if (!templateTraitInstDecl->isInstantiated) {
                    processTemplateTraitInstDecl(templateTraitInstDecl);
                }

                break;
            }
            default:
                printError("[INTERNAL ERROR] unknown decl found in the delayed instantiation list!",
                           delayedInstantiation->startPosition(), delayedInstantiation->endPosition());
                break;
        }
    }

    _delayInstantiationDeclsSet.clear();
}

void gulc::DeclInstantiator::printError(std::string const& message, gulc::TextPosition startPosition,
                                        gulc::TextPosition endPosition) const {
    std::cerr << "gulc error[" << _filePaths[_currentFile->sourceFileID] << ", "
                           "{" << startPosition.line << ", " << startPosition.column << " "
                           "to " << endPosition.line << ", " << endPosition.column << "}]: "
              << message << std::endl;
    std::exit(1);
}

void gulc::DeclInstantiator::printWarning(std::string const& message, gulc::TextPosition startPosition,
                                          gulc::TextPosition endPosition) const {
    std::cout << "gulc warning[" << _filePaths[_currentFile->sourceFileID] << ", "
                             "{" << startPosition.line << ", " << startPosition.column << " "
                             "to " << endPosition.line << ", " << endPosition.column << "}]: "
              << message << std::endl;
}

bool gulc::DeclInstantiator::resolveType(gulc::Type*& type, bool delayInstantiation, bool containDependents) {
    if (llvm::isa<AliasType>(type)) {
        auto aliasType = llvm::dyn_cast<AliasType>(type);
        Type* resultType = aliasType->decl()->getInstantiation({});

        // We have to process the alias type value again...
        bool resultBool = resolveType(resultType, delayInstantiation);

        if (resultBool) {
            delete type;
            type = resultType;
            return true;
        } else {
            delete resultType;
            return false;
        }
    } else if (llvm::isa<DependentType>(type)) {
        auto dependentType = llvm::dyn_cast<DependentType>(type);

        if (resolveType(dependentType->container, true)) {
            // TODO: Are there any other types we need to support?
            switch (dependentType->container->getTypeKind()) {
                case Type::Kind::Struct: {
                    auto structType = llvm::dyn_cast<StructType>(dependentType->container);

                    if (TypeHelper::reresolveDependentWithinDecl(dependentType->dependent, structType->decl())) {
                        auto newType = dependentType->dependent;
                        dependentType->dependent = nullptr;
                        delete type;
                        type = newType;

                        return resolveType(type, delayInstantiation);
                    } else {
                        return false;
                    }
                }
                case Type::Kind::Trait: {
                    auto traitType = llvm::dyn_cast<TraitType>(dependentType->container);

                    if (TypeHelper::reresolveDependentWithinDecl(dependentType->dependent, traitType->decl())) {
                        auto newType = dependentType->dependent;
                        dependentType->dependent = nullptr;
                        delete type;
                        type = newType;

                        return resolveType(type, delayInstantiation);
                    } else {
                        return false;
                    }
                }
                default:
                    break;
            }

            return true;
        } else {
            return false;
        }
    } else if (llvm::isa<DimensionType>(type)) {
        auto dimensionType = llvm::dyn_cast<DimensionType>(type);

        return resolveType(dimensionType->nestedType, delayInstantiation);
    } else if (llvm::dyn_cast<UnresolvedNestedType>(type)) {
        auto nestedType = llvm::dyn_cast<UnresolvedNestedType>(type);

        // Process the template parameters and try to resolve any potential types in the list...
        for (Expr*& templateArgument : nestedType->templateArguments()) {
            processConstExpr(templateArgument);
        }

        // We delay the instantiation of our container as it isn't a requirement for us here
        // WARNING: Not delaying the instantiation WILL cause an internal error in some situations
        if (resolveType(nestedType->container, true)) {
            Type* fakeUnresolvedType = new UnresolvedType(nestedType->qualifier(), {},
                                                          nestedType->identifier(),
                                                          nestedType->templateArguments());

            // TODO: Are there any other kinds we should handle?
            switch (nestedType->container->getTypeKind()) {
                case Type::Kind::Struct: {
                    auto structType = llvm::dyn_cast<StructType>(nestedType->container);

                    if (TypeHelper::resolveTypeWithinDecl(fakeUnresolvedType, structType->decl())) {
                        // Clear the template parameters from `nestedType` and delete it, it was resolved.
                        nestedType->templateArguments().clear();
                        delete type;
                        type = fakeUnresolvedType;

                        // If we've resolved the nested type then we pass it back into our `resolveType` function
                        // again to try and further resolve down (if we don't do this then long, "super-nested" types
                        // won't get fully resolved)
                        return resolveType(type, delayInstantiation);
                    } else {
                        // Clear the template parameters from `fakeUnresolvedType` and delete it, it wasn't resolved
                        llvm::dyn_cast<UnresolvedType>(fakeUnresolvedType)->templateArguments.clear();
                        delete fakeUnresolvedType;
                    }

                    break;
                }
                case Type::Kind::Trait: {
                    auto traitType = llvm::dyn_cast<TraitType>(nestedType->container);

                    if (TypeHelper::resolveTypeWithinDecl(fakeUnresolvedType, traitType->decl())) {
                        // Clear the template parameters from `nestedType` and delete it, it was resolved.
                        nestedType->templateArguments().clear();
                        delete type;
                        type = fakeUnresolvedType;

                        // If we've resolved the nested type then we pass it back into our `resolveType` function
                        // again to try and further resolve down (if we don't do this then long, "super-nested" types
                        // won't get fully resolved)
                        return resolveType(type, delayInstantiation);
                    } else {
                        // Clear the template parameters from `fakeUnresolvedType` and delete it, it wasn't resolved
                        llvm::dyn_cast<UnresolvedType>(fakeUnresolvedType)->templateArguments.clear();
                        delete fakeUnresolvedType;
                    }

                    break;
                }
                case Type::Kind::TemplateStruct: {
                    auto templateStructType = llvm::dyn_cast<TemplateStructType>(nestedType->container);
                    Type* resolvedDependent = resolveDependentType(templateStructType->decl()->ownedMembers(),
                                                                   nestedType->identifier(),
                                                                   nestedType->templateArguments());

                    if (resolvedDependent == nullptr) {
                        return false;
                    } else {
                        auto newType = new DependentType(nestedType->qualifier(), nestedType->container, resolvedDependent);

                        // We steal the container
                        nestedType->container = nullptr;
                        // We steal the parameters
                        nestedType->templateArguments().clear();
                        delete nestedType;

                        type = newType;

                        return true;
                    }
                }
                case Type::Kind::TemplateTrait: {
                    auto templateTraitType = llvm::dyn_cast<TemplateTraitType>(nestedType->container);
                    Type* resolvedDependent = resolveDependentType(templateTraitType->decl()->ownedMembers(),
                                                                   nestedType->identifier(),
                                                                   nestedType->templateArguments());

                    if (resolvedDependent == nullptr) {
                        return false;
                    } else {
                        auto newType = new DependentType(nestedType->qualifier(), nestedType->container, resolvedDependent);

                        // We steal the container
                        nestedType->container = nullptr;
                        // We steal the parameters
                        nestedType->templateArguments().clear();
                        delete nestedType;

                        type = newType;

                        return true;
                    }
                }
                case Type::Kind::Dependent: {
                    // If the container was found to be a `Dependent` then we will convert ourselves to `Dependent`
                    auto dependentType = llvm::dyn_cast<DependentType>(nestedType->container)->dependent;
                    std::vector<Decl*>* checkDecls = nullptr;

                    switch (dependentType->getTypeKind()) {
                        case Type::Kind::Struct:
                            checkDecls = &llvm::dyn_cast<StructType>(dependentType)->decl()->ownedMembers();
                            break;
                        case Type::Kind::Trait:
                            checkDecls = &llvm::dyn_cast<TraitType>(dependentType)->decl()->ownedMembers();
                            break;
                        case Type::Kind::TemplateStruct:
                            checkDecls = &llvm::dyn_cast<TemplateStructType>(dependentType)->decl()->ownedMembers();
                            break;
                        case Type::Kind::TemplateTrait:
                            checkDecls = &llvm::dyn_cast<TemplateTraitType>(dependentType)->decl()->ownedMembers();
                            break;
                        default:
                            printError("[INTERNAL ERROR] unsupported container found for dependent type!",
                                       dependentType->startPosition(), dependentType->endPosition());
                            break;
                    }

                    Type* resolvedDependent = resolveDependentType(*checkDecls, nestedType->identifier(),
                                                                   nestedType->templateArguments());

                    if (resolvedDependent == nullptr) {
                        return false;
                    } else {
                        auto newType = new DependentType(nestedType->qualifier(), nestedType->container, resolvedDependent);

                        // We steal the container
                        nestedType->container = nullptr;
                        // We steal the parameters
                        nestedType->templateArguments().clear();
                        delete nestedType;

                        type = newType;

                        return true;
                    }
                }
                default:
                    break;
            }

            return true;
        } else {
            return false;
        }
    } else if (llvm::isa<PointerType>(type)) {
        auto pointerType = llvm::dyn_cast<PointerType>(type);

        return resolveType(pointerType->nestedType, delayInstantiation);
    } else if (llvm::isa<ReferenceType>(type)) {
        auto referenceType = llvm::dyn_cast<ReferenceType>(type);

        return resolveType(referenceType->nestedType, delayInstantiation);
    } else if (llvm::isa<TemplatedType>(type)) {
        auto templatedType = llvm::dyn_cast<TemplatedType>(type);

        // Process the template parameters and try to resolve any potential types in the list...
        for (Expr*& templateArgument : templatedType->templateArguments()) {
            processConstExpr(templateArgument);
        }

        // We fill these two lists based on the potential matches found within the `TemplatedType`
        // If there is more than one exact match then we error out due to ambiguity
        // If there are no exact matches but multiple partial matches then we error out for the same reason
        // TODO: We need to keep `namespaceDepth` into account (i.e. if there is an exact match at depth 0 AND depth 1
        //       then depth 0 shadows depth 1 so we DO NOT need to error due to ambiguity)
        //       Depth is 0 for in the same container, add 1 for every container until we reach the file. It is -1?
        //       for imports
        std::vector<TemplateDeclMatch> matches;

        for (Decl* checkDecl : templatedType->matchingTemplateDecls()) {
            bool declIsMatch = true;
            bool declIsExact = true;
            std::vector<std::size_t> argMatchStrengths;
            argMatchStrengths.resize(templatedType->templateArguments().size());

            switch (checkDecl->getDeclKind()) {
                case Decl::Kind::TemplateStruct: {
                    auto templateStructDecl = llvm::dyn_cast<TemplateStructDecl>(checkDecl);

                    compareDeclTemplateArgsToParams(templatedType->templateArguments(),
                                                    templateStructDecl->templateParameters(),
                                                    &declIsMatch, &declIsExact, argMatchStrengths);

                    // NOTE: Once we've reached this point the decl has been completely evaluated...

                    break;
                }
                case Decl::Kind::TemplateTrait: {
                    auto templateTraitDecl = llvm::dyn_cast<TemplateTraitDecl>(checkDecl);

                    compareDeclTemplateArgsToParams(templatedType->templateArguments(),
                                                    templateTraitDecl->templateParameters(),
                                                    &declIsMatch, &declIsExact, argMatchStrengths);

                    // NOTE: Once we've reached this point the decl has been completely evaluated...

                    break;
                }
                case Decl::Kind::TypeAlias: {
                    // TODO: I don't think we can call aliases here?
                    auto typeAliasDecl = llvm::dyn_cast<TypeAliasDecl>(checkDecl);

                    compareDeclTemplateArgsToParams(templatedType->templateArguments(),
                                                    typeAliasDecl->templateParameters(),
                                                    &declIsMatch, &declIsExact, argMatchStrengths);

                    // NOTE: Once we've reached this point the decl has been completely evaluated...

                    break;
                }
                default:
                    declIsMatch = false;
                    declIsExact = false;
                    printWarning("[INTERNAL] unknown template declaration found in `BaseResolver::resolveType`!",
                                 checkDecl->startPosition(), checkDecl->endPosition());
                    break;
            }

            if (declIsMatch) {
                auto match = TemplateDeclMatch(
                        declIsExact ? TemplateDeclMatch::Kind::Exact : TemplateDeclMatch::Kind::Castable,
                        checkDecl, argMatchStrengths);

                matches.push_back(match);
            }
        }

        if (matches.empty()) {
            printError("template type `" + templatedType->toString() + "` was not found for the provided parameters!",
                       templatedType->startPosition(), templatedType->endPosition());
        }

        Decl* foundTemplateDecl = nullptr;
        std::vector<std::size_t>* foundArgStrengths = nullptr;
        TemplateDeclMatch::Kind foundMatchKind = TemplateDeclMatch::Kind::Unknown;
        bool isAmbiguous = false;

        // TODO: I think we can treat argument specialization as if there were multiple nested templates.
        //       I think the way we could do this is as follows:
        //           struct Example<T: GreatGrandParent_A, U: Parent_B>
        //           struct Example<T: GrandParent_A, U: GreatGrandParent_B>
        //           let test: Example<Parent_A, Parent_B>
        //       In the above we do left-prioritization. Because `T == Parent_A` we choose the 2nd struct.
        //       It doesn't matter that `U == Parent_B` (making `U` a better match for the 1st struct)
        //       The first argument in the list gets priority, then the second, then third, etc.
        if (matches.size() == 1) {
            foundTemplateDecl = matches[0].decl;
            foundMatchKind = matches[0].kind;
        } else {
            for (TemplateDeclMatch& checkMatch : matches) {
                if (foundTemplateDecl == nullptr) {
                    foundTemplateDecl = checkMatch.decl;
                    foundMatchKind = checkMatch.kind;
                    foundArgStrengths = &checkMatch.argMatchStrengths;
                } else {
                    // Regardless of what kind the matches are if they are both the same we go through checking
                    // strengths from left to right.
                    if (foundMatchKind == checkMatch.kind) {
                        bool exactSame = true;

                        // We break on first difference. If the first different argument is closer to zero then we
                        // replace `foundMatch`, if it was worse then we continue searching. If the arguments have the
                        // exact same strength then we set `isAmbiguous` but keep searching in case something better
                        // comes along.
                        for (std::size_t i = 0; i < foundArgStrengths->size(); ++i) {
                            std::size_t foundStrength = (*foundArgStrengths)[i];
                            std::size_t checkStrength = checkMatch.argMatchStrengths[i];

                            if (checkStrength < foundStrength) {
                                foundTemplateDecl = checkMatch.decl;
                                foundMatchKind = checkMatch.kind;
                                foundArgStrengths = &checkMatch.argMatchStrengths;
                                isAmbiguous = false;

                                exactSame = false;
                                break;
                            } else if (checkStrength > foundStrength) {
                                exactSame = false;
                                break;
                            }
                        }

                        if (exactSame) {
                            isAmbiguous = true;
                        }
                    } else if (foundMatchKind == TemplateDeclMatch::Kind::Exact) {
                        // We continue the loop unless `checkMatch` is also exact, in that scenario the above code
                        // will execute instead.
                        continue;
                    } else if (foundMatchKind == TemplateDeclMatch::Kind::DefaultValues) {
                        // If we found a `DefaultValues` match then we follow these rules:
                        //  1. If the `checkMatch` is `Exact` we replace without question
                        //  2. If the `checkMatch` is also `DefaultValues` then we compare strengths
                        //  3. If the `checkMatch` is `Castable` we skip it
                        if (checkMatch.kind == TemplateDeclMatch::Kind::Exact) {
                            foundTemplateDecl = checkMatch.decl;
                            foundMatchKind = checkMatch.kind;
                            foundArgStrengths = &checkMatch.argMatchStrengths;
                            isAmbiguous = false;
                        } else if (checkMatch.kind == TemplateDeclMatch::Kind::DefaultValues) {
                            // In this scenario the matches are the same kind so this will never execute.
                            continue;
                        }
                    }

                    // We do nothing for `Castable` at this point.
                    // Castable only makes it past this loop on one of two conditions:
                    //  1. It is the only match in the list
                    //  2. There are multiple matches in the list and we choose the strongest match.
                }
            }
        }

        if (isAmbiguous) {
            printError("template type `" + templatedType->toString() + "` is ambiguous in the current context!",
                       templatedType->startPosition(), templatedType->endPosition());
        }

        // TODO: Do the casting here...

        // NOTE: I'm not sure if there is a better way to do this
        //       I'm just re-detecting what the Decl is for the template. It could be better to
        //       Make a generate "TemplateDecl" or something that will handle holding everything for
        //       "TemplateStructDecl", "TemplateTraitDecl", etc.
        switch (foundTemplateDecl->getDeclKind()) {
            case Decl::Kind::TemplateStruct: {
                auto templateStructDecl = llvm::dyn_cast<TemplateStructDecl>(foundTemplateDecl);

                if (!templateStructDecl->contractsAreInstantiated) {
                    processTemplateStructDeclContracts(templateStructDecl);
                }

                // Output an easy to read error message for any `where...` failures
                errorOnWhereContractFailure(templateStructDecl->contracts(), templatedType->templateArguments(),
                                            templateStructDecl->templateParameters(),
                                            templatedType->toString(),
                                            templatedType->startPosition(), templatedType->endPosition());

                // Append any missing default values to the end of the template argument list
                for (std::size_t i = templatedType->templateArguments().size();
                        i < templateStructDecl->templateParameters().size(); ++i) {
                    templatedType->templateArguments().push_back(
                            templateStructDecl->templateParameters()[i]->defaultValue->deepCopy());
                }

                if (!templateStructDecl->containedInTemplate &&
                    ConstExprHelper::templateArgumentsAreSolved(templatedType->templateArguments())) {
                    TemplateStructInstDecl* templateStructInstDecl = nullptr;
                    templateStructDecl->getInstantiation(templatedType->templateArguments(), &templateStructInstDecl);

                    // We add the template to the `delayInstantiation` list so that it is guaranteed to be instantiated
                    // If we don't do this and the actual template has already been instantiated then this `inst`
                    // will never be instantiated...
                    if (!templateStructInstDecl->isInstantiated) {
                        if (std::find(_delayInstantiationDeclsSet.begin(), _delayInstantiationDeclsSet.end(),
                                      templateStructInstDecl) == _delayInstantiationDeclsSet.end()) {
                            _delayInstantiationDeclsSet.insert(templateStructInstDecl);
                            _delayInstantiationDecls.push(templateStructInstDecl);
                        }
                    }

                    Type* newType = new StructType(templatedType->qualifier(), templateStructInstDecl,
                                                   templatedType->startPosition(), templatedType->endPosition());
                    delete type;
                    type = newType;
                } else {
                    // The template parameters weren't solved, we will return a `TemplateStructType` instead...
                    Type* newType = new TemplateStructType(templatedType->qualifier(),
                                                           templatedType->templateArguments(), templateStructDecl,
                                                           templatedType->startPosition(),
                                                           templatedType->endPosition());

                    if (templateStructDecl->containedInTemplate && containDependents) {
                        auto containerTemplateType = templateStructDecl->containerTemplateType->deepCopy();
                        newType = new DependentType(templatedType->qualifier(), containerTemplateType, newType);
                    }

                    // We steal the template parameters
                    templatedType->templateArguments().clear();
                    delete type;
                    type = newType;
                }

                return true;
            }
            case Decl::Kind::TemplateTrait: {
                auto templateTraitDecl = llvm::dyn_cast<TemplateTraitDecl>(foundTemplateDecl);

                if (!templateTraitDecl->contractsAreInstantiated) {
                    processTemplateTraitDeclContracts(templateTraitDecl);
                }

                // Output an easy to read error message for any `where...` failures
                errorOnWhereContractFailure(templateTraitDecl->contracts(), templatedType->templateArguments(),
                                            templateTraitDecl->templateParameters(), templatedType->toString(),
                                            templatedType->startPosition(), templatedType->endPosition());

                // Append any missing default values to the end of the template argument list
                for (std::size_t i = templatedType->templateArguments().size();
                        i < templateTraitDecl->templateParameters().size(); ++i) {
                    templatedType->templateArguments().push_back(
                            templateTraitDecl->templateParameters()[i]->defaultValue->deepCopy());
                }

                if (!templateTraitDecl->containedInTemplate &&
                    ConstExprHelper::templateArgumentsAreSolved(templatedType->templateArguments())) {
                    TemplateTraitInstDecl* templateTraitInstDecl = nullptr;
                    templateTraitDecl->getInstantiation(templatedType->templateArguments(), &templateTraitInstDecl);

                    // We add the template to the `delayInstantiation` list so that it is guaranteed to be instantiated
                    // If we don't do this and the actual template has already been instantiated then this `inst`
                    // will never be instantiated...
                    if (!templateTraitInstDecl->isInstantiated) {
                        if (std::find(_delayInstantiationDeclsSet.begin(), _delayInstantiationDeclsSet.end(),
                                      templateTraitInstDecl) == _delayInstantiationDeclsSet.end()) {
                            _delayInstantiationDeclsSet.insert(templateTraitInstDecl);
                            _delayInstantiationDecls.push(templateTraitInstDecl);
                        }
                    }

                    Type* newType = new TraitType(templatedType->qualifier(), templateTraitInstDecl,
                                                  templatedType->startPosition(), templatedType->endPosition());
                    delete type;
                    type = newType;
                } else {
                    // The template parameters weren't solved, we will return a `TemplateTraitType` instead...
                    Type* newType = new TemplateTraitType(templatedType->qualifier(),
                                                          templatedType->templateArguments(), templateTraitDecl,
                                                          templatedType->startPosition(),
                                                          templatedType->endPosition());

                    if (templateTraitDecl->containedInTemplate && containDependents) {
                        auto containerTemplateType = templateTraitDecl->containerTemplateType->deepCopy();
                        newType = new DependentType(templatedType->qualifier(), containerTemplateType, newType);
                    }

                    // We steal the template parameters
                    templatedType->templateArguments().clear();
                    delete type;
                    type = newType;
                }

                return true;
            }
            case Decl::Kind::TypeAlias: {
                auto typeAlias = llvm::dyn_cast<TypeAliasDecl>(foundTemplateDecl);

                // Append any missing default values to the end of the template argument list
                for (std::size_t i = templatedType->templateArguments().size();
                        i < typeAlias->templateParameters().size(); ++i) {
                    templatedType->templateArguments().push_back(
                            typeAlias->templateParameters()[i]->defaultValue->deepCopy());
                }

                // TODO: Should we apply the `qualifier`?
                // TODO: Should we handle checking if the alias is contained within a template?
                //       I think we MIGHT need to do what we did with template structs and template traits here...
                Type* newType = typeAlias->getInstantiation(templatedType->templateArguments());
                delete type;
                type = newType;
                // We pass the `newType` back into `resolveType` since it might be a `TemplatedType`
                return resolveType(newType, delayInstantiation);
            }
            default:
                printWarning("[INTERNAL] unknown template declaration found in `BaseResolver::resolveType`!",
                             foundTemplateDecl->startPosition(), foundTemplateDecl->endPosition());
                break;
        }
    } else if (llvm::isa<TemplateStructType>(type)) {
        auto templateStructType = llvm::dyn_cast<TemplateStructType>(type);

        if (ConstExprHelper::templateArgumentsAreSolved(templateStructType->templateArguments())) {
            TemplateStructInstDecl* templateStructInstDecl = nullptr;
            templateStructType->decl()->getInstantiation(templateStructType->templateArguments(),
                                                         &templateStructInstDecl);

            // We add the template to the `delayInstantiation` list so that it is guaranteed to be instantiated
            // If we don't do this and the actual template has already been instantiated then this `inst`
            // will never be instantiated...
            if (!templateStructInstDecl->isInstantiated) {
                if (std::find(_delayInstantiationDeclsSet.begin(), _delayInstantiationDeclsSet.end(),
                              templateStructInstDecl) == _delayInstantiationDeclsSet.end()) {
                    _delayInstantiationDeclsSet.insert(templateStructInstDecl);
                    _delayInstantiationDecls.push(templateStructInstDecl);
                }
            }

            Type* newType = new StructType(templateStructType->qualifier(), templateStructInstDecl,
                                           templateStructType->startPosition(), templateStructType->endPosition());
            delete type;
            type = newType;

            return true;
        } else {
            // TODO: Should we return false? Or should we make an enum for the return type?
            return true;
        }
    } else if (llvm::isa<TemplateTraitType>(type)) {
        auto templateTraitType = llvm::dyn_cast<TemplateTraitType>(type);

        if (ConstExprHelper::templateArgumentsAreSolved(templateTraitType->templateArguments())) {
            TemplateTraitInstDecl* templateTraitInstDecl = nullptr;
            templateTraitType->decl()->getInstantiation(templateTraitType->templateArguments(),
                                                        &templateTraitInstDecl);

            // We add the template to the `delayInstantiation` list so that it is guaranteed to be instantiated
            // If we don't do this and the actual template has already been instantiated then this `inst`
            // will never be instantiated...
            if (!templateTraitInstDecl->isInstantiated) {
                if (std::find(_delayInstantiationDeclsSet.begin(), _delayInstantiationDeclsSet.end(),
                              templateTraitInstDecl) == _delayInstantiationDeclsSet.end()) {
                    _delayInstantiationDeclsSet.insert(templateTraitInstDecl);
                    _delayInstantiationDecls.push(templateTraitInstDecl);
                }
            }

            Type* newType = new TraitType(templateTraitType->qualifier(), templateTraitInstDecl,
                                          templateTraitType->startPosition(), templateTraitType->endPosition());
            delete type;
            type = newType;

            return true;
        } else {
            // TODO: Should we return false? Or should we make an enum for the return type?
            return true;
        }
    } else if (llvm::isa<UnresolvedType>(type)) {
        printError("[INTERNAL ERROR] `UnresolvedType` found in `DeclInstantiator::resolveType`!",
                   type->startPosition(), type->endPosition());

        return false;
    } else {
        return true;
    }

    return false;
}

gulc::Type* gulc::DeclInstantiator::resolveDependentType(std::vector<Decl*>& checkDecls,
                                                         const gulc::Identifier& identifier,
                                                         std::vector<Expr*>& templateArguments) {
    bool templated = !templateArguments.empty();

    std::vector<Decl*> potentialTemplates;

    for (Decl* checkDecl : checkDecls) {
        if (checkDecl->identifier().name() != identifier.name()) {
            continue;
        }

        switch (checkDecl->getDeclKind()) {
            case Decl::Kind::TypeAlias: {
                auto checkAlias = llvm::dyn_cast<TypeAliasDecl>(checkDecl);

                if (checkAlias->hasTemplateParameters()) {
                    if (!templated) continue;

                    // TODO: Support optional template parameters
                    if (checkAlias->templateParameters().size() == templateArguments.size()) {
                        potentialTemplates.push_back(checkAlias);
                        // We keep searching as this might be the wrong type...
                    }
                } else {
                    if (templated) continue;

                    return new AliasType(Type::Qualifier::Unassigned, checkAlias, {}, {});
                }

                break;
            }
            case Decl::Kind::Enum: {
                if (templated) continue;

                return new EnumType(Type::Qualifier::Unassigned, llvm::dyn_cast<EnumDecl>(checkDecl), {}, {});
            }
            case Decl::Kind::Struct: {
                if (templated) continue;

                return new StructType(Type::Qualifier::Unassigned, llvm::dyn_cast<StructDecl>(checkDecl), {}, {});
            }
            case Decl::Kind::Trait: {
                if (templated) continue;

                return new TraitType(Type::Qualifier::Unassigned, llvm::dyn_cast<TraitDecl>(checkDecl), {}, {});
            }
            case Decl::Kind::TemplateStruct: {
                if (!templated) continue;

                auto checkTemplateStruct = llvm::dyn_cast<TemplateStructDecl>(checkDecl);

                if (checkTemplateStruct->templateParameters().size() == templateArguments.size()) {
                    potentialTemplates.push_back(checkTemplateStruct);
                    // We keep searching as this might be the wrong type...
                }
            }
            case Decl::Kind::TemplateTrait: {
                if (!templated) continue;

                auto checkTemplateTrait = llvm::dyn_cast<TemplateTraitDecl>(checkDecl);

                if (checkTemplateTrait->templateParameters().size() == templateArguments.size()) {
                    potentialTemplates.push_back(checkTemplateTrait);
                    // We keep searching as this might be the wrong type...
                }
            }
            default:
                continue;
        }
    }

    // If there aren't any potential templates at this point then the type wasn't found...
    if (potentialTemplates.empty()) {
        return nullptr;
    }

    Type* templatedType = new TemplatedType(Type::Qualifier::Unassigned, potentialTemplates,
                                            templateArguments, {}, {});

    // TODO: If the tempalted type is a type alias this will return the wrong stuff I believe.
    if (resolveType(templatedType, true, false)) {
        return templatedType;
    } else {
        llvm::dyn_cast<TemplatedType>(templatedType)->templateArguments().clear();
        delete templatedType;
        return nullptr;
    }
}

void gulc::DeclInstantiator::compareDeclTemplateArgsToParams(const std::vector<Expr*>& args,
                                                             const std::vector<TemplateParameterDecl*>& params,
                                                             bool* outIsMatch, bool* outIsExact,
                                                             std::vector<std::size_t>& outArgMatchStrengths) const {
    if (params.size() < args.size()) {
        // If there are more template parameters than parameters then we skip this Decl...
        *outIsMatch = false;
        *outIsExact = false;
        return;
    }

    *outIsMatch = true;
    // TODO: Once we support implicit casts and default template parameter values we need to set this to false
    //       when we implicit cast or use a default template parameter.
    *outIsExact = true;

    TypeCompareUtil typeCompareUtil;

    for (int i = 0; i < params.size(); ++i) {
        if (i >= args.size()) {
            // TODO: We need to differentiate between a `default-value-use` and `cast-needed` non-exact match
            *outIsMatch = true;
            *outIsExact = false;
            break;
        } else {
            outArgMatchStrengths[i] = 0;

            if (params[i]->templateParameterKind() == TemplateParameterDecl::TemplateParameterKind::Typename) {
                if (!llvm::isa<TypeExpr>(args[i])) {
                    // If the parameter is a `typename` then the argument MUST be a resolved type
                    *outIsMatch = false;
                    *outIsExact = false;
                    break;
                } else if (params[i]->type != nullptr) {
                    auto checkTypeExpr = llvm::dyn_cast<TypeExpr>(args[i]);

                    if (typeCompareUtil.compareAreSame(params[i]->type, checkTypeExpr->type)) {
                        // Exact match
                        outArgMatchStrengths[i] = 0;
                    } else {
                        if (llvm::isa<StructType>(params[i]->type) && llvm::isa<StructType>(checkTypeExpr->type)) {
                            // We have to search the argument type to see if it inherits the param type
                            // Example:
                            //     class View {}
                            //     class Window: View {}
                            //
                            //     struct box<T: View> {}
                            //
                            //     // Here we have to check if `Window` inherits `View` (which in this case is a simple
                            //     // search)
                            //     let rootView: box<Window>
                            auto searchStruct = llvm::dyn_cast<StructType>(checkTypeExpr->type);
                            auto findStruct = llvm::dyn_cast<StructType>(params[i]->type);
                            // We start the strength at `1` for inherited types, an exact match is `0`
                            std::size_t inheritanceStrength = 1;
                            bool foundMatch = false;

                            // TODO: If we haven't already validated that structs don't have circular references in
                            //       their inheritance list we will need to do that here...
                            for (StructDecl* checkStruct = searchStruct->decl()->baseStruct;
                                 checkStruct != nullptr;
                                 checkStruct = checkStruct->baseStruct, ++inheritanceStrength) {
                                if (checkStruct == findStruct->decl()) {
                                    foundMatch = true;
                                    break;
                                }
                            }

                            if (!foundMatch) {
                                *outIsMatch = false;
                                *outIsExact = false;
                                break;
                            }

                            outArgMatchStrengths[i] = inheritanceStrength;
                        } else {
                            // If the template type is specialized and there wasn't a match then the args don't match
                            // the template parameter list
                            *outIsMatch = false;
                            *outIsExact = false;
                            break;
                        }
                    }
                }
            } else {
                if (llvm::isa<TypeExpr>(args[i])) {
                    // If the parameter is a `const` then it MUST NOT be a type...
                    *outIsMatch = false;
                    *outIsExact = false;
                    break;
                } else if (llvm::isa<ValueLiteralExpr>(args[i])) {
                    auto valueLiteral = llvm::dyn_cast<ValueLiteralExpr>(args[i]);

                    if (!typeCompareUtil.compareAreSame(params[i]->type, valueLiteral->valueType)) {
                        // TODO: Support checking if an implicit cast is possible...
                        *outIsMatch = false;
                        *outIsExact = false;
                        break;
                    }
                } else {
                    printError("unsupported expression in template parameters list!",
                               args[i]->startPosition(), args[i]->endPosition());
                }
            }
        }
    }

    // NOTE: Once we've reached this point the decl has been completely evaluated...
}

void gulc::DeclInstantiator::errorOnWhereContractFailure(std::vector<Cont*> const& contracts,
                                                         std::vector<Expr*> const& args,
                                                         std::vector<TemplateParameterDecl*> const& params,
                                                         std::string const& errorMessageName,
                                                         TextPosition errorStartPosition,
                                                         TextPosition errorEndPosition) {
    for (Cont* contract : contracts) {
        if (llvm::isa<WhereCont>(contract)) {
            auto checkWhere = llvm::dyn_cast<WhereCont>(contract);

            ContractUtil contractUtil(_filePaths[_currentFile->sourceFileID], &params, &args);

            if (!contractUtil.checkWhereCont(checkWhere)) {
                // TODO: Once possible, we should output a better error message formatted as:
                //       struct Example<T>
                //           where T : Trait
                //                 ^^^^^^^^^
                //       type `i32` does not pass the annotated `where` contract for `Example<i32>`
                printError("instantiation failed for template `" + errorMessageName + "`, "
                           "`where " + checkWhere->condition->toString() + "` failed!",
                           errorStartPosition, errorEndPosition);
            }
        }
    }
}

void gulc::DeclInstantiator::processContract(gulc::Cont* contract) {
    switch (contract->getContKind()) {
        case Cont::Kind::Where: {
            auto whereCont = llvm::dyn_cast<WhereCont>(contract);

            if (!llvm::isa<CheckExtendsTypeExpr>(whereCont->condition)) {
                printError("currently only checking if a type extends another is supported by the `where` clause!",
                           whereCont->startPosition(), whereCont->endPosition());
            }

            processConstExpr(whereCont->condition);

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

void gulc::DeclInstantiator::processDecl(gulc::Decl* decl, bool isGlobal) {
    switch (decl->getDeclKind()) {
        case Decl::Kind::Import:
            // We skip imports, they're no longer useful here...
            break;

        case Decl::Kind::Enum:
            processEnumDecl(llvm::dyn_cast<EnumDecl>(decl));
            break;
        case Decl::Kind::Extension:
            // Extensions are preprocessed
//            processExtensionDecl(llvm::dyn_cast<ExtensionDecl>(decl));
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
            printError("INTERNAL ERROR - unhandled Decl type found in `DeclInstantiator`!",
                       decl->startPosition(), decl->endPosition());
            // If we don't know the declaration we just skip it, we don't care in this pass
            break;
    }
}

void gulc::DeclInstantiator::processPrototypeDecl(gulc::Decl* decl, bool isGlobal) {
    switch (decl->getDeclKind()) {
        case Decl::Kind::TraitPrototype:
            processTraitPrototypeDecl(llvm::dyn_cast<TraitPrototypeDecl>(decl));
            break;
        default:
            processDecl(decl, isGlobal);
            break;
    }
}

void gulc::DeclInstantiator::processEnumDecl(gulc::EnumDecl* enumDecl) {
    if (enumDecl->constType != nullptr) {
        if (!resolveType(enumDecl->constType)) {
            printError("enum base type `" + enumDecl->constType->toString() + "` was not found!",
                       enumDecl->startPosition(), enumDecl->endPosition());
        }
    }

    _workingDecls.push_back(enumDecl);

    for (EnumConstDecl* enumConst : enumDecl->enumConsts()) {
        processConstExpr(enumConst->constValue);
    }

    for (Decl* member : enumDecl->ownedMembers()) {
        processDecl(member, false);
    }

    _workingDecls.pop_back();
}

void gulc::DeclInstantiator::processExtensionDecl(gulc::ExtensionDecl* extensionDecl) {
    if (!resolveType(extensionDecl->typeToExtend)) {
        printError("extension type `" + extensionDecl->typeToExtend->toString() + "` was not found!",
                   extensionDecl->typeToExtend->startPosition(), extensionDecl->typeToExtend->endPosition());
    }

    for (Type*& inheritedType : extensionDecl->inheritedTypes()) {
        if (!resolveType(inheritedType)) {
            printError("extension inherited type `" + inheritedType->toString() + "` was not found!",
                       inheritedType->startPosition(), inheritedType->endPosition());
        }

        Type* processType = inheritedType;

        // We have to account for dependent types as there might be a niche scenario where they access a type within
        // a template using the template parameters
        // `extension<T> Example<T> : OtherTemplate<T>.TInnerTrait {}`
        if (llvm::isa<DependentType>(processType)) {
            processType = llvm::dyn_cast<DependentType>(processType)->dependent;
        }

        if (!llvm::isa<TraitType>(processType) && !llvm::isa<TemplateTraitType>(processType)) {
            printError("extensions can only inherit traits!",
                       inheritedType->startPosition(), inheritedType->endPosition());
        }
    }

    for (Decl* ownedMember : extensionDecl->ownedMembers()) {
        processDecl(ownedMember, false);
    }

    for (ConstructorDecl* constructor : extensionDecl->constructors()) {
        processDecl(constructor);
    }
}

void gulc::DeclInstantiator::processFunctionDecl(gulc::FunctionDecl* functionDecl) {
    for (ParameterDecl* parameter : functionDecl->parameters()) {
        processParameterDecl(parameter);
    }

    if (functionDecl->returnType != nullptr) {
        if (!resolveType(functionDecl->returnType)) {
            printError("function return type `" + functionDecl->returnType->toString() + "` was not found!",
                       functionDecl->returnType->startPosition(), functionDecl->returnType->endPosition());
        }
    }

    functionDecl->isInstantiated = true;
}

void gulc::DeclInstantiator::processNamespaceDecl(gulc::NamespaceDecl* namespaceDecl) {
    // Preprocess extensions
    for (ExtensionDecl* extension : namespaceDecl->scopeExtensions) {
        processExtensionDecl(llvm::dyn_cast<ExtensionDecl>(extension));
    }

    for (Decl* nestedDecl : namespaceDecl->nestedDecls()) {
        processDecl(nestedDecl);
    }
}

void gulc::DeclInstantiator::processParameterDecl(gulc::ParameterDecl* parameterDecl) {
    if (!resolveType(parameterDecl->type)) {
        printError("function parameter type `" + parameterDecl->type->toString() + "` was not found!",
                   parameterDecl->startPosition(), parameterDecl->endPosition());
    }

    if (parameterDecl->defaultValue != nullptr) {
        processConstExpr(parameterDecl->defaultValue);
    }
}

void gulc::DeclInstantiator::processPropertyDecl(gulc::PropertyDecl* propertyDecl) {
    if (!resolveType(propertyDecl->type)) {
        printError("property type `" + propertyDecl->type->toString() + "` was not found!",
                   propertyDecl->startPosition(), propertyDecl->endPosition());
    }

    for (PropertyGetDecl* getter : propertyDecl->getters()) {
        processDecl(getter);
    }

    if (propertyDecl->hasSetter()) {
        processDecl(propertyDecl->setter());
    }
}

void gulc::DeclInstantiator::processPropertyGetDecl(gulc::PropertyGetDecl* propertyGetDecl) {
    processFunctionDecl(propertyGetDecl);
}

void gulc::DeclInstantiator::processPropertySetDecl(gulc::PropertySetDecl* propertySetDecl) {
    processFunctionDecl(propertySetDecl);
}

void gulc::DeclInstantiator::processStructDeclInheritedTypes(gulc::StructDecl* structDecl) {
    // To prevent any circular references from causing an infinite loop, we set this here.
    // Since we only call `processStructDeclInheritedTypes` if this is false we will only have this called once, even if
    // another type down the line references us. To it we've already processed the inherited types...
    structDecl->inheritedTypesIsInitialized = true;

    for (Type*& inheritedType : structDecl->inheritedTypes()) {
        if (!resolveType(inheritedType)) {
            printError("struct inherited type `" + inheritedType->toString() + "` was not found!",
                       structDecl->startPosition(), structDecl->endPosition());
        }

        Type* processType = inheritedType;

        if (llvm::isa<DependentType>(processType)) {
            processType = llvm::dyn_cast<DependentType>(processType)->dependent;
        }

        if (llvm::isa<StructType>(processType)) {
            auto structType = llvm::dyn_cast<StructType>(processType);

            if (structDecl->baseStruct != nullptr) {
                printError("struct '" + structDecl->identifier().name() + "' cannot extend both '" +
                           structDecl->baseStruct->identifier().name() + "' and '" +
                           structType->decl()->identifier().name() +
                           "' at the same time! (both types are structs)",
                           structDecl->startPosition(), structDecl->endPosition());
            } else {
                structDecl->baseStruct = structType->decl();
            }

            if (!structType->decl()->inheritedTypesIsInitialized) {
                processStructDeclInheritedTypes(structType->decl());
            }
        } else if (llvm::isa<TraitType>(processType)) {
            auto traitType = llvm::dyn_cast<TraitType>(processType);

            if (!traitType->decl()->inheritedTypesIsInitialized) {
                processTraitDeclInheritedTypes(traitType->decl());
            }
        } else if (llvm::isa<TemplateStructType>(processType)) {
            auto templateStructType = llvm::dyn_cast<TemplateStructType>(processType);

            if (!templateStructType->decl()->contractsAreInstantiated) {
                processTemplateStructDeclContracts(templateStructType->decl());
            }

            if (!templateStructType->decl()->inheritedTypesIsInitialized) {
                processStructDeclInheritedTypes(templateStructType->decl());
            }
        } else if (llvm::isa<TemplateTraitType>(processType)) {
            auto templateTraitType = llvm::dyn_cast<TemplateTraitType>(processType);

            if (!templateTraitType->decl()->contractsAreInstantiated) {
                processTemplateTraitDeclContracts(templateTraitType->decl());
            }

            if (!templateTraitType->decl()->inheritedTypesIsInitialized) {
                processTraitDeclInheritedTypes(templateTraitType->decl());
            }
        } else {
            printError("structs and classes can only inherited other `struct`, `class`, or `trait` types!",
                       structDecl->startPosition(), structDecl->endPosition());
        }
    }

    // At this point we have the base struct and all other inherited types resolved so we can copy our base struct's
    // inherited members list...
    // TODO: We need to set `baseStruct` even when the base struct is a TemplateStructDecl!!!!!!!!!!===================
    //       This will most likely cause issues we will have to handle properly...
    if (structDecl->baseStruct != nullptr) {
        // TODO: We need to loop the base `allMembers` and check to make sure they're visible to `structDecl`
        structDecl->allMembers = structDecl->baseStruct->allMembers;
    }

    for (Decl* ownedMember : structDecl->ownedMembers()) {
        // We loop through our `inheritedMembers` list and either replace a `Decl` if it is shadowed/overridden
        // OR if the shadow/override wasn't found we append the `baseMember`
        bool wasOverrideOrShadow = false;

        // NOTE: We don't have to handle redeclarations here, that is handled in another pass.
        for (std::size_t i = 0; i < structDecl->allMembers.size(); ++i) {
            Decl* checkInherited = structDecl->allMembers[i];

            if (InheritUtil::overridesOrShadows(ownedMember, checkInherited)) {
                structDecl->allMembers[i] = ownedMember;
                // Perfect example or where named loops would be great... just `continue baseMemberLoop`
                wasOverrideOrShadow = true;
                break;
            }
        }

        if (!wasOverrideOrShadow) {
            structDecl->allMembers.push_back(ownedMember);
        }
    }

    // TODO: We need to loop our traits and add any predefined implementations to our `inheritedMembers`
    //       If we've implemented them ourselves then they will be ignored. If not, they'll be callable.
    // TODO: We need to trait default implementations within traits the same way we do templates. If we actually use
    //       the default implementation then we need to automatically re-implement it where `typeof self` == current struct
    for (Type* checkBase : structDecl->inheritedTypes()) {
        if (llvm::isa<TraitType>(checkBase)) {
            auto checkTrait = llvm::dyn_cast<TraitType>(checkBase);

            // TODO: Finish traits once we've started creating auto-specialized functions for default implementations
            //       NOTE: I think we can actually ignore adding them to the `inheritedMembers` list. Default
            //             implementations should be added to the `ownedMembers` once specialized to the struct.
        }
    }
}

void gulc::DeclInstantiator::processStructDecl(gulc::StructDecl* structDecl, bool calculateSizeAndVTable) {
    // The struct could already have been instantiated. We skip any that have already been processed...
    if (structDecl->isInstantiated) {
        return;
    }

    _workingDecls.push_back(structDecl);

    // The inherited types might have already been processed before this point...
    if (!structDecl->inheritedTypesIsInitialized) {
        processStructDeclInheritedTypes(structDecl);
    }

    for (Decl* member : structDecl->ownedMembers()) {
        processDecl(member, false);

        // If the member is a variable we have to make sure it isn't using the containing struct as a value type...
        // (If we don't then this will create an infinite loop when trying to generate the struct's memory layout)
        if (llvm::isa<VariableDecl>(member)) {
            auto checkVariable = llvm::dyn_cast<VariableDecl>(member);

            if (llvm::dyn_cast<StructType>(checkVariable->type)) {
                auto checkStructType = llvm::dyn_cast<StructType>(checkVariable->type);

                if (checkStructType->decl() == structDecl) {
                    printError("struct members CANNOT be the same type as the structs they are in! (did you mean to make `" + checkVariable->identifier().name() + "` a pointer or reference instead?)",
                               checkVariable->startPosition(), checkVariable->endPosition());
                }

                if (structUsesStructTypeAsValue(structDecl, checkStructType->decl(), true)) {
                    printError("type of member variable `" + checkVariable->identifier().name() + "` uses the current struct `" + structDecl->identifier().name() + "` by value in it's members or inheritance, this creates an illegal circular reference!",
                               checkVariable->startPosition(), checkVariable->endPosition());
                }
            }
        }
    }

    // Make sure we process the constructors and destructor since they are NOT in the `ownedMembers` list...
    for (ConstructorDecl* constructor : structDecl->constructors()) {
        processDecl(constructor);

        switch (constructor->constructorType()) {
            case gulc::ConstructorType::Move:
                if (structDecl->cachedMoveConstructor == nullptr) {
                    structDecl->cachedMoveConstructor = constructor;
                } else {
                    printError("there can only be one `move` constructor!",
                               constructor->startPosition(), constructor->endPosition());
                }
                break;
            case gulc::ConstructorType::Copy:
                if (structDecl->cachedCopyConstructor == nullptr) {
                    structDecl->cachedCopyConstructor = constructor;
                } else {
                    printError("there can only be one `copy` constructor!",
                               constructor->startPosition(), constructor->endPosition());
                }
                break;
            case gulc::ConstructorType::Normal: {
                // If the current default constructor has parameters then we keep looking for the 0 parameter default
                // constructor that would be the exact match.
                // NOTE: We don't do any ambiguity checks or duplication checks here, we'll let the generic constructor
                //       duplication checker handle that.
                if (structDecl->cachedDefaultConstructor == nullptr ||
                    structDecl->cachedDefaultConstructor->parameters().size() > 0) {
                    if (constructor->parameters().empty()) {
                        // This is the exact default constructor
                        structDecl->cachedDefaultConstructor = constructor;
                    } else if (structDecl->cachedDefaultConstructor == nullptr &&
                               constructor->parameters()[0]->defaultValue != nullptr) {
                        // If the first parameter has a default value then ALL parameters have a default value. This
                        // is the alternative default constructor for if the empty default is missing
                        // NOTE: We can only set `structDecl->cachedDefaultConstructor` if it is null in this case.
                        //       If it isn't null then we assume it is already the exact match.
                        structDecl->cachedDefaultConstructor = constructor;
                    }
                }
                break;
            }
            default:
                break;
        }
    }

    if (structDecl->destructor != nullptr) {
        processDecl(structDecl->destructor);
    }

    if (structDecl->baseStruct != nullptr) {
        // We have to check that the base struct doesn't implement the current struct as a value type
        if (structUsesStructTypeAsValue(structDecl, structDecl->baseStruct, true)) {
            printError("cannot extend from base type `" + structDecl->baseStruct->identifier().name() + "`, base type uses current struct `" + structDecl->identifier().name() + "` by value which is illegal!",
                       structDecl->startPosition(), structDecl->endPosition());
        }

        // TODO: Set the base constructor call. If `baseStruct` isn't null.
    }

    // NOTE: If `cachedDefaultConstructor` is missing then we create a default constructor that keeps track that it is
    //       auto-generated. We can't do any further processing at this point because we need all other types to have
    //       `cachedDefaultConstructor` set for us to reference. Once that is true we will generate the default
    //       contents of the constructors and mark any constructors that have to be "deleted" due to not being complete.
    //       THEN we can create errors for if any constructors are deleted for any reason.
    if (structDecl->cachedDefaultConstructor == nullptr) {
        structDecl->cachedDefaultConstructor = new ConstructorDecl(
                structDecl->sourceFileID(), {},
                Decl::Visibility::Public, false,
                Identifier({}, {}, "init"),
                DeclModifiers::None, {}, nullptr, {},
                new CompoundStmt({}, {}, {}), {}, {},
                ConstructorType::Normal
            );
        structDecl->cachedDefaultConstructor->isAutoGenerated = true;
        structDecl->cachedDefaultConstructor->container = structDecl;

        // We have to add it to the list of constructors so it can be called normally and so it is properly deleted
        // when the struct is deleted
        structDecl->constructors().push_back(structDecl->cachedDefaultConstructor);
    }

    if (structDecl->cachedMoveConstructor == nullptr) {
        std::vector<ParameterDecl*> moveParameters = {
                new ParameterDecl(
                        structDecl->sourceFileID(), {},
                        Identifier({}, {}, "other"), Identifier({}, {}, "other"),
                        new ReferenceType(
                                Type::Qualifier::Unassigned,
                                new StructType(Type::Qualifier::Mut, structDecl, {}, {})),
                        nullptr, ParameterDecl::ParameterKind::Val, {}, {})
        };

        structDecl->cachedMoveConstructor = new ConstructorDecl(
                structDecl->sourceFileID(), {},
                Decl::Visibility::Public, false,
                Identifier({}, {}, "init"),
                DeclModifiers::None, moveParameters, nullptr, {},
                new CompoundStmt({}, {}, {}), {}, {},
                ConstructorType::Move
        );
        structDecl->cachedMoveConstructor->isAutoGenerated = true;
        structDecl->cachedMoveConstructor->container = structDecl;

        // We have to add it to the list of constructors so it can be called normally and so it is properly deleted
        // when the struct is deleted
        structDecl->constructors().push_back(structDecl->cachedMoveConstructor);
    }

    if (structDecl->cachedCopyConstructor == nullptr) {
        std::vector<ParameterDecl*> copyParameters = {
                new ParameterDecl(
                        structDecl->sourceFileID(), {},
                        Identifier({}, {}, "other"), Identifier({}, {}, "other"),
                        new ReferenceType(
                                Type::Qualifier::Unassigned,
                                new StructType(Type::Qualifier::Immut, structDecl, {}, {})),
                        nullptr, ParameterDecl::ParameterKind::Val, {}, {})
        };

        structDecl->cachedCopyConstructor = new ConstructorDecl(
                structDecl->sourceFileID(), {},
                Decl::Visibility::Public, false,
                Identifier({}, {}, "init"),
                DeclModifiers::None, copyParameters, nullptr, {},
                new CompoundStmt({}, {}, {}), {}, {},
                ConstructorType::Copy
        );
        structDecl->cachedCopyConstructor->isAutoGenerated = true;
        structDecl->cachedCopyConstructor->container = structDecl;

        // We have to add it to the list of constructors so it can be called normally and so it is properly deleted
        // when the struct is deleted
        structDecl->constructors().push_back(structDecl->cachedCopyConstructor);
    }

    // If there isn't a destructor we have to create one. If it is unneeded we can optimize it out later
    if (structDecl->destructor == nullptr) {
        DeclModifiers destructorModifiers = DeclModifiers::None;

        // If the base struct's destructor is virtual then we specify override by default
        if (structDecl->baseStruct != nullptr && structDecl->baseStruct->destructor->isAnyVirtual()) {
            destructorModifiers = DeclModifiers::Override;
        }

        structDecl->destructor = new DestructorDecl(structDecl->sourceFileID(), {}, Decl::Visibility::Public,
                                                    false, Identifier({}, {}, "deinit"),
                                                    destructorModifiers, {}, new CompoundStmt({}, {}, {}), {}, {});
        structDecl->destructor->container = structDecl;
    } else {
        // If the base struct's destructor is virtual we have to verify our own is override
        // (NOTE: This check is only needed for manually created destructors)
        if (structDecl->baseStruct != nullptr && structDecl->baseStruct->destructor->isAnyVirtual() &&
            !structDecl->destructor->isOverride()) {
            printError("struct `" + structDecl->identifier().name() + "`'s destructor must be marked override! "
                       "(base struct `" + structDecl->baseStruct->identifier().name() + "` is virtual)",
                       structDecl->destructor->startPosition(), structDecl->destructor->endPosition());
        }
    }

    // If the struct is a template we don't handle checking for circular dependencies. That is why the check for that
    // is nested inside `calculateSizeAndVTable`
    if (calculateSizeAndVTable && !structDecl->containedInTemplate) {
        // Since `structDecl->baseStruct` MUST be instantiated by this point we can just copy the vtable from it
        // and modify it here (if it has a vtable)
        if (structDecl->baseStruct != nullptr) {
            structDecl->vtable = structDecl->baseStruct->vtable;
            // We copy the `vtableOwner` as well BUT if the `vtableOwner` is null after this line it means the base
            // does NOT have a vtable, making us the `vtableOwner` if there are any virtual methods in our owned members
            structDecl->vtableOwner = structDecl->baseStruct->vtableOwner;
        }

        // We only have to loop our owned members for the vtable. If the base had a vtable we already have the table
        // from there.
        for (Decl* checkDecl : structDecl->ownedMembers()) {
            if (llvm::isa<FunctionDecl>(checkDecl)) {
                auto checkFunctionDecl = llvm::dyn_cast<FunctionDecl>(checkDecl);

                if (checkFunctionDecl->isAnyVirtual()) {
                    // NOTE: GUL allows shadowing on the vtable. If it isn't an `override` we don't check the existing
                    if (checkFunctionDecl->isOverride()) {
                        bool baseFunctionFound = false;

                        for (FunctionDecl*& vtableFunction : structDecl->vtable) {
                            if (SignatureComparer::compareFunctions(checkFunctionDecl, vtableFunction, false)) {
                                vtableFunction = checkFunctionDecl;
                                baseFunctionFound = true;
                                break;
                            }
                        }

                        if (!baseFunctionFound) {
                            printError("no base function found to override for virtual function `" +
                                       checkFunctionDecl->identifier().name() + "`!",
                                       checkFunctionDecl->startPosition(), checkFunctionDecl->endPosition());
                        }
                    } else {
                        // If the function isn't `override` we just add the function to the vtable. Shadow warnings
                        // should be handled in a later step with non-virtual shadow warnings.
                        structDecl->vtable.push_back(checkFunctionDecl);
                    }
                }
            }
        }

        if (!structDecl->vtable.empty() && structDecl->vtableOwner == nullptr) {
            // If the struct has a vtable but the owner hasn't been set then we are the vtable owner
            structDecl->vtableOwner = structDecl;
        }

        std::size_t sizeWithoutPadding = 0;

        // If the struct isn't null it will be instantiated and have its size set already by this point
        if (structDecl->baseStruct != nullptr) {
            sizeWithoutPadding = structDecl->baseStruct->dataSizeWithoutPadding;
        }

        // To make things easier and less verbose we add the vtable as a hidden member of the `ownedMembers`
        if (structDecl->vtableOwner == structDecl) {
            auto vtableMember = new VariableDecl(structDecl->sourceFileID(), {}, Decl::Visibility::Private, false,
                                                 Identifier({}, {}, "_"), DeclModifiers::None,
                                                 new VTableType(), nullptr, {}, {});
            structDecl->ownedMembers().insert(structDecl->ownedMembers().begin(), vtableMember);
        }

        for (Decl* checkDecl : structDecl->ownedMembers()) {
            // Only `VariableDecl` can affect the actual size of the struct.
            if (llvm::isa<VariableDecl>(checkDecl)) {
                auto dataMember = llvm::dyn_cast<VariableDecl>(checkDecl);
                auto sizeAndAlignment = SizeofUtil::getSizeAndAlignmentOf(_target, dataMember->type);

                std::size_t alignPadding = 0;

                // `align` can't be zero, `n % 0` is illegal since `n / 0` is illegal
                if (sizeAndAlignment.align != 0) {
                    alignPadding = sizeAndAlignment.align - (sizeWithoutPadding % sizeAndAlignment.align);

                    // Rather than deal with casting to a signed type and rearrange the above algorithm to prevent
                    // this from happening, we just check if the `alignPadding` is equal to the `align` and set
                    // `alignPadding` to zero if it happens
                    if (alignPadding == sizeAndAlignment.align) {
                        alignPadding = 0;
                    }
                }

                // If there is an alignment padding then we have to add hidden "_" members to pad the memory layout
                if (alignPadding > 0) {
                    auto i8Type = gulc::BuiltInType::get(Type::Qualifier::Immut, "i8", {}, {});
                    auto lengthLiteral = new ValueLiteralExpr(ValueLiteralExpr::LiteralType::Integer,
                                                              std::to_string(alignPadding), "", {}, {});
                    auto paddingType = new FlatArrayType(Type::Qualifier::Immut, i8Type, lengthLiteral);
                    auto paddingExpr = new VariableDecl(structDecl->sourceFileID(), {}, Decl::Visibility::Private,
                                                        false, Identifier({}, {}, "_"),
                                                        DeclModifiers::None,
                                                        paddingType, nullptr, {}, {});

                    structDecl->memoryLayout.push_back(paddingExpr);
                    // We have to add the padding expression here too so that it is deleted properly...
                    structDecl->ownedPaddingMembers.push_back(paddingExpr);
                }

                structDecl->memoryLayout.push_back(dataMember);

                sizeWithoutPadding += alignPadding;
                sizeWithoutPadding += sizeAndAlignment.size;
            }
        }

        structDecl->dataSizeWithoutPadding = sizeWithoutPadding;
        structDecl->dataSizeWithPadding = _target.alignofStruct() - (structDecl->dataSizeWithoutPadding % _target.alignofStruct());
    }

    _workingDecls.pop_back();

    structDecl->isInstantiated = true;
}

void gulc::DeclInstantiator::processSubscriptOperatorDecl(gulc::SubscriptOperatorDecl* subscriptOperatorDecl) {
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
        // This is a dirty and wasteful way to do this but to give `get` and `set` their parameters we just copy the
        // part declarations parameters into their parameter list.
        for (ParameterDecl* parameterDecl : subscriptOperatorDecl->parameters()) {
            getter->parameters().push_back(llvm::dyn_cast<ParameterDecl>(parameterDecl->deepCopy()));
        }

        processDecl(getter);
    }

    if (subscriptOperatorDecl->hasSetter()) {
        // NOTE: `set` will already have the `value` parameter.
        auto& setterParams = subscriptOperatorDecl->setter()->parameters();

        // This is a dirty and wasteful way to do this but to give `get` and `set` their parameters we just copy the
        // part declarations parameters into their parameter list.
        for (ParameterDecl* parameterDecl : subscriptOperatorDecl->parameters()) {
            setterParams.push_back(llvm::dyn_cast<ParameterDecl>(parameterDecl->deepCopy()));
        }

        processDecl(subscriptOperatorDecl->setter());
    }
}

void gulc::DeclInstantiator::processSubscriptOperatorGetDecl(gulc::SubscriptOperatorGetDecl* subscriptOperatorGetDecl) {
    processFunctionDecl(subscriptOperatorGetDecl);
}

void gulc::DeclInstantiator::processSubscriptOperatorSetDecl(gulc::SubscriptOperatorSetDecl* subscriptOperatorSetDecl) {
    processFunctionDecl(subscriptOperatorSetDecl);
}

void gulc::DeclInstantiator::processTemplateFunctionDecl(gulc::TemplateFunctionDecl* templateFunctionDecl) {
//    _workingDecls.push_back(templateFunctionDecl);

    for (TemplateParameterDecl* templateParameter : templateFunctionDecl->templateParameters()) {
        processTemplateParameterDecl(templateParameter);
    }

    processFunctionDecl(templateFunctionDecl);

//    _workingDecls.pop_back();
}

void gulc::DeclInstantiator::processTemplateFunctionDeclContracts(gulc::TemplateFunctionDecl* templateFunctionDecl) {
    templateFunctionDecl->contractsAreInstantiated = true;

    for (Cont*& contract : templateFunctionDecl->contracts()) {
        processContract(contract);

        if (llvm::isa<WhereCont>(contract)) {
            descriptTemplateParameterForWhereCont(llvm::dyn_cast<WhereCont>(contract));
        }
    }
}

void gulc::DeclInstantiator::processTemplateFunctionInstDecl(gulc::TemplateFunctionInstDecl* templateFunctionInstDecl) {
    // The struct could already have been instantiated. We skip any that have already been processed...
    if (templateFunctionInstDecl->isInstantiated) {
        return;
    }

    processFunctionDecl(templateFunctionInstDecl);
}

void gulc::DeclInstantiator::processTemplateParameterDecl(gulc::TemplateParameterDecl* templateParameterDecl) {
    // If the template parameter is a const then we have to process its underlying type
    if (templateParameterDecl->templateParameterKind() == TemplateParameterDecl::TemplateParameterKind::Const) {
        if (!resolveType(templateParameterDecl->type)) {
            printError("const template parameter type `" + templateParameterDecl->type->toString() + "` was not found!",
                       templateParameterDecl->startPosition(), templateParameterDecl->endPosition());
        }
    } else {
        // `typename` parameters don't have to have specialization types...
        if (templateParameterDecl->type != nullptr) {
            if (!resolveType(templateParameterDecl->type)) {
                printError("template parameter specialized type `" +
                           templateParameterDecl->type->toString() + "` was not found!",
                           templateParameterDecl->startPosition(), templateParameterDecl->endPosition());
            }

            if (llvm::isa<TraitType>(templateParameterDecl->type)) {
                printError("traits cannot be used for template parameter specialization!",
                           templateParameterDecl->startPosition(), templateParameterDecl->endPosition());
            }
        }
    }

    if (templateParameterDecl->defaultValue != nullptr) {
        processConstExpr(templateParameterDecl->defaultValue);
    }
}

void gulc::DeclInstantiator::processTemplateStructDecl(gulc::TemplateStructDecl* templateStructDecl) {
    // The struct could already have been instantiated. We skip any that have already been processed...
    if (templateStructDecl->isInstantiated) {
        return;
    }

    _workingDecls.push_back(templateStructDecl);

    for (TemplateParameterDecl* templateParameter : templateStructDecl->templateParameters()) {
        processTemplateParameterDecl(templateParameter);
    }

    processTemplateStructDeclContracts(templateStructDecl);

    _workingDecls.pop_back();

    // TODO: Here we should be creating `ImaginaryTypeDecl` and `ImaginaryType` to be used to create a
    //       `TemplateStructInstDecl` that is used solely for validating logic. `TemplateStructDecl` should no longer
    //       be treated as a normal struct from this point on. This should be what we do for ALL templates.
    //       For `const` template variables we should create `Imaginary` variables. These are normal `VariableDecl`s
    //       but are `const` by default. These should then be referenced normally with `VariableRefExpr`.
    asdasdasdasdasd
//    processStructDecl(templateStructDecl, false);
    templateStructDecl->isInstantiated = true;

    for (TemplateStructInstDecl* templateStructInstDecl : templateStructDecl->templateInstantiations()) {
        processTemplateStructInstDecl(templateStructInstDecl);
    }
}

void gulc::DeclInstantiator::processTemplateStructDeclContracts(gulc::TemplateStructDecl* templateStructDecl) {
    templateStructDecl->contractsAreInstantiated = true;

    for (Cont*& contract : templateStructDecl->contracts()) {
        processContract(contract);

        if (llvm::isa<WhereCont>(contract)) {
            descriptTemplateParameterForWhereCont(llvm::dyn_cast<WhereCont>(contract));
        }
    }
}

void gulc::DeclInstantiator::processTemplateStructInstDecl(gulc::TemplateStructInstDecl* templateStructInstDecl) {
    // The struct could already have been instantiated. We skip any that have already been processed...
    if (templateStructInstDecl->isInstantiated) {
        return;
    }

    processStructDecl(templateStructInstDecl);
}

void gulc::DeclInstantiator::processTemplateTraitDecl(gulc::TemplateTraitDecl* templateTraitDecl) {
    // The trait could already have been instantiated. We skip any that have already been processed...
    if (templateTraitDecl->isInstantiated) {
        return;
    }

    _workingDecls.push_back(templateTraitDecl);

    for (TemplateParameterDecl* templateParameter : templateTraitDecl->templateParameters()) {
        processTemplateParameterDecl(templateParameter);
    }

    processTemplateTraitDeclContracts(templateTraitDecl);

    _workingDecls.pop_back();

    processTraitDecl(templateTraitDecl);
    templateTraitDecl->isInstantiated = true;

    for (TemplateTraitInstDecl* templateTraitInstDecl : templateTraitDecl->templateInstantiations()) {
        processTemplateTraitInstDecl(templateTraitInstDecl);
    }
}

void gulc::DeclInstantiator::processTemplateTraitDeclContracts(gulc::TemplateTraitDecl* templateTraitDecl) {
    templateTraitDecl->contractsAreInstantiated = true;

    for (Cont*& contract : templateTraitDecl->contracts()) {
        processContract(contract);

        if (llvm::isa<WhereCont>(contract)) {
            descriptTemplateParameterForWhereCont(llvm::dyn_cast<WhereCont>(contract));
        }
    }
}

void gulc::DeclInstantiator::processTemplateTraitInstDecl(gulc::TemplateTraitInstDecl* templateTraitInstDecl) {
    // The trait could already have been instantiated. We skip any that have already been processed...
    if (templateTraitInstDecl->isInstantiated) {
        return;
    }

    processTraitDecl(templateTraitInstDecl);
}

void gulc::DeclInstantiator::processTraitDeclInheritedTypes(gulc::TraitDecl* traitDecl) {
    // To prevent any circular references from causing an infinite loop, we set this here.
    // Since we only call `processStructDeclInheritedTypes` if this is false we will only have this called once, even if
    // another type down the line references us. To it we've already processed the inherited types...
    traitDecl->inheritedTypesIsInitialized = true;

    for (Type*& inheritedType : traitDecl->inheritedTypes()) {
        if (!resolveType(inheritedType)) {
            printError("trait inherited type `" + inheritedType->toString() + "` was not found!",
                       traitDecl->startPosition(), traitDecl->endPosition());
        }

        Type* processType = inheritedType;

        if (llvm::isa<DependentType>(processType)) {
            processType = llvm::dyn_cast<DependentType>(processType)->dependent;
        }

        if (llvm::isa<TraitType>(processType)) {
            auto traitType = llvm::dyn_cast<TraitType>(processType);

            if (!traitType->decl()->inheritedTypesIsInitialized) {
                processTraitDeclInheritedTypes(traitType->decl());
            }
        } else if (llvm::isa<TemplateTraitType>(processType)) {
            auto templateTraitType = llvm::dyn_cast<TemplateTraitType>(processType);

            if (!templateTraitType->decl()->inheritedTypesIsInitialized) {
                processTraitDeclInheritedTypes(templateTraitType->decl());
            }
        } else {
            printError("traits can only extend other traits! (`" + inheritedType->toString() + "` is not a trait!)",
                       inheritedType->startPosition(), inheritedType->endPosition());
        }
    }

    for (Type* inheritedType : traitDecl->inheritedTypes()) {
        if (llvm::isa<TraitType>(inheritedType)) {
            auto inheritedTraitType = llvm::dyn_cast<TraitType>(inheritedType);
            auto inheritedTrait = inheritedTraitType->decl();

            // TODO: Loop members and add to the inherited members list
            // TODO: How will we handle shadows? Do we just consider them to be the same function here?
            //       I would LIKE to be able to do something like:
            //           func ToString::toString() -> string { return Self::toString(self) }
            //           func OtherTrait::toString() -> string { return Self::toString(self) }
            //           func toString() -> string { return "real `toString`" }
            //       As a way to allow users to fix their ambiguous inheritance AND as a potential way to both
            //       implement a virtual function while also overloading it:
            //           override func Stmt::getKind() -> Stmt::Kind { ... }
            //           @shadows(Stmt::getKind) // NOTE: The `shadows` attribute is WIP and just here for fun.
            //           virtual func getKind() -> Expr::Kind { ... }
            //       The above is something that would benefit the current compiler but requires a middle-man class to
            //       allow within C++. It is very niche but would be a great feature to have.
            // The trait must already have it's inherited members list filled so we will just loop them to create ours
            for (Decl* baseMember : inheritedTrait->allMembers) {
                // NOTE: Traits cannot define `virtual` members so they can only shadow.
                bool wasShadowed = false;

                for (std::size_t i = 0; i < traitDecl->allMembers.size(); ++i) {
                    Decl* checkInherited = traitDecl->allMembers[i];

                    if (InheritUtil::overridesOrShadows(baseMember, checkInherited)) {
                        traitDecl->allMembers[i] = baseMember;
                        // Perfect example or where named loops would be great... just `continue baseMemberLoop`
                        wasShadowed = true;
                        break;
                    }
                }

                if (!wasShadowed) {
                    traitDecl->allMembers.push_back(baseMember);
                }
            }
        }
    }

    for (Decl* ownedMember : traitDecl->ownedMembers()) {
        // NOTE: Traits cannot define `virtual` members so they can only shadow.
        bool wasShadowed = false;

        // NOTE: Redelcaration will be handled in another pass, we don't need to check that here.
        for (std::size_t i = 0; i < traitDecl->allMembers.size(); ++i) {
            Decl* checkInherited = traitDecl->allMembers[i];

            if (InheritUtil::overridesOrShadows(ownedMember, checkInherited)) {
                traitDecl->allMembers[i] = ownedMember;
                // Perfect example or where named loops would be great... just `continue baseMemberLoop`
                wasShadowed = true;
                break;
            }
        }

        if (!wasShadowed) {
            traitDecl->allMembers.push_back(ownedMember);
        }
    }
}

void gulc::DeclInstantiator::processTraitDecl(gulc::TraitDecl* traitDecl) {
    // The trait could already have been instantiated. We skip any that have already been processed...
    if (traitDecl->isInstantiated) {
        return;
    }

    _workingDecls.push_back(traitDecl);

    if (!traitDecl->inheritedTypesIsInitialized) {
        processTraitDeclInheritedTypes(traitDecl);
    }

    for (Decl* member : traitDecl->ownedMembers()) {
        processDecl(member, false);
    }

    _workingDecls.pop_back();

    traitDecl->isInstantiated = true;
}

void gulc::DeclInstantiator::processTraitPrototypeDecl(gulc::TraitPrototypeDecl* traitPrototypeDecl) {
    if (!resolveType(traitPrototypeDecl->traitType)) {
        printError("trait type `" + traitPrototypeDecl->traitType->toString() + "` was not found!",
                   traitPrototypeDecl->startPosition(), traitPrototypeDecl->endPosition());
    }

    if (!llvm::isa<TraitType>(traitPrototypeDecl->traitType)) {
        printError("type `" + traitPrototypeDecl->traitType->toString() + "` is not a trait!",
                   traitPrototypeDecl->startPosition(), traitPrototypeDecl->endPosition());
    }
}

void gulc::DeclInstantiator::processTypeAliasDecl(gulc::TypeAliasDecl* typeAliasDecl) {
    // TODO: Detect circular references with the potential for `typealias prefix ^<T> = ^T;` or something.
    for (TemplateParameterDecl* templateParameter : typeAliasDecl->templateParameters()) {
        processTemplateParameterDecl(templateParameter);
    }

    if (!resolveType(typeAliasDecl->typeValue)) {
        printError("aliased type `" + typeAliasDecl->typeValue->toString() + "` was not found!",
                   typeAliasDecl->startPosition(), typeAliasDecl->endPosition());
    }
}

void gulc::DeclInstantiator::processTypeAliasDeclContracts(gulc::TypeAliasDecl* typeAliasDecl) {
    printWarning("[INTERNAL WARNING] type aliases don't support contracts yet!",
                 typeAliasDecl->startPosition(), typeAliasDecl->endPosition());
}

void gulc::DeclInstantiator::processVariableDecl(gulc::VariableDecl* variableDecl, bool isGlobal) {
    // We do the `const` and `static` check in our validator now.
    if (!resolveType(variableDecl->type)) {
        printError("variable type `" + variableDecl->type->toString() + "` was not found!",
                   variableDecl->type->startPosition(), variableDecl->type->endPosition());
    }

    if (isGlobal && variableDecl->initialValue != nullptr) {
        processConstExpr(variableDecl->initialValue);
    }
}

void gulc::DeclInstantiator::descriptTemplateParameterForWhereCont(gulc::WhereCont* whereCont) {
    switch (whereCont->condition->getExprKind()) {
        case Expr::Kind::CheckExtendsType: {
            auto checkExtendsType = llvm::dyn_cast<CheckExtendsTypeExpr>(whereCont->condition);

            if (llvm::isa<TemplateTypenameRefType>(checkExtendsType->checkType)) {
                auto templateTypenameRefType = llvm::dyn_cast<TemplateTypenameRefType>(checkExtendsType->checkType);

                bool newTypeIsTrait = llvm::isa<TraitType>(checkExtendsType->extendsType) ||
                                      llvm::isa<TemplateTraitType>(checkExtendsType->extendsType);

                TypeCompareUtil typeCompareUtil;

                // Check for any issues such as a duplicate `where` or a `where` that
                for (Type* inheritedType : templateTypenameRefType->refTemplateParameter()->inheritedTypes) {
                    if (typeCompareUtil.compareAreSame(inheritedType, checkExtendsType->extendsType)) {
                        printError("duplicate `where` contract!",
                                   whereCont->startPosition(), whereCont->endPosition());
                    } else if (!newTypeIsTrait) {
                        // If the new type isn't a trait then we have to check if the `inheritedType` also isn't a trait
                        // if the `inheritedType` also isn't a trait then we error out, there can only be ONE non-trait
                        // type inherited
                        if (!llvm::isa<TraitType>(inheritedType) && !llvm::isa<TemplateTraitType>(inheritedType)) {
                            printError("there can only be one non-trait type specified by a `where` contract!",
                                       whereCont->startPosition(), whereCont->endPosition());
                        }
                    }
                }

                // At this point the type has been validated in the inheritance list and can be added...
                templateTypenameRefType->refTemplateParameter()->inheritedTypes.push_back(checkExtendsType->extendsType->deepCopy());
            }

            break;
        }
        default:
            printError("[INTERNAL ERROR] unhandled `where` contract condition!",
                       whereCont->startPosition(), whereCont->endPosition());
            break;
    }
}

std::vector<gulc::Expr*> gulc::DeclInstantiator::createImaginaryTemplateArguments(
        std::vector<TemplateParameterDecl*>& templateParameters, std::vector<Cont*>& contracts) {
    std::vector<Expr*> templateArguments;
    templateArguments.reserve(templateParameters.size());

    for (TemplateParameterDecl* templateParameter : templateParameters) {
        if (templateParameter->templateParameterKind() == TemplateParameterDecl::TemplateParameterKind::Const) {
            // We currently don't keep track of `where` constraints on `const` variables... So we just create an
            // imaginary reference and end processing there for this type of expression.
            auto imaginaryRefExpr = new ImaginaryRefExpr(templateParameter);
            imaginaryRefExpr->valueType = templateParameter->type->deepCopy();
            imaginaryRefExpr->valueType->setIsLValue(false);
            templateArguments.push_back(imaginaryRefExpr);
        } else {
            Type* baseType = templateParameter->type == nullptr ? nullptr : templateParameter->type->deepCopy();
            std::vector<TraitType*> inheritedTraits;
            std::vector<ConstructorDecl*> constructors;
            DestructorDecl* destructor = nullptr;
            std::vector<Decl*> ownedMembers;

            // For the rest of the type information we need to scan the `where` contracts for `has` operations.
            for (Cont* checkContract : contracts) {
                // We only care about the `where` constraints here.
                if (!llvm::isa<WhereCont>(checkContract)) continue;

                auto checkWhere = llvm::dyn_cast<WhereCont>(checkContract);

                if (llvm::isa<HasExpr>(checkWhere->condition)) {
                    auto checkHas = llvm::dyn_cast<HasExpr>(checkWhere->condition);

                    // We have to make sure the `has` is on a type expression referencing the `TemplateParameterDecl`
                    // as the type.
                    if (!llvm::isa<TypeExpr>(checkHas->expr)) continue;

                    auto checkType = llvm::dyn_cast<TypeExpr>(checkHas->expr);

                    if (!llvm::isa<TemplateTypenameRefType>(checkType->type)) continue;

                    auto checkTemplateTypenameRef = llvm::dyn_cast<TemplateTypenameRefType>(checkType->type);

                    // The type could still be referencing another parameter.
                    if (checkTemplateTypenameRef->refTemplateParameter() != templateParameter) continue;

                    // Here we need to take decl whatever is in `has {decl}` and use it to construct the imaginary type
                    switch (checkHas->decl->getDeclKind()) {
                        case Decl::Kind::TraitPrototype: {
                            auto addTraitPrototype = llvm::dyn_cast<TraitPrototypeDecl>(checkHas->decl);
                            inheritedTraits.push_back(
                                    llvm::dyn_cast<TraitType>(addTraitPrototype->traitType->deepCopy()));
                            break;
                        }
                        case Decl::Kind::Constructor:
                            constructors.push_back(llvm::dyn_cast<ConstructorDecl>(checkHas->decl->deepCopy()));
                            break;
                        case Decl::Kind::Destructor: {
                            // We don't handle errors here. They should be handled else where (and this probably will
                            // never be triggered) but just to be safe if we've already set the destructor then we
                            // delete it to prevent a memory leak.
                            delete destructor;

                            destructor = llvm::dyn_cast<DestructorDecl>(checkHas->decl->deepCopy());

                            break;
                        }
                        case Decl::Kind::Variable:
                        case Decl::Kind::Property:
                        case Decl::Kind::SubscriptOperator:
                        case Decl::Kind::Function:
                        case Decl::Kind::Operator:
                        case Decl::Kind::CallOperator:
                            ownedMembers.push_back(checkHas->decl->deepCopy());
                            break;
                        default:
                            break;
                    }
                }
            }

            auto imaginaryTypeDecl = new ImaginaryTypeDecl(templateParameter->sourceFileID(), {},
                                                           Decl::Visibility::Unassigned, false,
                                                           templateParameter->identifier(),
                                                           templateParameter->startPosition(),
                                                           templateParameter->endPosition(), baseType,
                                                           inheritedTraits, ownedMembers,
                                                           constructors, destructor);
            processImaginaryTypeDecl(imaginaryTypeDecl);

            auto imaginaryType = new ImaginaryType(Type::Qualifier::Unassigned, imaginaryTypeDecl,
                                                   templateParameter->startPosition(),
                                                   templateParameter->endPosition());
            templateArguments.push_back(new TypeExpr(imaginaryType));
        }
    }

    return templateArguments;
}

void gulc::DeclInstantiator::createValidationTemplateFunctionInstDecl(
        gulc::TemplateFunctionDecl* templateFunctionDecl) {
    if (templateFunctionDecl->validationInst != nullptr) {
        return;
    }

    std::vector<Expr*> templateArguments = createImaginaryTemplateArguments(
            templateFunctionDecl->templateParameters(), templateFunctionDecl->contracts());

    // TODO: We need a way to specify to the `getInstantiation` that this should NOT be added to the list. Or should it?
    TemplateFunctionInstDecl* templateFunctionInstDecl = nullptr;
    templateFunctionDecl->getInstantiation(templateArguments, &templateFunctionInstDecl);

    if (!templateFunctionInstDecl->isInstantiated) {
        processTemplateFunctionInstDecl(templateFunctionInstDecl);
    }
}

void gulc::DeclInstantiator::createValidationTemplateStructInstDecl(gulc::TemplateStructDecl* templateStructDecl) {
    if (templateStructDecl->validationInst != nullptr) {
        return;
    }
}

void gulc::DeclInstantiator::createValidationTemplateTraitInstDecl(gulc::TemplateTraitDecl* templateTraitDecl) {
    if (templateTraitDecl->validationInst != nullptr) {
        return;
    }
}

void gulc::DeclInstantiator::processImaginaryTypeDecl(gulc::ImaginaryTypeDecl* imaginaryTypeDecl) {
    // TODO: We need to do the same stuff we do on `StructDecl` and `TraitDecl`, inherit everything from the base type
    //       and inherited traits?
    if (llvm::isa<StructType>(imaginaryTypeDecl->baseType())) {
        // TODO: Is the struct instantiated? I think we will have to account for that.
        auto baseStructDecl = llvm::dyn_cast<StructType>(imaginaryTypeDecl->baseType())->decl();

        // Copy our base's `allMembers`
        imaginaryTypeDecl->allMembers = baseStructDecl->allMembers;
    } else if (llvm::isa<ImaginaryType>(imaginaryTypeDecl->baseType())) {
        // TODO: Is this possible? If so what should we do in this situation?
    }

    // TODO: While `baseType` is relatively straight forward, for the `inheritedTrait` we will have to manually go
    //       through ALL traits and create prototype clones of their members. Replacing all references to the trait
    //       with a reference to the `ImaginaryTypeDecl`... This might require a specialized class.

}

void gulc::DeclInstantiator::processDependantDecl(gulc::Decl* decl) {
    for (Decl* checkDecl : _workingDecls) {
        if (decl == checkDecl) {
            // TODO: We need to improve this error message by detecting where in the chain the circular reference is
            //       caused. My thoughts on this is if `decl == checkDecl` then we need to find what `checkDecl++` is
            //       as that is most likely the error causing Decl is.
            printError("[INTERNAL] circular dependency detected!",
                       decl->startPosition(), decl->endPosition());
        }
    }

    // NOTE: Template instantiation declarations are special cases that have to be processed differently...
    if (llvm::isa<TemplateStructInstDecl>(decl)) {
        // For the `TemplateStructInstDecl` we have to make sure the parent template has been processed first...
        auto templateStructInstDecl = llvm::dyn_cast<TemplateStructInstDecl>(decl);

        if (!templateStructInstDecl->parentTemplateStruct()->isInstantiated) {
            // If the parent hasn't been instantiated already then we pass to instantiate it (which will handle the
            // inst for us)
            processTemplateStructDecl(templateStructInstDecl->parentTemplateStruct());
        } else {
            // If not we will just instantiate the template instantiation...
            processTemplateStructInstDecl(templateStructInstDecl);
        }
    } else {
        // If we reach this point then there isn't any circular dependency, we process the Decl as usual.
        processDecl(decl);
    }
}

bool gulc::DeclInstantiator::structUsesStructTypeAsValue(gulc::StructDecl* structType, gulc::StructDecl* checkStruct,
                                                         bool checkBaseStruct) {
    if (checkStruct == structType) {
        // If the check struct IS struct type we return true
        return true;
    }

    for (StructDecl* circularReferenceCheck : _baseCheckStructDecls) {
        if (circularReferenceCheck == checkStruct) {
            // TODO: Make a better error message.
            printError("[INTERNAL] circular dependency found for struct `" + structType->identifier().name() + "`!",
                       structType->startPosition(), structType->endPosition());
        }
    }

    // If we have to check the base we can only do that if the struct has been instantiated...
    if (!checkStruct->isInstantiated) {
        processDependantDecl(checkStruct);
    }

    _baseCheckStructDecls.push_back(checkStruct);

    for (Decl* checkDecl : checkStruct->ownedMembers()) {
        if (llvm::isa<VariableDecl>(checkDecl)) {
            auto checkVariable = llvm::dyn_cast<VariableDecl>(checkDecl);

            if (llvm::isa<StructType>(checkVariable->type)) {
                auto checkStructType = llvm::dyn_cast<StructType>(checkVariable->type);

                if (checkStructType->decl() == structType) {
                    // If the member's type is `structType` we immediately stop searching and return true
                    _baseCheckStructDecls.pop_back();
                    return true;
                } else {
                    // If the decl isn't instantiated we have to instantiate it before calling the check
                    if (!checkStructType->decl()->isInstantiated) {
                        processDependantDecl(checkStructType->decl());
                    }

                    // If it isn't the `structType` we also have to check the type to see if it uses `structType`
                    if (structUsesStructTypeAsValue(structType, checkStructType->decl(), true)) {
                        _baseCheckStructDecls.pop_back();
                        return true;
                    }
                }
            }
        }
    }

    if (checkBaseStruct) {
        // If the base struct IS the struct type we return true
        if (checkStruct->baseStruct == structType) {
            _baseCheckStructDecls.pop_back();
            return true;
        }

        // We also have to check the base classes for it...
        for (StructDecl *checkBase = checkStruct->baseStruct; checkBase != nullptr;
             checkBase = checkBase->baseStruct) {
            // This is most likely impossible, but if the base hasn't been instantiated then we instantiate it so it
            // can be properly processed
            if (!checkBase->isInstantiated) {
                processDependantDecl(checkBase);
            }

            // If the base type uses `structType` we immediately return true...
            if (structUsesStructTypeAsValue(structType, checkBase, true)) {
                _baseCheckStructDecls.pop_back();
                return true;
            }
        }
    }

    _baseCheckStructDecls.pop_back();

    // If we reach this point then `checkStruct` doesn't implement `structType` in any way as a value type
    return false;
}

void gulc::DeclInstantiator::processStmt(gulc::Stmt* stmt) {
    switch (stmt->getStmtKind()) {
        case Stmt::Kind::Break:
            // There isn't anything we need to process with `break` here...
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
            // There isn't anything we need to process with `continue` here...
            break;
        case Stmt::Kind::DoCatch:
            processDoCatchStmt(llvm::dyn_cast<DoCatchStmt>(stmt));
            break;
        case Stmt::Kind::Fallthrough:
            // There isn't anything we need to process with `fallthrough` here...
            break;
        case Stmt::Kind::For:
            processForStmt(llvm::dyn_cast<ForStmt>(stmt));
            break;
        case Stmt::Kind::Goto:
            // There isn't anything we need to process with `goto` here...
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
        default:
            printError("[INTERNAL ERROR] unsupported `Stmt` found in `DeclInstantiator::processStmt`!",
                       stmt->startPosition(), stmt->endPosition());
    }
}

void gulc::DeclInstantiator::processCaseStmt(gulc::CaseStmt* caseStmt) {
    processExpr(caseStmt->condition);

    for (Stmt* statement : caseStmt->body) {
        processStmt(statement);
    }
}

void gulc::DeclInstantiator::processCatchStmt(gulc::CatchStmt* catchStmt) {
    // TODO: We need to make the catch variable accessible (it currently isn't included in the local variables list)
    if (catchStmt->hasExceptionType()) {
        if (!resolveType(catchStmt->exceptionType)) {
            printError("catch type `" + catchStmt->exceptionType->toString() + "` was not found!",
                       catchStmt->exceptionType->startPosition(), catchStmt->exceptionType->endPosition());
        }
    }

    processCompoundStmt(catchStmt->body());
}

void gulc::DeclInstantiator::processCompoundStmt(gulc::CompoundStmt* compoundStmt) {
    for (Stmt* statement : compoundStmt->statements) {
        processStmt(statement);
    }
}

void gulc::DeclInstantiator::processDoCatchStmt(gulc::DoCatchStmt* doCatchStmt) {
    processCompoundStmt(doCatchStmt->body());

    for (CatchStmt* catchStmt : doCatchStmt->catchStatements()) {
        processCatchStmt(catchStmt);
    }

    if (doCatchStmt->hasFinallyStatement()) {
        processCompoundStmt(doCatchStmt->finallyStatement());
    }
}

void gulc::DeclInstantiator::processForStmt(gulc::ForStmt* forStmt) {
    if (forStmt->init != nullptr) {
        processExpr(forStmt->init);
    }

    if (forStmt->condition != nullptr) {
        processExpr(forStmt->condition);
    }

    if (forStmt->iteration != nullptr) {
        processExpr(forStmt->iteration);
    }

    processCompoundStmt(forStmt->body());
}

void gulc::DeclInstantiator::processIfStmt(gulc::IfStmt* ifStmt) {
    processExpr(ifStmt->condition);
    processCompoundStmt(ifStmt->trueBody());

    if (ifStmt->hasFalseBody()) {
        // TODO: We need to validate this at some point to make sure it is either `if` or `compound`, nothing else is
        //       allowed
        processStmt(ifStmt->falseBody());
    }
}

void gulc::DeclInstantiator::processLabeledStmt(gulc::LabeledStmt* labeledStmt) {
    processStmt(labeledStmt->labeledStmt);
}

void gulc::DeclInstantiator::processRepeatWhileStmt(gulc::RepeatWhileStmt* repeatWhileStmt) {
    processCompoundStmt(repeatWhileStmt->body());
    processExpr(repeatWhileStmt->condition);
}

void gulc::DeclInstantiator::processReturnStmt(gulc::ReturnStmt* returnStmt) {
    if (returnStmt->returnValue != nullptr) {
        processExpr(returnStmt->returnValue);
    }
}

void gulc::DeclInstantiator::processSwitchStmt(gulc::SwitchStmt* switchStmt) {
    processExpr(switchStmt->condition);

    for (CaseStmt* caseStmt : switchStmt->cases) {
        processStmt(caseStmt);
    }
}

void gulc::DeclInstantiator::processWhileStmt(gulc::WhileStmt* whileStmt) {
    processExpr(whileStmt->condition);
    processCompoundStmt(whileStmt->body());
}

void gulc::DeclInstantiator::processConstExpr(gulc::Expr* expr) {
    switch (expr->getExprKind()) {
        case Expr::Kind::CheckExtendsType:
            processCheckExtendsTypeExpr(llvm::dyn_cast<CheckExtendsTypeExpr>(expr));
            break;
        case Expr::Kind::Type:
            processTypeExpr(llvm::dyn_cast<TypeExpr>(expr));
            break;
        case Expr::Kind::ValueLiteral:
            processValueLiteralExpr(llvm::dyn_cast<ValueLiteralExpr>(expr));
            break;
        default:
            printError("unsupported expression found where const expression was expected!",
                       expr->startPosition(), expr->endPosition());
            break;
    }
}

void gulc::DeclInstantiator::processExpr(gulc::Expr* expr) {
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
        case Expr::Kind::InfixOperator:
            processInfixOperatorExpr(llvm::dyn_cast<InfixOperatorExpr>(expr));
            break;
        case Expr::Kind::Is:
            processIsExpr(llvm::dyn_cast<IsExpr>(expr));
            break;
        case Expr::Kind::LabeledArgument:
            printError("argument label found outside of function or subscript call!",
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
        case Expr::Kind::SubscriptCall:
            processSubscriptCallExpr(llvm::dyn_cast<SubscriptCallExpr>(expr));
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
            printError("[INTERNAL ERROR] unsupported expression found in `DeclInstantiator::processExpr`!",
                       expr->startPosition(), expr->endPosition());
    }
}

void gulc::DeclInstantiator::processArrayLiteralExpr(gulc::ArrayLiteralExpr* arrayLiteralExpr) {
    for (Expr* index : arrayLiteralExpr->indexes) {
        processExpr(index);
    }
}

void gulc::DeclInstantiator::processAsExpr(gulc::AsExpr* asExpr) {
    processExpr(asExpr->expr);

    if (!resolveType(asExpr->asType)) {
        printError("type `" + asExpr->asType->toString() + "` was not found!",
                   asExpr->asType->startPosition(), asExpr->asType->endPosition());
    }
}

void gulc::DeclInstantiator::processAssignmentOperatorExpr(gulc::AssignmentOperatorExpr* assignmentOperatorExpr) {
    processExpr(assignmentOperatorExpr->leftValue);
    processExpr(assignmentOperatorExpr->rightValue);
}

void gulc::DeclInstantiator::processCheckExtendsTypeExpr(gulc::CheckExtendsTypeExpr* checkExtendsTypeExpr) {
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

void gulc::DeclInstantiator::processFunctionCallExpr(gulc::FunctionCallExpr* functionCallExpr) {
    processExpr(functionCallExpr->functionReference);

    for (LabeledArgumentExpr* labeledArgument : functionCallExpr->arguments) {
        processLabeledArgumentExpr(labeledArgument);
    }
}

void gulc::DeclInstantiator::processHasExpr(gulc::HasExpr* hasExpr) {
    processExpr(hasExpr->expr);
    processPrototypeDecl(hasExpr->decl, false);
}

void gulc::DeclInstantiator::processIdentifierExpr(gulc::IdentifierExpr* identifierExpr) {
    for (Expr* templateArgument : identifierExpr->templateArguments()) {
        processExpr(templateArgument);
    }
}

void gulc::DeclInstantiator::processInfixOperatorExpr(gulc::InfixOperatorExpr* infixOperatorExpr) {
    processExpr(infixOperatorExpr->leftValue);
    processExpr(infixOperatorExpr->rightValue);
}

void gulc::DeclInstantiator::processIsExpr(gulc::IsExpr* isExpr) {
    processExpr(isExpr->expr);

    if (!resolveType(isExpr->isType)) {
        printError("type `" + isExpr->isType->toString() + "` was not found!",
                   isExpr->isType->startPosition(), isExpr->isType->endPosition());
    }
}

void gulc::DeclInstantiator::processLabeledArgumentExpr(gulc::LabeledArgumentExpr* labeledArgumentExpr) {
    processExpr(labeledArgumentExpr->argument);
}

void gulc::DeclInstantiator::processMemberAccessCallExpr(gulc::MemberAccessCallExpr* memberAccessCallExpr) {
    processExpr(memberAccessCallExpr->objectRef);
}

void gulc::DeclInstantiator::processParenExpr(gulc::ParenExpr* parenExpr) {
    processExpr(parenExpr->nestedExpr);
}

void gulc::DeclInstantiator::processPostfixOperatorExpr(gulc::PostfixOperatorExpr* postfixOperatorExpr) {
    processExpr(postfixOperatorExpr->nestedExpr);
}

void gulc::DeclInstantiator::processPrefixOperatorExpr(gulc::PrefixOperatorExpr* prefixOperatorExpr) {
    processExpr(prefixOperatorExpr->nestedExpr);
}

void gulc::DeclInstantiator::processSubscriptCallExpr(gulc::SubscriptCallExpr* subscriptCallExpr) {
    processExpr(subscriptCallExpr->subscriptReference);

    for (LabeledArgumentExpr* labeledArgument : subscriptCallExpr->arguments) {
        processLabeledArgumentExpr(labeledArgument);
    }
}

void gulc::DeclInstantiator::processTemplateConstRefExpr(gulc::TemplateConstRefExpr* templateConstRefExpr) {
    // TODO: Do we need to do anything here?
}

void gulc::DeclInstantiator::processTernaryExpr(gulc::TernaryExpr* ternaryExpr) {
    processExpr(ternaryExpr->condition);
    processExpr(ternaryExpr->trueExpr);
    processExpr(ternaryExpr->falseExpr);
}

void gulc::DeclInstantiator::processTryExpr(gulc::TryExpr* tryExpr) {
    processExpr(tryExpr->nestedExpr);
}

void gulc::DeclInstantiator::processTypeExpr(gulc::TypeExpr* typeExpr) {
    if (!resolveType(typeExpr->type)) {
        printError("type `" + typeExpr->type->toString() + "` was not found!",
                   typeExpr->startPosition(), typeExpr->endPosition());
    }
}

void gulc::DeclInstantiator::processValueLiteralExpr(gulc::ValueLiteralExpr* valueLiteralExpr) const {
    if (valueLiteralExpr->hasSuffix()) {
        printError("type suffixes not yet supported!",
                   valueLiteralExpr->startPosition(), valueLiteralExpr->endPosition());
    }

    if (valueLiteralExpr->valueType == nullptr) {
        switch (valueLiteralExpr->literalType()) {
            case ValueLiteralExpr::LiteralType::Integer:
                valueLiteralExpr->valueType = BuiltInType::get(Type::Qualifier::Unassigned, "i32", {}, {});
                break;
            case ValueLiteralExpr::LiteralType::Float:
                valueLiteralExpr->valueType = BuiltInType::get(Type::Qualifier::Unassigned, "f64", {}, {});
                break;
            default:
                printError("only integer and floating literals are currently supported! (char and string coming soon)",
                           valueLiteralExpr->startPosition(), valueLiteralExpr->endPosition());
                break;
        }
    }
}

void gulc::DeclInstantiator::processVariableDeclExpr(gulc::VariableDeclExpr* variableDeclExpr) {
    if (variableDeclExpr->type != nullptr) {
        if (!resolveType(variableDeclExpr->type)) {
            printError("variable type `" + variableDeclExpr->type->toString() + "` was not found!",
                       variableDeclExpr->type->startPosition(), variableDeclExpr->type->endPosition());
        }
    } else {
        // TODO: Detect type from the initial value...
    }

    if (variableDeclExpr->initialValue != nullptr) {
        processExpr(variableDeclExpr->initialValue);
    }
}
