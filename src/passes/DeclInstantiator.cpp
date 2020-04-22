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
    if (llvm::isa<DimensionType>(type)) {
        auto dimensionType = llvm::dyn_cast<DimensionType>(type);

        return resolveType(dimensionType->nestedType);
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
                case Decl::Kind::TemplateStruct: {
                    auto templateStructDecl = llvm::dyn_cast<TemplateStructDecl>(checkDecl);

                    if (templateStructDecl->templateParameters().size() <
                        templatedType->templateArguments().size()) {
                        // If there are more template arguments than parameters then we skip this Decl...
                        break;
                    }

                    for (int i = 0; i < templateStructDecl->templateParameters().size(); ++i) {
                        if (i >= templatedType->templateArguments().size()) {
                            // TODO: Once we support default values for template types we need to account for them here.
                            declIsMatch = false;
                            declIsExact = false;
                            break;
                        } else {
                            if (templateStructDecl->templateParameters()[i]->templateParameterKind() ==
                                TemplateParameterDecl::TemplateParameterKind::Typename) {
                                if (!llvm::isa<TypeExpr>(templatedType->templateArguments()[i])) {
                                    // If the parameter is a `typename` then the argument MUST be a resolved type
                                    declIsMatch = false;
                                    declIsExact = false;
                                    break;
                                }
                            } else {
                                if (llvm::isa<TypeExpr>(templatedType->templateArguments()[i])) {
                                    // If the parameter is a `const` then it MUST NOT be a type...
                                    declIsMatch = false;
                                    declIsExact = false;
                                    break;
                                } else if (llvm::isa<ValueLiteralExpr>(templatedType->templateArguments()[i])) {
                                    auto valueLiteral = llvm::dyn_cast<ValueLiteralExpr>(templatedType->templateArguments()[i]);

                                    if (!TypeHelper::compareAreSame(templateStructDecl->templateParameters()[i]->constType, valueLiteral->valueType)) {
                                        // TODO: Support checking if an implicit cast is possible...
                                        declIsMatch = false;
                                        declIsExact = false;
                                        break;
                                    }
                                } else {
                                    printError("unsupported expression in template arguments list!",
                                               templatedType->templateArguments()[i]->startPosition(),
                                               templatedType->templateArguments()[i]->endPosition());
                                }
                            }
                        }
                    }

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
                if (!templateStructInstDecl->isInstantiated) {
                    processDependantDecl(templateStructInstDecl);
                }

                Type* newType = new StructType(templatedType->qualifier(), templateStructInstDecl,
                                               templatedType->startPosition(), templatedType->endPosition());
                delete type;
                type = newType;

                return true;
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

void gulc::DeclInstantiator::processDecl(gulc::Decl* decl, bool isGlobal) {
    switch (decl->getDeclKind()) {
        case Decl::Kind::Import:
            // We skip imports, they're no longer useful here...
            break;

        case Decl::Kind::Function:
            processFunctionDecl(llvm::dyn_cast<FunctionDecl>(decl));
            break;
        case Decl::Kind::Namespace:
            processNamespaceDecl(llvm::dyn_cast<NamespaceDecl>(decl));
            break;
        case Decl::Kind::Struct:
            processStructDecl(llvm::dyn_cast<StructDecl>(decl));
            break;
        case Decl::Kind::TemplateFunction:
            processTemplateFunctionDecl(llvm::dyn_cast<TemplateFunctionDecl>(decl));
            break;
        case Decl::Kind::TemplateStruct:
            processTemplateStructDecl(llvm::dyn_cast<TemplateStructDecl>(decl));
            break;
        case Decl::Kind::Variable:
            processVariableDecl(llvm::dyn_cast<VariableDecl>(decl), isGlobal);
            break;

        default:
//            printError("unknown declaration found!",
//                       decl->startPosition(), decl->endPosition());
            // If we don't know the declaration we just skip it, we don't care in this pass
            break;
    }
}

void gulc::DeclInstantiator::processFunctionDecl(gulc::FunctionDecl* functionDecl) {
    for (ParameterDecl* parameter : functionDecl->parameters()) {
        processParameterDecl(parameter);
    }
}

void gulc::DeclInstantiator::processNamespaceDecl(gulc::NamespaceDecl* namespaceDecl) {
    for (Decl* nestedDecl : namespaceDecl->nestedDecls()) {
        processDecl(nestedDecl);
    }
}

void gulc::DeclInstantiator::processParameterDecl(gulc::ParameterDecl* parameterDecl) {
    // TODO: Should we also handle the default value here? It might be useful to `const` expressions...
    if (!resolveType(parameterDecl->type)) {
        printError("function parameter type `" + parameterDecl->type->toString() + "` was not found!",
                   parameterDecl->startPosition(), parameterDecl->endPosition());
    }
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
                                                 Identifier({}, {}, "_"), new VTableType(), nullptr, {}, {},
                                                 DeclModifiers::None);
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
                                                        paddingType, nullptr, {}, {},
                                                        DeclModifiers::None);

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

void gulc::DeclInstantiator::processVariableDecl(gulc::VariableDecl* variableDecl, bool isGlobal) {
    if (isGlobal) {
        if (!variableDecl->isConstExpr() && !variableDecl->isStatic()) {
            printError("global variables must be marked `const` or `static`!",
                       variableDecl->startPosition(), variableDecl->endPosition());
        }
    }

    // TODO: Should we handle the variable's initial value? I think we should for `const` expressions
    if (variableDecl->type == nullptr) {
        printError("variables outside of function bodies and similar MUST have a type specified!",
                   variableDecl->startPosition(), variableDecl->endPosition());
    } else {
        if (!resolveType(variableDecl->type)) {
            printError("variable type `" + variableDecl->type->toString() + "` was not found!",
                       variableDecl->type->startPosition(), variableDecl->type->endPosition());
        }
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
