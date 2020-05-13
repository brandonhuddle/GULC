#include <llvm/Support/Casting.h>
#include <ast/types/DimensionType.hpp>
#include <ast/types/PointerType.hpp>
#include <ast/types/ReferenceType.hpp>
#include <ast/types/UnresolvedType.hpp>
#include <ast/types/BuiltInType.hpp>
#include <make_reverse_iterator.hpp>
#include <ast/types/TemplateTypenameRefType.hpp>
#include <ast/decls/StructDecl.hpp>
#include <ast/types/StructType.hpp>
#include <ast/decls/TemplateStructDecl.hpp>
#include <ast/types/TemplatedType.hpp>
#include <ast/decls/TraitDecl.hpp>
#include <ast/decls/TemplateTraitDecl.hpp>
#include <ast/types/TraitType.hpp>
#include <ast/decls/TypeAliasDecl.hpp>
#include <ast/types/AliasType.hpp>
#include <ast/decls/NamespaceDecl.hpp>
#include <ast/decls/EnumDecl.hpp>
#include <ast/types/EnumType.hpp>
#include <ast/types/NestedType.hpp>
#include <ast/types/FlatArrayType.hpp>
#include <ast/types/TemplateStructType.hpp>
#include "TypeHelper.hpp"

bool gulc::TypeHelper::resolveType(gulc::Type*& type, ASTFile const* currentFile,
                                   std::vector<NamespaceDecl*>& namespacePrototypes,
                                   std::vector<std::vector<TemplateParameterDecl*>*> const& templateParameters,
                                   std::vector<gulc::Decl*> const& containingDecls, bool* outIsAmbiguous) {
    *outIsAmbiguous = false;

    switch (type->getTypeKind()) {
        case Type::Kind::Alias:
            return true;
        case Type::Kind::BuiltIn:
            return true;
        case Type::Kind::Dimension: {
            auto dimensionType = llvm::dyn_cast<DimensionType>(type);
            return resolveType(dimensionType->nestedType, currentFile, namespacePrototypes,
                               templateParameters, containingDecls, outIsAmbiguous);
        }
        case Type::Kind::Enum:
            return true;
        case Type::Kind::FlatArray: {
            auto flatArrayType = llvm::dyn_cast<FlatArrayType>(type);
            return resolveType(flatArrayType->indexType, currentFile, namespacePrototypes,
                               templateParameters, containingDecls, outIsAmbiguous);
        }
        case Type::Kind::Nested: {
            auto nestedType = llvm::dyn_cast<NestedType>(type);

            if (resolveType(nestedType->container, currentFile, namespacePrototypes, templateParameters,
                            containingDecls, outIsAmbiguous)) {
                std::vector<Decl*> ignore;

                // If we've resolved the inner type then we've been resolved. No matter what. Further resolution past
                // what we can do here MUST happen outside of this function.
                switch (nestedType->container->getTypeKind()) {
                    case Type::Kind::Struct: {
                        auto structType = llvm::dyn_cast<StructType>(nestedType->container);

                        Type* fakeUnresolvedType = new UnresolvedType(nestedType->qualifier(), {},
                                                                      nestedType->identifier(),
                                                                      nestedType->templateArguments());

                        if (resolveTypeToDecl(fakeUnresolvedType, structType->decl(), nestedType->identifier().name(),
                                              !nestedType->templateArguments().empty(), ignore,
                                              true, false)) {
                            // If the type was resolved we delete the old type, set it to our fake unresolved type,
                            // and immediately return true to prevent anyone from accessing `type` in the wrong way.
                            delete type;
                            type = fakeUnresolvedType;
                            return true;
                        } else {
                            // Clear the template arguments list (since it is owned by `nestedType`)
                            llvm::dyn_cast<UnresolvedType>(fakeUnresolvedType)->templateArguments.clear();
                            delete fakeUnresolvedType;
                        }

                        break;
                    }
                    case Type::Kind::Trait: {
                        auto traitType = llvm::dyn_cast<TraitType>(nestedType->container);

                        Type* fakeUnresolvedType = new UnresolvedType(nestedType->qualifier(), {},
                                                                      nestedType->identifier(),
                                                                      nestedType->templateArguments());

                        if (resolveTypeToDecl(fakeUnresolvedType, traitType->decl(), nestedType->identifier().name(),
                                              !nestedType->templateArguments().empty(), ignore,
                                              true, false)) {
                            // If the type was resolved we delete the old type, set it to our fake unresolved type,
                            // and immediately return true to prevent anyone from accessing `type` in the wrong way.
                            delete type;
                            type = fakeUnresolvedType;
                            return true;
                        } else {
                            // Clear the template arguments list (since it is owned by `nestedType`)
                            llvm::dyn_cast<UnresolvedType>(fakeUnresolvedType)->templateArguments.clear();
                            delete fakeUnresolvedType;
                        }

                        break;
                    }
                    default:
                        break;
                }

                return true;
            } else {
                return false;
            }
        }
        case Type::Kind::Pointer: {
            auto pointerType = llvm::dyn_cast<PointerType>(type);
            return resolveType(pointerType->nestedType, currentFile, namespacePrototypes,
                               templateParameters, containingDecls, outIsAmbiguous);
        }
        case Type::Kind::Reference: {
            auto referenceType = llvm::dyn_cast<ReferenceType>(type);
            return resolveType(referenceType->nestedType, currentFile, namespacePrototypes,
                               templateParameters, containingDecls, outIsAmbiguous);
        }
        case Type::Kind::Unresolved: {
            auto unresolvedType = llvm::dyn_cast<UnresolvedType>(type);

            if (!unresolvedType->namespacePath().empty()) {
                std::string const& firstPathName = unresolvedType->namespacePath()[0].name();
                // This is the `Decl` the namespacePath points to (if found)
                Decl* foundContainer = nullptr;

                // First search the `currentFile->declarations` for anything EXCEPT `NamespaceDecl`
                // if we don't find the first identifier there then search `namespacePrototypes`
                // (`namespacePrototypes` will contain the `NamespaceDecl`s that we skipped but will contain
                //  the missing `Decl`s that are found in other files)
                // TODO: How will we support templates within the namespace path? We allow nested structs so
                //       `Temp<12>.InnerTemp<i32>.NestedTemp<i32, 12 + 4>` is a legal (but nasty to look at) type.
                //       I'm thinking maybe a `NestedTemplatedType` that will take a template type as a `container`
                //       and then a normal type for the nested type:
                //       ```
                //           struct NestedTemplatedType : Type {
                //               var container: *TemplatedType;
                //               var nestedType: *Type;
                //           }
                //       ```
                //       Then when `TemplatedType` can be resolved we can resolve `NestedTemplatedType` as well.
                for (Decl* checkDecl : currentFile->declarations) {
                    switch (checkDecl->getDeclKind()) {
                        case Decl::Kind::Struct: {
                            if (firstPathName != checkDecl->identifier().name()) continue;

                            auto checkStruct = llvm::dyn_cast<StructDecl>(checkDecl);

                            if (unresolvedType->namespacePath().size() > 1) {
                                if (resolveNamespacePathToDecl(unresolvedType->namespacePath(), 1,
                                                               checkStruct->ownedMembers(), &foundContainer)) {
                                    goto exitCurrentFileDeclarationsLoop;
                                }
                            } else {
                                foundContainer = checkDecl;
                                goto exitCurrentFileDeclarationsLoop;
                            }

                            break;
                        }
                        case Decl::Kind::Trait: {
                            if (firstPathName != checkDecl->identifier().name()) continue;

                            auto checkTrait = llvm::dyn_cast<TraitDecl>(checkDecl);

                            if (unresolvedType->namespacePath().size() > 1) {
                                if (resolveNamespacePathToDecl(unresolvedType->namespacePath(), 1,
                                                               checkTrait->ownedMembers(), &foundContainer)) {
                                    goto exitCurrentFileDeclarationsLoop;
                                }
                            } else {
                                foundContainer = checkDecl;
                                goto exitCurrentFileDeclarationsLoop;
                            }

                            break;
                        }
                        default:
                            break;
                    }
                }

                // If we haven't found the container we check the imports with aliases
            exitCurrentFileDeclarationsLoop:
                if (foundContainer == nullptr) {
                    for (ImportDecl* checkImport : currentFile->imports) {
                        if (checkImport->hasAlias()) {
                            if (checkImport->importAlias().name() == firstPathName) {
                                if (unresolvedType->namespacePath().size() > 1) {
                                    if (resolveNamespacePathToDecl(unresolvedType->namespacePath(), 1,
                                                                   checkImport->pointToNamespace->nestedDecls(),
                                                                   &foundContainer)) {
                                        goto exitImportAliasesLoop;
                                    }
                                } else {
                                    foundContainer = checkImport->pointToNamespace;
                                    goto exitImportAliasesLoop;
                                }
                            }
                        }
                    }
                }

                // If we still haven't found the container we check all namespace prototypes
            exitImportAliasesLoop:
                if (foundContainer == nullptr) {
                    for (NamespaceDecl* checkNamespace : namespacePrototypes) {
                        if (checkNamespace->identifier().name() == firstPathName) {
                            if (unresolvedType->namespacePath().size() > 1) {
                                if (resolveNamespacePathToDecl(unresolvedType->namespacePath(), 1,
                                                               checkNamespace->nestedDecls(), &foundContainer)) {
                                    goto exitNamespacePrototypesLoop;
                                }
                            } else {
                                foundContainer = checkNamespace;
                                goto exitNamespacePrototypesLoop;
                            }
                        }
                    }
                }

                // If the container is still null then we return false, we didn't find the type.
            exitNamespacePrototypesLoop:
                if (foundContainer == nullptr) {
                    return false;
                }

                std::string const& checkName = unresolvedType->identifier().name();
                bool templated = unresolvedType->hasTemplateArguments();
                std::vector<Decl*> potentialTemplates;

                // Now that we have the container we need to search the container for the actual type...
                // Since we're checking containers we allow searching members
                if (resolveTypeToDecl(type, foundContainer, checkName, templated, potentialTemplates,
                                      true, false)) {
                    return true;
                }

                if (templated && !potentialTemplates.empty()) {
                    auto result = new TemplatedType(unresolvedType->qualifier(), potentialTemplates,
                                                    std::move(unresolvedType->templateArguments),
                                                    unresolvedType->startPosition(), unresolvedType->endPosition());
                    result->setIsLValue(unresolvedType->isLValue());

                    delete unresolvedType;

                    type = result;

                    return true;
                } else {
                    return false;
                }
            } else {
                std::string const& checkName = unresolvedType->identifier().name();
                bool templated = unresolvedType->hasTemplateArguments();
                std::vector<Decl*> potentialTemplates;

                if (!templated) {
                    // First check if it is a built in type
                    if (BuiltInType::isBuiltInType(checkName)) {
                        auto result = BuiltInType::get(unresolvedType->qualifier(), checkName,
                                                       unresolvedType->startPosition(), unresolvedType->endPosition());
                        result->setIsLValue(unresolvedType->isLValue());

                        delete unresolvedType;

                        type = result;

                        return true;
                    }
                }

                // Then if there are template parameters we'll check them
                // NOTE: Template parameter types cannot be templated themselves
                if (!templated && !templateParameters.empty()) {
                    for (std::vector<TemplateParameterDecl*>* checkTemplateParameters :
                            gulc::reverse(templateParameters)) {
                        for (TemplateParameterDecl* templateParameter : *checkTemplateParameters) {
                            if (templateParameter->templateParameterKind() ==
                                TemplateParameterDecl::TemplateParameterKind::Typename) {
                                if (checkName == templateParameter->identifier().name()) {
                                    auto result = new TemplateTypenameRefType(unresolvedType->qualifier(),
                                                                              templateParameter,
                                                                              unresolvedType->startPosition(),
                                                                              unresolvedType->endPosition());
                                    result->setIsLValue(unresolvedType->isLValue());

                                    delete unresolvedType;

                                    type = result;

                                    return true;
                                }
                            }
                        }
                    }
                }

                // Then check our containing decls
                if (!containingDecls.empty()) {
                    for (Decl* checkContainer : gulc::reverse(containingDecls)) {
                        // Since we're checking containers we allow searching members
                        if (resolveTypeToDecl(type, checkContainer, checkName, templated, potentialTemplates,
                                              true, false)) {
                            return true;
                        }
                    }
                }

                // Then check our file
                for (Decl* checkDecl : currentFile->declarations) {
                    // We set `searchMembers` to false because the declarations in the current file may not actually be
                    // our container
                    if (resolveTypeToDecl(type, checkDecl, checkName, templated, potentialTemplates,
                                          false, true)) {
                        return true;
                    }
                }

                // Then check our imports
                {
                    // For import we have to do an ambiguity check. If this isn't false when we find a Decl then that
                    // means the type reference is ambiguous and needs to be manually disambiguated.
                    bool typeWasResolved = false;
                    Decl* foundDecl = nullptr;

                    for (ImportDecl* checkImport : currentFile->imports) {
                        // We ignore any imports with aliases (we can only implicitly search the non-aliased)
                        if (!checkImport->hasAlias()) {
                            if (!typeWasResolved) {
                                if (resolveTypeToDecl(type, checkImport->pointToNamespace, checkName, templated,
                                                      potentialTemplates, false, true,
                                                      &foundDecl)) {
                                    typeWasResolved = true;
                                }
                            } else {
                                if (checkImportForAmbiguity(checkImport, checkName, foundDecl)) {
                                    // Notify we resolved the type while also notifying that the type is ambiguous
                                    *outIsAmbiguous = true;
                                    return true;
                                }
                            }
                        }
                    }

                    if (typeWasResolved) {
                        // If the type was resolved and wasn't ambiguous we return true here.
                        return true;
                    }
                }

                if (templated && !potentialTemplates.empty()) {
                    auto result = new TemplatedType(unresolvedType->qualifier(), potentialTemplates,
                                                    std::move(unresolvedType->templateArguments),
                                                    unresolvedType->startPosition(), unresolvedType->endPosition());
                    result->setIsLValue(unresolvedType->isLValue());

                    delete unresolvedType;

                    type = result;

                    return true;
                } else {
                    return false;
                }
            }
        }
        default:
            // If we don't know what the type is we can't resolve it
            return false;
    }
}

bool gulc::TypeHelper::typeIsConstExpr(gulc::Type* resolvedType) {
    switch (resolvedType->getTypeKind()) {
        case Type::Kind::Alias: {
            auto aliasType = llvm::dyn_cast<AliasType>(resolvedType);
            return typeIsConstExpr(aliasType->decl()->typeValue);
        }
        case Type::Kind::BuiltIn:
            return true;
        case Type::Kind::Dimension:
            // TODO: We cannot know if `[]` is const or not...
            return false;
        case Type::Kind::Enum:
            // All enums are `const`
            return true;
        case Type::Kind::Pointer: {
            auto pointerType = llvm::dyn_cast<PointerType>(resolvedType);
            return typeIsConstExpr(pointerType->nestedType);
        }
        case Type::Kind::Reference: {
            auto referenceType = llvm::dyn_cast<ReferenceType>(resolvedType);
            return typeIsConstExpr(referenceType->nestedType);
        }
        case Type::Kind::Struct: {
            auto structType = llvm::dyn_cast<StructType>(resolvedType);
            return structType->decl()->isConstExpr();
        }
        case Type::Kind::Templated: {
            // NOTE: A templated type means it is a template that isn't 100% resolved. We can't know if it is `const`
            //       or not so we lie and say it is. If it turns out it isn't that must be handled in a pass
            return true;
        }
        case Type::Kind::Trait: {
            auto traitType = llvm::dyn_cast<TraitType>(resolvedType);
            return traitType->decl()->isConstExpr();
        }
        case Type::Kind::TemplateTypenameRef: {
            // NOTE: A template typename is considered const-qualified in this pass.
            //       If this is being used in a situation where `const` is required then the argument will be checked
            //       to be a valid `const` instead.
            return true;
        }
        default:
            return false;
    }
}

bool gulc::TypeHelper::resolveTypeWithinDecl(gulc::Type*& type, gulc::Decl* container) {
    auto unresolvedType = llvm::dyn_cast<UnresolvedType>(type);
    std::string const& checkName = unresolvedType->identifier().name();
    bool templated = unresolvedType->hasTemplateArguments();
    std::vector<Decl*> potentialTemplates;

    if (resolveTypeToDecl(type, container, checkName, templated, potentialTemplates,
                          true, false, nullptr)) {
        return true;
    }

    if (!potentialTemplates.empty()) {
        auto newType = new TemplatedType(type->qualifier(), potentialTemplates, unresolvedType->templateArguments,
                                         type->startPosition(), type->endPosition());
        // Delete the old type without deleting the template arguments (since we've given them to the `TemplatedType`)
        unresolvedType->templateArguments.clear();
        delete type;
        type = newType;

        return true;
    }

    return false;
}

bool gulc::TypeHelper::resolveTypeToDecl(Type*& type, gulc::Decl* checkDecl,
                                         std::string const& checkName, bool templated,
                                         std::vector<Decl*>& potentialTemplates,
                                         bool searchMembers, bool resolveToCheckDecl, Decl** outFoundDecl) {
    auto unresolvedType = llvm::dyn_cast<UnresolvedType>(type);

    // TODO: I think we might have a bug where `outFoundDecl` is nested within another `Decl` it will be overwritten
    //       with the container rather than the actual found declaration. This needs double checked and fixed.
    switch (checkDecl->getDeclKind()) {
        case Decl::Kind::Namespace: {
            if (!searchMembers) return false;

            auto namespaceDecl = llvm::dyn_cast<NamespaceDecl>(checkDecl);

            for (Decl* checkNestedDecl : namespaceDecl->nestedDecls()) {
                // Nested Decl will always have `searchMembers` set to false to avoid following branches.
                // We only want to check the owned members, not the members of the namespace members.
                if (resolveTypeToDecl(type, checkNestedDecl, checkName, templated, potentialTemplates,
                                      false, true, outFoundDecl)) {
                    if (outFoundDecl != nullptr) {
                        *outFoundDecl = checkNestedDecl;
                    }

                    // We only return true if the Decl was resolved
                    return true;
                }
            }

            break;
        }
        case Decl::Kind::TemplateStructInst: {
            if (!searchMembers) return false;

            auto templateStructInstDecl = llvm::dyn_cast<TemplateStructInstDecl>(checkDecl);

            for (Decl* checkNestedDecl : templateStructInstDecl->ownedMembers()) {
                // Nested Decl will always have `searchMembers` set to false to avoid following branches.
                // We only want to check the owned members, not the members of the namespace members.
                if (resolveTypeToDecl(type, checkNestedDecl, checkName, templated, potentialTemplates,
                                      false, true, outFoundDecl)) {
                    if (outFoundDecl != nullptr) {
                        *outFoundDecl = checkNestedDecl;
                    }

                    // We only return true if the Decl was resolved
                    return true;
                }
            }

            break;
        }
        case Decl::Kind::TemplateTraitInst: {
            if (!searchMembers) return false;

            auto templateStructInstDecl = llvm::dyn_cast<TemplateStructInstDecl>(checkDecl);

            for (Decl* checkNestedDecl : templateStructInstDecl->ownedMembers()) {
                // Nested Decl will always have `searchMembers` set to false to avoid following branches.
                // We only want to check the owned members, not the members of the namespace members.
                if (resolveTypeToDecl(type, checkNestedDecl, checkName, templated, potentialTemplates,
                                      false, true, outFoundDecl)) {
                    if (outFoundDecl != nullptr) {
                        *outFoundDecl = checkNestedDecl;
                    }

                    // We only return true if the Decl was resolved
                    return true;
                }
            }

            break;
        }
        case Decl::Kind::Enum: {
            if (templated || !resolveToCheckDecl) return false;

            auto checkEnum = llvm::dyn_cast<EnumDecl>(checkDecl);

            if (checkEnum->identifier().name() == checkName) {
                if (checkEnum->containedInTemplate) {
                    auto containerTemplate = checkEnum->containerTemplateType->deepCopy();
                    auto result = new NestedType(unresolvedType->qualifier(), containerTemplate,
                                                 unresolvedType->identifier(), {},
                                                 unresolvedType->startPosition(), unresolvedType->endPosition());
                    result->setIsLValue(unresolvedType->isLValue());

                    delete type;

                    type = result;

                    if (outFoundDecl != nullptr) {
                        *outFoundDecl = checkDecl;
                    }

                    return true;
                } else {
                    auto result = new EnumType(unresolvedType->qualifier(), checkEnum,
                                               unresolvedType->startPosition(),
                                               unresolvedType->endPosition());
                    result->setIsLValue(unresolvedType->isLValue());

                    delete type;

                    type = result;

                    if (outFoundDecl != nullptr) {
                        *outFoundDecl = checkDecl;
                    }

                    return true;
                }
            }

            break;
        }
        case Decl::Kind::Struct: {
            auto checkStruct = llvm::dyn_cast<StructDecl>(checkDecl);

            if (resolveToCheckDecl && !templated && checkStruct->identifier().name() == checkName) {
                if (checkStruct->containedInTemplate) {
                    auto containerTemplate = checkStruct->containerTemplateType->deepCopy();
                    auto result = new NestedType(unresolvedType->qualifier(), containerTemplate,
                                                 unresolvedType->identifier(), {},
                                                 unresolvedType->startPosition(), unresolvedType->endPosition());
                    result->setIsLValue(unresolvedType->isLValue());

                    delete type;

                    type = result;

                    if (outFoundDecl != nullptr) {
                        *outFoundDecl = checkDecl;
                    }

                    return true;
                } else {
                    auto result = new StructType(unresolvedType->qualifier(), checkStruct,
                                                 unresolvedType->startPosition(),
                                                 unresolvedType->endPosition());
                    result->setIsLValue(unresolvedType->isLValue());

                    delete type;

                    type = result;

                    if (outFoundDecl != nullptr) {
                        *outFoundDecl = checkDecl;
                    }

                    return true;
                }
            } else if (searchMembers) {
                for (Decl* checkNestedDecl : checkStruct->ownedMembers()) {
                    // Nested Decl will always have `searchMembers` set to false to avoid following branches.
                    // We only want to check the owned members, not the members of the namespace members.
                    if (resolveTypeToDecl(type, checkNestedDecl, checkName, templated, potentialTemplates,
                                          false, true, outFoundDecl)) {
                        if (outFoundDecl != nullptr) {
                            *outFoundDecl = checkNestedDecl;
                        }

                        // We only return true if the Decl was resolved
                        return true;
                    }
                }
            }

            break;
        }
        case Decl::Kind::Trait: {
            auto checkTrait = llvm::dyn_cast<TraitDecl>(checkDecl);

            if (resolveToCheckDecl && !templated && checkTrait->identifier().name() == checkName) {
                if (checkTrait->containedInTemplate) {
                    auto containerTemplate = checkTrait->containerTemplateType->deepCopy();
                    auto result = new NestedType(unresolvedType->qualifier(), containerTemplate,
                                                 unresolvedType->identifier(), {},
                                                 unresolvedType->startPosition(), unresolvedType->endPosition());
                    result->setIsLValue(unresolvedType->isLValue());

                    delete type;

                    type = result;

                    if (outFoundDecl != nullptr) {
                        *outFoundDecl = checkDecl;
                    }

                    return true;
                } else {
                    auto result = new TraitType(unresolvedType->qualifier(), checkTrait,
                                                unresolvedType->startPosition(),
                                                unresolvedType->endPosition());
                    result->setIsLValue(unresolvedType->isLValue());

                    delete type;

                    type = result;

                    if (outFoundDecl != nullptr) {
                        *outFoundDecl = checkDecl;
                    }

                    return true;
                }
            } else if (searchMembers) {
                for (Decl* checkNestedDecl : checkTrait->ownedMembers()) {
                    // Nested Decl will always have `searchMembers` set to false to avoid following branches.
                    // We only want to check the owned members, not the members of the namespace members.
                    if (resolveTypeToDecl(type, checkNestedDecl, checkName, templated, potentialTemplates,
                                          false, true, outFoundDecl)) {
                        if (outFoundDecl != nullptr) {
                            *outFoundDecl = checkNestedDecl;
                        }

                        // We only return true if the Decl was resolved
                        return true;
                    }
                }
            }

            break;
        }
        case Decl::Kind::TemplateStruct: {
            auto checkTemplateStruct = llvm::dyn_cast<TemplateStructDecl>(checkDecl);

            if (resolveToCheckDecl && templated && checkTemplateStruct->identifier().name() == checkName) {
                // TODO: Support optional template parameters
                if (checkTemplateStruct->templateParameters().size() == unresolvedType->templateArguments.size()) {
                    potentialTemplates.push_back(checkTemplateStruct);
                    // We keep searching as this might be the wrong type...
                }
            } else if (searchMembers) {
                for (Decl* checkNestedDecl : checkTemplateStruct->ownedMembers()) {
                    // Nested Decl will always have `searchMembers` set to false to avoid following branches.
                    // We only want to check the owned members, not the members of the namespace members.
                    if (resolveTypeToDecl(type, checkNestedDecl, checkName, templated, potentialTemplates,
                                          false, true, outFoundDecl)) {
                        if (outFoundDecl != nullptr) {
                            *outFoundDecl = checkNestedDecl;
                        }

                        // We only return true if the Decl was resolved
                        return true;
                    }
                }
            }

            break;
        }
        case Decl::Kind::TemplateTrait: {
            auto checkTemplateTrait = llvm::dyn_cast<TemplateTraitDecl>(checkDecl);

            if (resolveToCheckDecl && templated && checkTemplateTrait->identifier().name() == checkName) {
                // TODO: Support optional template parameters
                if (checkTemplateTrait->templateParameters().size() == unresolvedType->templateArguments.size()) {
                    potentialTemplates.push_back(checkTemplateTrait);
                    // We keep searching as this might be the wrong type...
                }
            } else if (searchMembers) {
                for (Decl* checkNestedDecl : checkTemplateTrait->ownedMembers()) {
                    // Nested Decl will always have `searchMembers` set to false to avoid following branches.
                    // We only want to check the owned members, not the members of the namespace members.
                    if (resolveTypeToDecl(type, checkNestedDecl, checkName, templated, potentialTemplates,
                                          false, true, outFoundDecl)) {
                        if (outFoundDecl != nullptr) {
                            *outFoundDecl = checkNestedDecl;
                        }

                        // We only return true if the Decl was resolved
                        return true;
                    }
                }
            }

            break;
        }
        case Decl::Kind::TypeAlias: {
            auto checkAlias = llvm::dyn_cast<TypeAliasDecl>(checkDecl);

            if (resolveToCheckDecl && checkAlias->identifier().name() == checkName) {
                if (templated) {
                    // We skip any aliases that don't have template parameters (since we have arguments)
                    if (!checkAlias->hasTemplateParameters()) {
                        return false;
                    }

                    // TODO: Support optional template parameters
                    if (checkAlias->templateParameters().size() == unresolvedType->templateArguments.size()) {
                        potentialTemplates.push_back(checkAlias);
                        // We keep searching as this might be the wrong type...
                    }
                } else {
                    // We skip any aliases that have template parameters (since we have no arguments)
                    if (checkAlias->hasTemplateParameters()) {
                        return false;
                    }

                    if (checkAlias->containedInTemplate) {
                        auto containerTemplate = checkAlias->containerTemplateType->deepCopy();
                        auto result = new NestedType(unresolvedType->qualifier(), containerTemplate,
                                                     unresolvedType->identifier(), {},
                                                     unresolvedType->startPosition(), unresolvedType->endPosition());
                        result->setIsLValue(unresolvedType->isLValue());

                        delete type;

                        type = result;

                        if (outFoundDecl != nullptr) {
                            *outFoundDecl = checkDecl;
                        }

                        return true;
                    } else {
                        // Rather than access `checkAlias->typeValue` we return an `AliasType`
                        // this is due to the fact that if `checkAlias->typeValue` isn't resolve yet we will
                        // give an error in the wrong spot OR potentially resolve it to the wrong area.
                        auto result = new AliasType(unresolvedType->qualifier(), checkAlias,
                                                    unresolvedType->startPosition(),
                                                    unresolvedType->endPosition());
                        result->setIsLValue(unresolvedType->isLValue());

                        delete unresolvedType;

                        type = result;

                        if (outFoundDecl != nullptr) {
                            *outFoundDecl = checkDecl;
                        }

                        return true;
                    }
                }
            }

            break;
        }
        default:
            break;
    }

    return false;
}

bool gulc::TypeHelper::resolveNamespacePathToDecl(const std::vector<Identifier>& namespacePath, std::size_t pathIndex,
                                                  const std::vector<Decl*>& declList, gulc::Decl** resultDecl) {
    for (Decl* checkDecl : declList) {
        // NOTE: We currently don't support templated types.
        //       (`Example<i32>::InnerType` and `Example<i8>::InnerType` are different types)
        switch (checkDecl->getDeclKind()) {
            case Decl::Kind::Namespace: {
                if (checkDecl->identifier().name() != namespacePath[pathIndex].name()) continue;

                if (pathIndex == namespacePath.size() - 1) {
                    // We've reached the end of the namespace path, this is the correct decl
                    *resultDecl = checkDecl;
                    return true;
                } else {
                    // We need to search the namespace for a matching declaration
                    auto checkNamespace = llvm::dyn_cast<NamespaceDecl>(checkDecl);

                    if (resolveNamespacePathToDecl(namespacePath, pathIndex + 1,
                                                   checkNamespace->nestedDecls(), resultDecl)) {
                        // If it returns true then we return true, effectively exiting `resolveNamespacePathToDecl`
                        // entirely
                        return true;
                    }
                }

                break;
            }
            case Decl::Kind::Struct: {
                if (checkDecl->identifier().name() != namespacePath[pathIndex].name()) continue;

                if (pathIndex == namespacePath.size() - 1) {
                    // We've reached the end of the namespace path, this is the correct decl
                    *resultDecl = checkDecl;
                    return true;
                } else {
                    // We need to search the struct for a matching declaration
                    auto checkStruct = llvm::dyn_cast<StructDecl>(checkDecl);

                    if (resolveNamespacePathToDecl(namespacePath, pathIndex + 1,
                                                   checkStruct->ownedMembers(), resultDecl)) {
                        // If it returns true then we return true, effectively exiting `resolveNamespacePathToDecl`
                        // entirely
                        return true;
                    }
                }

                break;
            }
            case Decl::Kind::Trait: {
                if (checkDecl->identifier().name() != namespacePath[pathIndex].name()) continue;

                if (pathIndex == namespacePath.size() - 1) {
                    // We've reached the end of the namespace path, this is the correct decl
                    *resultDecl = checkDecl;
                    return true;
                } else {
                    // We need to search the trait for a matching declaration
                    auto checkTrait = llvm::dyn_cast<TraitDecl>(checkDecl);

                    if (resolveNamespacePathToDecl(namespacePath, pathIndex + 1,
                                                   checkTrait->ownedMembers(), resultDecl)) {
                        // If it returns true then we return true, effectively exiting `resolveNamespacePathToDecl`
                        // entirely
                        return true;
                    }
                }

                break;
            }
            default:
                continue;
        }
    }
    return false;
}

bool gulc::TypeHelper::checkImportForAmbiguity(gulc::ImportDecl* importDecl, const std::string& checkName,
                                               gulc::Decl* skipDecl) {
    for (Decl* checkDecl : importDecl->pointToNamespace->nestedDecls()) {
        if (checkDecl == skipDecl) continue;

        switch (checkDecl->getDeclKind()) {
            case Decl::Kind::Enum: {
                auto enumDecl = llvm::dyn_cast<EnumDecl>(checkDecl);

                if (enumDecl->identifier().name() == checkName) {
                    // If the identifier is the same then we found an ambiguity.
                    return true;
                }

                break;
            }
            case Decl::Kind::Struct: {
                auto structDecl = llvm::dyn_cast<StructDecl>(checkDecl);

                if (structDecl->identifier().name() == checkName) {
                    // If the identifier is the same then we found an ambiguity.
                    return true;
                }

                break;
            }
            case Decl::Kind::Trait: {
                auto traitDecl = llvm::dyn_cast<TraitDecl>(checkDecl);

                if (traitDecl->identifier().name() == checkName) {
                    // If the identifier is the same then we found an ambiguity.
                    return true;
                }

                break;
            }
            case Decl::Kind::TypeAlias: {
                auto typeAlias = llvm::dyn_cast<TypeAliasDecl>(checkDecl);

                // We skip the templates, we can't resolve to them
                if (typeAlias->hasTemplateParameters()) continue;

                if (typeAlias->identifier().name() == checkName) {
                    // If the identifier is the same then we found an ambiguity.
                    return true;
                }

                break;
            }
            default:
                // NOTE: We don't have to handle any templates
                continue;
        }
    }

    return false;
}
