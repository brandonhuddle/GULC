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
#include <ast/types/NestedType.hpp>

void gulc::DeclInstantiator::processFiles(std::vector<ASTFile>& files) {
    for (ASTFile& file : files) {
        _currentFile = &file;

        for (Decl* decl : file.declarations) {
            processDecl(decl);
        }
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

bool gulc::DeclInstantiator::resolveType(gulc::Type*& type) {
    if (llvm::isa<AliasType>(type)) {
        auto aliasType = llvm::dyn_cast<AliasType>(type);
        Type* resultType = aliasType->decl()->getInstantiation({});

        // We have to process the alias type value again...
        bool resultBool = resolveType(resultType);

        if (resultBool) {
            delete type;
            type = resultType;
            return true;
        } else {
            delete resultType;
            return false;
        }
    } else if (llvm::isa<DimensionType>(type)) {
        auto dimensionType = llvm::dyn_cast<DimensionType>(type);

        return resolveType(dimensionType->nestedType);
    } else if (llvm::dyn_cast<NestedType>(type)) {
        auto nestedType = llvm::dyn_cast<NestedType>(type);

        // Process the template arguments and try to resolve any potential types in the list...
        for (Expr*& templateArgument : nestedType->templateArguments()) {
            processConstExpr(templateArgument);
        }

        if (resolveType(nestedType->container)) {
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
                        return resolveType(type);
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
                        return resolveType(type);
                    } else {
                        // Clear the template arguments from `fakeUnresolvedType` and delete it, it wasn't resolved
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
    } else if (llvm::isa<PointerType>(type)) {
        auto pointerType = llvm::dyn_cast<PointerType>(type);

        return resolveType(pointerType->nestedType);
    } else if (llvm::isa<ReferenceType>(type)) {
        auto referenceType = llvm::dyn_cast<ReferenceType>(type);

        return resolveType(referenceType->nestedType);
    } else if (llvm::isa<TemplatedType>(type)) {
        auto templatedType = llvm::dyn_cast<TemplatedType>(type);

        // Process the template arguments and try to resolve any potential types in the list...
        for (Expr*& templateArgument : templatedType->templateArguments()) {
            processConstExpr(templateArgument);
        }

        Decl* foundTemplateDecl = nullptr;
        bool isExact = false;
        bool isAmbiguous = false;

        // TODO: Process contracts
        for (Decl* checkDecl : templatedType->matchingTemplateDecls()) {
            bool declIsMatch = true;
            bool declIsExact = true;

            switch (checkDecl->getDeclKind()) {
                // TODO:
                case Decl::Kind::TemplateStruct: {
                    auto templateStructDecl = llvm::dyn_cast<TemplateStructDecl>(checkDecl);

                    compareDeclTemplateArgsToParams(templatedType->templateArguments(),
                                                    templateStructDecl->templateParameters(),
                                                    &declIsMatch, &declIsExact);

                    // NOTE: Once we've reached this point the decl has been completely evaluated...

                    break;
                }
                case Decl::Kind::TemplateTrait: {
                    auto templateTraitDecl = llvm::dyn_cast<TemplateTraitDecl>(checkDecl);

                    compareDeclTemplateArgsToParams(templatedType->templateArguments(),
                                                    templateTraitDecl->templateParameters(),
                                                    &declIsMatch, &declIsExact);

                    // NOTE: Once we've reached this point the decl has been completely evaluated...

                    break;
                }
                case Decl::Kind::TypeAlias: {
                    auto typeAliasDecl = llvm::dyn_cast<TypeAliasDecl>(checkDecl);

                    compareDeclTemplateArgsToParams(templatedType->templateArguments(),
                                                    typeAliasDecl->templateParameters(),
                                                    &declIsMatch, &declIsExact);

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
                if (declIsExact) {
                    if (isExact) {
                        // If the already found declaration is an exact match and this new decl is an exact match
                        // then there is an issue with ambiguity
                        isAmbiguous = true;
                    }

                    isExact = true;
                }

                foundTemplateDecl = checkDecl;
            }
        }

        if (foundTemplateDecl == nullptr) {
            printError("template type `" + templatedType->toString() + "` was not found for the provided arguments!",
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

                TemplateStructInstDecl* templateStructInstDecl = nullptr;
                templateStructDecl->getInstantiation(templatedType->templateArguments(), &templateStructInstDecl);

                // If the template struct instantiation isn't instantiated we have to call the `processDependantDecl`
                // We call this instead of `processTemplateStructInstDecl` so that we properly check for circular
                // references
                // TODO: Should we always do this? This could cause an issue in some niche scenarios when compiling
                //       a pointer to a tempalte struct. One potential issue is below:
                //       ```
                //       struct Example {
                //           var member: *TemplateExample<i32>;
                //       }
                //       struct TemplateExample<T> : Example {
                //           // This template will be processed AFTER `Example` and COULD cause a bug where
                //           // instantiating `TemplateExample<i32>` causes a false error. We don't need
                //           // `TemplateExample<i32>` to be processed for `Example` to be processed.
                //       }
                //       ```
                if (!templateStructInstDecl->isInstantiated) {
                    processDependantDecl(templateStructInstDecl);
                }

                Type* newType = new StructType(templatedType->qualifier(), templateStructInstDecl,
                                               templatedType->startPosition(), templatedType->endPosition());
                delete type;
                type = newType;

                return true;
            }
            case Decl::Kind::TemplateTrait: {
                auto templateTraitDecl = llvm::dyn_cast<TemplateTraitDecl>(foundTemplateDecl);

                TemplateTraitInstDecl* templateTraitInstDecl = nullptr;
                templateTraitDecl->getInstantiation(templatedType->templateArguments(), &templateTraitInstDecl);

                // TODO: Ditto to above.
                if (!templateTraitInstDecl->isInstantiated) {
                    processDependantDecl(templateTraitInstDecl);
                }

                Type* newType = new TraitType(templatedType->qualifier(), templateTraitInstDecl,
                                              templatedType->startPosition(), templatedType->endPosition());
                delete type;
                type = newType;

                return true;
            }
            case Decl::Kind::TypeAlias: {
                auto typeAlias = llvm::dyn_cast<TypeAliasDecl>(foundTemplateDecl);

                // TODO: Should we apply the `qualifier`?
                Type* newType = typeAlias->getInstantiation(templatedType->templateArguments());
                delete type;
                type = newType;

                // We pass the `newType` back into `resolveType` since it might be a `TemplatedType`
                return resolveType(newType);
            }
            default:
                printWarning("[INTERNAL] unknown template declaration found in `BaseResolver::resolveType`!",
                             foundTemplateDecl->startPosition(), foundTemplateDecl->endPosition());
                break;
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

void gulc::DeclInstantiator::compareDeclTemplateArgsToParams(const std::vector<Expr*>& args,
                                                             const std::vector<TemplateParameterDecl*>& params,
                                                             bool* outIsMatch, bool* outIsExact) const {
    if (params.size() < args.size()) {
        // If there are more template arguments than parameters then we skip this Decl...
        *outIsMatch = false;
        *outIsExact = false;
        return;
    }

    *outIsMatch = true;
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

                    if (!TypeHelper::compareAreSame(params[i]->constType, valueLiteral->valueType)) {
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

    // NOTE: Once we've reached this point the decl has been completely evaluated...
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

    // TODO: We need to improve our support for templates. Currently we can't properly finish instantiating any
    //       templates due to their need to keep their `TemplatedType` and not being able to properly account for
    //       non-template types nested within a template type.
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

void gulc::DeclInstantiator::processStructDecl(gulc::StructDecl* structDecl, bool calculateSizeAndVTable) {
    // The struct could already have been instantiated. We skip any that have already been processed...
    if (structDecl->isInstantiated) {
        return;
    }

    _workingDecls.push_back(structDecl);

    for (Type*& inheritedType : structDecl->inheritedTypes()) {
        if (!resolveType(inheritedType)) {
            printError("struct inherited type `" + inheritedType->toString() + "` was not found!",
                       structDecl->startPosition(), structDecl->endPosition());
        }

        if (llvm::isa<StructType>(inheritedType)) {
            auto structType = llvm::dyn_cast<StructType>(inheritedType);

            if (structDecl->baseStruct != nullptr) {
                printError("struct '" + structDecl->identifier().name() + "' cannot extend both '" +
                           structDecl->baseStruct->identifier().name() + "' and '" +
                           structType->decl()->identifier().name() +
                           "' at the same time! (both types are structs)",
                           structDecl->startPosition(), structDecl->endPosition());
            } else {
                structDecl->baseStruct = structType->decl();
            }
        }
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
                                                    destructorModifiers, {}, {}, {}, {});
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
    if (calculateSizeAndVTable) {
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

    _workingDecls.pop_back();

    // TODO: I've disabled processing the `TemplateStructDecl` as a normal struct. I don't think we should be doing
    //       that. We should still validate the contracts work properly and disallow the template from doing anything
    //       that isn't within their `where` clause BUT processing it like a normal struct will resolve
    //       `TemplatedType`s to `Template*InstDecl`s which we do NOT want. This creates issues with potentially
    //       overwriting/changing things we don't mean to change when processing `Template*InstDecl`s alone
    // TODO: I think a potential solution to this would be to resolve any `TemplatedType`s found within a
    //       `TemplateStructDecl` to a new `TemplatedType` containing ONLY the template `Decl` we've resolved to?
    //       This would have to apply even in scenarios of `struct ExOuter<G> { struct ExInner { ... } }`
    //       The types within `ExInner` MUST resolve back to `TemplatedType` because
    //       `ExOuter<i32>::ExInner` != `ExOuter<i8>::ExInner`
    // TODO: Building off above we could also do `PartialType` which would replace resolving back to `TemplatedType`?
    //       This gives us the extra benefit of knowing a `PartialType` has had it's contracts checked already and
    //       allow us to validate the rest of the code based off said contracts?
//    processStructDecl(templateStructDecl, false);
    templateStructDecl->isInstantiated = true;

    for (TemplateStructInstDecl* templateStructInstDecl : templateStructDecl->templateInstantiations()) {
        processTemplateStructInstDecl(templateStructInstDecl);
    }
}

void gulc::DeclInstantiator::processTemplateStructInstDecl(gulc::TemplateStructInstDecl* templateStructInstDecl) {
    // The struct could already have been instantiated. We skip any that have already been processed...
    if (templateStructInstDecl->isInstantiated) {
        return;
    }

    TemplateInstHelper templateInstHelper;
    templateInstHelper.instantiateTemplateStructInstDecl(templateStructInstDecl->parentTemplateStruct(),
                                                         templateStructInstDecl, false);

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

    _workingDecls.pop_back();

    // TODO: Apply the same solution as `TemplateStructDecl` here...
//    processTraitDecl(templateTraitDecl);
    templateTraitDecl->isInstantiated = true;

    for (TemplateTraitInstDecl* templateTraitInstDecl : templateTraitDecl->templateInstantiations()) {
        processTemplateTraitInstDecl(templateTraitInstDecl);
    }
}

void gulc::DeclInstantiator::processTemplateTraitInstDecl(gulc::TemplateTraitInstDecl* templateTraitInstDecl) {
    // The trait could already have been instantiated. We skip any that have already been processed...
    if (templateTraitInstDecl->isInstantiated) {
        return;
    }

    TemplateInstHelper templateInstHelper;
    templateInstHelper.instantiateTemplateTraitInstDecl(templateTraitInstDecl->parentTemplateTrait(),
                                                         templateTraitInstDecl, false);

    processTraitDecl(templateTraitInstDecl);
}

void gulc::DeclInstantiator::processTraitDecl(gulc::TraitDecl* traitDecl) {
    // The trait could already have been instantiated. We skip any that have already been processed...
    if (traitDecl->isInstantiated) {
        return;
    }

    _workingDecls.push_back(traitDecl);

    for (Type*& inheritedType : traitDecl->inheritedTypes()) {
        if (!resolveType(inheritedType)) {
            printError("trait inherited type `" + inheritedType->toString() + "` was not found!",
                       traitDecl->startPosition(), traitDecl->endPosition());
        }

        if (!llvm::isa<TraitType>(inheritedType)) {
            printError("traits can only extend other traits! (`" + inheritedType->toString() + "` is not a trait!)",
                       inheritedType->startPosition(), inheritedType->endPosition());
        }
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

    // NOTE: We don't process `typeAliasDecl->typeValue` again here. We want to keep the `TemplatedType` if it has one
    // TODO: Once we add a `PartialType` for handling partially resolved templates we need to process
    //       `typeAliasDecl->typeValue` here again.
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
