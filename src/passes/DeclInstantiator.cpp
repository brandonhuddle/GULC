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

void gulc::DeclInstantiator::processFiles(std::vector<ASTFile>& files) {
    for (ASTFile& file : files) {
        _currentFile = &file;

        for (Decl* decl : file.declarations) {
            processDecl(decl);
        }

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
    std::cerr << "gulc warning[" << _filePaths[_currentFile->sourceFileID] << ", "
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

        // Process the template arguments and try to resolve any potential types in the list...
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
                        // Clear the template arguments from `nestedType` and delete it, it was resolved.
                        nestedType->templateArguments().clear();
                        delete type;
                        type = fakeUnresolvedType;

                        // If we've resolved the nested type then we pass it back into our `resolveType` function
                        // again to try and further resolve down (if we don't do this then long, "super-nested" types
                        // won't get fully resolved)
                        return resolveType(type, delayInstantiation);
                    } else {
                        // Clear the template arguments from `fakeUnresolvedType` and delete it, it wasn't resolved
                        llvm::dyn_cast<UnresolvedType>(fakeUnresolvedType)->templateArguments.clear();
                        delete fakeUnresolvedType;
                    }

                    break;
                }
                case Type::Kind::Trait: {
                    auto traitType = llvm::dyn_cast<TraitType>(nestedType->container);

                    if (TypeHelper::resolveTypeWithinDecl(fakeUnresolvedType, traitType->decl())) {
                        // Clear the template arguments from `nestedType` and delete it, it was resolved.
                        nestedType->templateArguments().clear();
                        delete type;
                        type = fakeUnresolvedType;

                        // If we've resolved the nested type then we pass it back into our `resolveType` function
                        // again to try and further resolve down (if we don't do this then long, "super-nested" types
                        // won't get fully resolved)
                        return resolveType(type, delayInstantiation);
                    } else {
                        // Clear the template arguments from `fakeUnresolvedType` and delete it, it wasn't resolved
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
                        // We steal the arguments
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
                        // We steal the arguments
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
                        // We steal the arguments
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

        // Process the template arguments and try to resolve any potential types in the list...
        for (Expr*& templateArgument : templatedType->templateArguments()) {
            processConstExpr(templateArgument);
        }

        Decl* foundTemplateDecl = nullptr;
        bool isExact = false;
        bool isAmbiguous = false;
        WhereContractsResult whereContractsResult = WhereContractsResult::Failed;

        for (Decl* checkDecl : templatedType->matchingTemplateDecls()) {
            bool declIsMatch = true;
            bool declIsExact = true;
            WhereContractsResult declWhereContractsResult = WhereContractsResult::NoContracts;

            switch (checkDecl->getDeclKind()) {
                case Decl::Kind::TemplateStruct: {
                    auto templateStructDecl = llvm::dyn_cast<TemplateStructDecl>(checkDecl);

                    if (!templateStructDecl->contractsAreInstantiated) {
                        processTemplateStructDeclContracts(templateStructDecl);
                    }

                    compareDeclTemplateArgsToParams(templateStructDecl->contracts(),
                                                    templatedType->templateArguments(),
                                                    templateStructDecl->templateParameters(),
                                                    &declIsMatch, &declIsExact, &declWhereContractsResult);

                    // NOTE: Once we've reached this point the decl has been completely evaluated...

                    break;
                }
                case Decl::Kind::TemplateTrait: {
                    auto templateTraitDecl = llvm::dyn_cast<TemplateTraitDecl>(checkDecl);

                    if (!templateTraitDecl->contractsAreInstantiated) {
                        processTemplateTraitDeclContracts(templateTraitDecl);
                    }

                    compareDeclTemplateArgsToParams(templateTraitDecl->contracts(),
                                                    templatedType->templateArguments(),
                                                    templateTraitDecl->templateParameters(),
                                                    &declIsMatch, &declIsExact, &declWhereContractsResult);

                    // NOTE: Once we've reached this point the decl has been completely evaluated...

                    break;
                }
                case Decl::Kind::TypeAlias: {
                    auto typeAliasDecl = llvm::dyn_cast<TypeAliasDecl>(checkDecl);

//                    if (!typeAliasDecl->contractsAreInstantiated) {
//                        // TODO: Instantiate contracts
//                    }

                    compareDeclTemplateArgsToParams({}, // TODO: typeAliasDecl->contracts(),
                                                    templatedType->templateArguments(),
                                                    typeAliasDecl->templateParameters(),
                                                    &declIsMatch, &declIsExact, &declWhereContractsResult);

                    // NOTE: Once we've reached this point the decl has been completely evaluated...

                    break;
                }
                default:
                    declIsMatch = false;
                    declIsExact = false;
                    declWhereContractsResult = WhereContractsResult::Failed;
                    printWarning("[INTERNAL] unknown template declaration found in `BaseResolver::resolveType`!",
                                 checkDecl->startPosition(), checkDecl->endPosition());
                    break;
            }

            if (declIsMatch) {
                if (isExact) {
                    if (whereContractsResult == WhereContractsResult::Passed) {
                        // Only ambiguous if the new `decl` is exactly the same
                        if (declIsExact && declWhereContractsResult == WhereContractsResult::Passed) {
                            isAmbiguous = true;
                        }
                    } else if (whereContractsResult == WhereContractsResult::NoContracts) {
                        // Only upgrade if the new `decl` is `Passed`
                        if (declIsExact) {
                            if (declWhereContractsResult == WhereContractsResult::Passed) {
                                // Passed > no contracts
                                isAmbiguous = false;
                                foundTemplateDecl = checkDecl;
                                isExact = true;
                                whereContractsResult = WhereContractsResult::Passed;
                            } else if (declWhereContractsResult == WhereContractsResult::NoContracts) {
                                // The new decl is exactly the same as the old one we've already found...
                                isAmbiguous = true;
                            }
                        }
                    } else if (whereContractsResult == WhereContractsResult::Failed) {
                        // If the previous exact match was failed then it was stored for error messages, we replace it
                        isAmbiguous = false;
                        foundTemplateDecl = checkDecl;
                        isExact = declIsExact;
                        whereContractsResult = declWhereContractsResult;
                    }
                } else {
                    if (whereContractsResult == WhereContractsResult::Passed) {
                        // Only ambiguous if the new `decl` is exactly the same
                        if (declIsExact) {
                            if (whereContractsResult != WhereContractsResult::Failed) {
                                // If the new decl is exact and wasn't a failure we set it no matter what
                                isAmbiguous = false;
                                foundTemplateDecl = checkDecl;
                                isExact = declIsExact;
                                whereContractsResult = declWhereContractsResult;
                            }
                        } else {
                            if (declWhereContractsResult == WhereContractsResult::Passed) {
                                // The new decl is exactly the same as the old decl
                                isAmbiguous = true;
                            }
                        }
                    } else if (whereContractsResult == WhereContractsResult::NoContracts) {
                        if (declWhereContractsResult == WhereContractsResult::Passed) {
                            // If the contracts were passed then we set the decl even if it isn't an exact match
                            // (since the one we've already found isn't an exact match either...
                            isAmbiguous = false;
                            foundTemplateDecl = checkDecl;
                            isExact = declIsExact;
                            whereContractsResult = declWhereContractsResult;
                        } else if (declWhereContractsResult != WhereContractsResult::Failed) {
                            if (declIsExact) {
                                // If the new one didn't fail the contracts and is an exact match then we set the decl
                                isAmbiguous = false;
                                foundTemplateDecl = checkDecl;
                                isExact = declIsExact;
                                whereContractsResult = declWhereContractsResult;
                            } else if (declWhereContractsResult == WhereContractsResult::NoContracts) {
                                // If the new decl is also no contracts then it is exactly the same as the last one...
                                isAmbiguous = true;
                            }
                        }
                    } else if (whereContractsResult == WhereContractsResult::Failed) {
                        // If the new decl didn't fail the contracts or the `foundTemplateDecl` is still null then we
                        // set the decl. We set it here just to be used in error messages if it did fail
                        if (declWhereContractsResult != WhereContractsResult::Failed || foundTemplateDecl == nullptr) {
                            isAmbiguous = false;
                            foundTemplateDecl = checkDecl;
                            isExact = declIsExact;
                            whereContractsResult = declWhereContractsResult;
                        }
                    }
                }
            }
        }

        if (foundTemplateDecl == nullptr) {
            printError("template type `" + templatedType->toString() + "` was not found for the provided arguments!",
                       templatedType->startPosition(), templatedType->endPosition());
        }

        if (whereContractsResult == WhereContractsResult::Failed) {
            // TODO: We need to improve this error message to say what contract failed...
            printError("template arguments provided for type `" + templatedType->toString() + "` do not match required contracts!",
                       templatedType->startPosition(), templatedType->endPosition());
        }

        if (isAmbiguous) {
            printError("template type `" + templatedType->toString() + "` is ambiguous in the current context!",
                       templatedType->startPosition(), templatedType->endPosition());
        }

        if (!isExact) {
            // TODO: Once default arguments are supported on template types we will have to support grabbing the
            //       template type data and putting it into our arguments list...
            printError("[INTERNAL] non-exact match found for template type, this is not yet handled!",
                       templatedType->startPosition(), templatedType->endPosition());
        }

        // NOTE: I'm not sure if there is a better way to do this
        //       I'm just re-detecting what the Decl is for the template. It could be better to
        //       Make a generate "TemplateDecl" or something that will handle holding everything for
        //       "TemplateStructDecl", "TemplateTraitDecl", etc.
        switch (foundTemplateDecl->getDeclKind()) {
            case Decl::Kind::TemplateStruct: {
                auto templateStructDecl = llvm::dyn_cast<TemplateStructDecl>(foundTemplateDecl);

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
                    // The template arguments weren't solved, we will return a `TemplateStructType` instead...
                    Type* newType = new TemplateStructType(templatedType->qualifier(),
                                                           templatedType->templateArguments(), templateStructDecl,
                                                           templatedType->startPosition(),
                                                           templatedType->endPosition());

                    if (templateStructDecl->containedInTemplate && containDependents) {
                        auto containerTemplateType = templateStructDecl->containerTemplateType->deepCopy();
                        newType = new DependentType(templatedType->qualifier(), containerTemplateType, newType);
                    }

                    // We steal the template arguments
                    templatedType->templateArguments().clear();
                    delete type;
                    type = newType;
                }

                return true;
            }
            case Decl::Kind::TemplateTrait: {
                auto templateTraitDecl = llvm::dyn_cast<TemplateTraitDecl>(foundTemplateDecl);

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
                    // The template arguments weren't solved, we will return a `TemplateTraitType` instead...
                    Type* newType = new TemplateTraitType(templatedType->qualifier(),
                                                          templatedType->templateArguments(), templateTraitDecl,
                                                          templatedType->startPosition(),
                                                          templatedType->endPosition());

                    if (templateTraitDecl->containedInTemplate && containDependents) {
                        auto containerTemplateType = templateTraitDecl->containerTemplateType->deepCopy();
                        newType = new DependentType(templatedType->qualifier(), containerTemplateType, newType);
                    }

                    // We steal the template arguments
                    templatedType->templateArguments().clear();
                    delete type;
                    type = newType;
                }

                return true;
            }
            case Decl::Kind::TypeAlias: {
                auto typeAlias = llvm::dyn_cast<TypeAliasDecl>(foundTemplateDecl);

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

void gulc::DeclInstantiator::compareDeclTemplateArgsToParams(const std::vector<Cont*>& contracts,
                                                             const std::vector<Expr*>& args,
                                                             const std::vector<TemplateParameterDecl*>& params,
                                                             bool* outIsMatch, bool* outIsExact,
                                                             WhereContractsResult* outPassedWhereContracts) const {
    if (params.size() < args.size()) {
        // If there are more template arguments than parameters then we skip this Decl...
        *outIsMatch = false;
        *outIsExact = false;
        *outPassedWhereContracts = WhereContractsResult::Failed;
        return;
    }

    *outIsMatch = true;
    *outPassedWhereContracts = WhereContractsResult::Failed;
    // TODO: Once we support implicit casts and default template parameter values we need to set this to false
    //       when we implicit cast or use a default template parameter.
    *outIsExact = true;

    for (int i = 0; i < params.size(); ++i) {
        if (i >= args.size()) {
            // TODO: Once we support default values for template types we need to account for them here.
            *outIsMatch = false;
            *outIsExact = false;
            break;
        } else {
            if (params[i]->templateParameterKind() == TemplateParameterDecl::TemplateParameterKind::Typename) {
                if (!llvm::isa<TypeExpr>(args[i])) {
                    // If the parameter is a `typename` then the argument MUST be a resolved type
                    *outIsMatch = false;
                    *outIsExact = false;
                    break;
                }
            } else {
                if (llvm::isa<TypeExpr>(args[i])) {
                    // If the parameter is a `const` then it MUST NOT be a type...
                    *outIsMatch = false;
                    *outIsExact = false;
                    break;
                } else if (llvm::isa<ValueLiteralExpr>(args[i])) {
                    auto valueLiteral = llvm::dyn_cast<ValueLiteralExpr>(args[i]);

                    TypeCompareUtil typeCompareUtil;

                    if (!typeCompareUtil.compareAreSame(params[i]->constType, valueLiteral->valueType)) {
                        // TODO: Support checking if an implicit cast is possible...
                        *outIsMatch = false;
                        *outIsExact = false;
                        break;
                    }
                } else {
                    printError("unsupported expression in template arguments list!",
                               args[i]->startPosition(), args[i]->endPosition());
                }
            }
        }
    }

    if (*outIsMatch) {
        // If it was a match we now will check the contracts
        // For this we default to `NoContracts` until we find a where contract
        *outPassedWhereContracts = WhereContractsResult::NoContracts;

        for (Cont* contract : contracts) {
            if (llvm::isa<WhereCont>(contract)) {
                if (*outPassedWhereContracts == WhereContractsResult::NoContracts) {
                    *outPassedWhereContracts = WhereContractsResult::Passed;
                }

                auto checkWhere = llvm::dyn_cast<WhereCont>(contract);

                ContractUtil contractUtil(_filePaths[_currentFile->sourceFileID], &params, &args);

                if (!contractUtil.checkWhereCont(checkWhere)) {
                    *outPassedWhereContracts = WhereContractsResult::Failed;
                    break;
                }
            }
        }
    }

    // NOTE: Once we've reached this point the decl has been completely evaluated...
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
            printError("INTERNAL ERROR - unhandled Decl type found in `DeclInstantiator`!",
                       decl->startPosition(), decl->endPosition());
            // If we don't know the declaration we just skip it, we don't care in this pass
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

    for (EnumConstDecl* enumConst : enumDecl->enumConsts()) {
        processConstExpr(enumConst->constValue);
    }
}

void gulc::DeclInstantiator::processExtensionDecl(gulc::ExtensionDecl* extensionDecl) {
    for (TemplateParameterDecl* templateParameter : extensionDecl->templateParameters()) {
        processTemplateParameterDecl(templateParameter);
    }

    if (!extensionDecl->hasTemplateParameters()) {
        if (!resolveType(extensionDecl->typeToExtend)) {
            printError("extension type `" + extensionDecl->typeToExtend->toString() + "` was not found!",
                       extensionDecl->typeToExtend->startPosition(), extensionDecl->typeToExtend->endPosition());
        }

        for (Type*& inheritedType : extensionDecl->inheritedTypes()) {
            if (!resolveType(inheritedType)) {
                printError("extension inherited type `" + inheritedType->toString() + "` was not found!",
                           inheritedType->startPosition(), inheritedType->endPosition());
            }

            if (!llvm::isa<TraitType>(inheritedType)) {
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
}

void gulc::DeclInstantiator::processNamespaceDecl(gulc::NamespaceDecl* namespaceDecl) {
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
        processDecl(getter);
    }

    if (subscriptOperatorDecl->hasSetter()) {
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

void gulc::DeclInstantiator::processTemplateParameterDecl(gulc::TemplateParameterDecl* templateParameterDecl) {
    // If the template parameter is a const then we have to process its underlying type
    if (templateParameterDecl->templateParameterKind() == TemplateParameterDecl::TemplateParameterKind::Const) {
        if (!resolveType(templateParameterDecl->constType)) {
            printError("const template parameter type `" + templateParameterDecl->constType->toString() + "` was not found!",
                       templateParameterDecl->startPosition(), templateParameterDecl->endPosition());
        }
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

    processStructDecl(templateStructDecl, false);
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
