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
#include "CodeGen.hpp"

#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Utils.h>
#include <ast/types/BuiltInType.hpp>
#include <ast/types/EnumType.hpp>
#include <ast/types/FlatArrayType.hpp>
#include <ast/types/PointerType.hpp>
#include <ast/types/ReferenceType.hpp>
#include <ast/types/StructType.hpp>
#include <make_reverse_iterator.hpp>
#include <ast/exprs/FunctionReferenceExpr.hpp>
#include <ast/types/FunctionPointerType.hpp>
#include <utilities/TypeCompareUtil.hpp>

gulc::Module gulc::CodeGen::generate(gulc::ASTFile* file) {
    auto llvmContext = new llvm::LLVMContext();
    auto irBuilder = llvm::IRBuilder<>(*llvmContext);
    // TODO: Should we remove the ending file extension?
    auto genModule = new llvm::Module(_filePaths[file->sourceFileID], *llvmContext);
    //llvm::PassManager<llvm::Function>* funcPassManager = new llvm::PassManager<llvm::Function>();
    auto funcPassManager = new llvm::legacy::FunctionPassManager(genModule);

    // Promote allocas to registers.
    funcPassManager->add(llvm::createPromoteMemoryToRegisterPass());
    // Do simple "peephole" optimizations and bit-twiddling optzns.
    funcPassManager->add(llvm::createInstructionCombiningPass());
    // Reassociate expressions.
    funcPassManager->add(llvm::createReassociatePass());
    // Eliminate Common SubExpressions.
    funcPassManager->add(llvm::createGVNPass());
    // Simplify the control flow graph (deleting unreachable blocks, etc).
    funcPassManager->add(llvm::createCFGSimplificationPass());

    funcPassManager->doInitialization();

    _currentFile = file;
    this->_llvmContext = llvmContext;
    this->_irBuilder = &irBuilder;
    this->_llvmModule = genModule;
    this->_funcPassManager = funcPassManager;

    // TODO: We will need to account for the imported `extern` decls. These are implicitly `extern`ed declarations that
    //       come from the `import` statements. Much easier and quicker than needing to `extern` every declaration from
    //       every single import.

    for (gulc::Decl const* decl : file->declarations) {
        generateDecl(decl);

        //globalObject.print()
    }

    genModule->print(llvm::errs(), nullptr);

    funcPassManager->doFinalization();
    delete funcPassManager;

    return Module(_filePaths[_currentFile->sourceFileID], llvmContext, genModule);
}

void gulc::CodeGen::printError(std::string const& message, gulc::TextPosition startPosition,
                               gulc::TextPosition endPosition) {
    std::cout << "gulc codegen error[" << _filePaths[_currentFile->sourceFileID] << ", "
                                   "{" << startPosition.line << ", " << startPosition.column << "} "
                                   "to {" << endPosition.line << ", " << endPosition.column << "}]: "
              << message
              << std::endl;
    std::exit(1);
}

llvm::Type* gulc::CodeGen::generateLlvmType(gulc::Type const* type) {
    switch (type->getTypeKind()) {
        case Type::Kind::Bool: {
            return llvm::Type::getInt8Ty(*_llvmContext);
        }
        case Type::Kind::BuiltIn: {
            auto builtInType = llvm::dyn_cast<BuiltInType>(type);

            // Void is special...
            if (builtInType->sizeInBytes() == 0) {
                return llvm::Type::getVoidTy(*_llvmContext);
            }

            // TODO: Bool is special...
//            if (builtInType->isBool()) {
//                // TODO: Should we support making booleans int1?
////                return llvm::Type::getInt1Ty(*llvmContext);
//                return llvm::Type::getInt8Ty(*_llvmContext);
//            }

            // Floats...
            if (builtInType->isFloating()) {
                switch (builtInType->sizeInBytes()) {
                    case 2:
                        return llvm::Type::getHalfTy(*_llvmContext);
                    case 4:
                        return llvm::Type::getFloatTy(*_llvmContext);
                    case 8:
                        return llvm::Type::getDoubleTy(*_llvmContext);
                        //case 16: 128 bit is support on some platforms, we should support it...
                    default:
                        printError("unsupported floating point size for type '" + type->toString() + "'!",
                                   type->startPosition(), type->endPosition());
                        return nullptr;
                }
            }

            // Signed/unsigned ints...
            // Thing to note here is that LLVM handles the signedness of a variable by the operation you used,
            // similar to assembly. That is why we don't check if it is signed here.
            switch (builtInType->sizeInBytes()) {
                case 1:
                    return llvm::Type::getInt8Ty(*_llvmContext);
                case 2:
                    return llvm::Type::getInt16Ty(*_llvmContext);
                case 4:
                    return llvm::Type::getInt32Ty(*_llvmContext);
                case 8:
                    return llvm::Type::getInt64Ty(*_llvmContext);
                    //case 16: 128 bit is supported on some platforms, we should support it...
                default:
                    printError("unsupported floating point size for type '" + type->toString() + "'!",
                               type->startPosition(), type->endPosition());
                    return nullptr;
            }
        }
        case Type::Kind::Enum: {
            auto enumType = llvm::dyn_cast<EnumType>(type);
            printError("enum types not yet supported!", enumType->startPosition(), enumType->endPosition());
            return nullptr;
        }
        case Type::Kind::FlatArray: {
            auto flatArrayType = llvm::dyn_cast<FlatArrayType>(type);
            auto indexType = generateLlvmType(flatArrayType->indexType);
            std::uint64_t length = generateConstSize(flatArrayType->length);

            return llvm::ArrayType::get(indexType, length);
        }
        case Type::Kind::FunctionPointer: {
            printError("[INTERNAL] function pointer type not yet supported!",
                       type->startPosition(), type->endPosition());
            return nullptr;
        }
        case Type::Kind::Pointer: {
            auto pointerType = llvm::dyn_cast<gulc::PointerType>(type);
            return llvm::PointerType::getUnqual(generateLlvmType(pointerType->nestedType));
        }
        case Type::Kind::Reference: {
            auto referenceType = llvm::dyn_cast<gulc::ReferenceType>(type);
            return llvm::PointerType::getUnqual(generateLlvmType(referenceType->nestedType));
        }
        case Type::Kind::Struct: {
            auto structType = llvm::dyn_cast<StructType>(type);
            return generateLlvmStructType(structType->decl());
        }
        case Type::Kind::Trait: {
            printError("[INTERNAL] trait types not yet supported!",
                       type->startPosition(), type->endPosition());
            return nullptr;
        }
        case Type::Kind::VTable: {
            // We just make the vtable a `void**` and will bitcast later to what it needs to be later.
            llvm::Type* varArgFuncType = llvm::FunctionType::get(llvm::Type::getVoidTy(*_llvmContext), true);
            return llvm::PointerType::get(llvm::PointerType::get(varArgFuncType, 0), 0);
        }
        default:
            printError("[INTERNAL] unsupported type `" + type->toString() + "` found in `CodeGen::generateLlvmType`!",
                       type->startPosition(), type->endPosition());
            return nullptr;
    }
}

std::vector<llvm::Type*> gulc::CodeGen::generateLlvmParamTypes(std::vector<ParameterDecl*> const& parameters,
                                                               gulc::StructDecl const* parentStruct) {
    std::vector<llvm::Type*> paramTypes{};
    paramTypes.reserve(parameters.size());

    if (parentStruct) {
        paramTypes.push_back(llvm::PointerType::getUnqual(generateLlvmStructType(parentStruct)));
    }

    for (ParameterDecl const* parameterDecl : parameters) {
        paramTypes.push_back(generateLlvmType(parameterDecl->type));
    }

    return paramTypes;
}

llvm::StructType* gulc::CodeGen::generateLlvmStructType(gulc::StructDecl const* structDecl, bool unpadded) {
    if (unpadded && _cachedLlvmStructTypes.find(structDecl->mangledName() + ".unpadded") !=
            _cachedLlvmStructTypes.end()) {
        return _cachedLlvmStructTypes[structDecl->mangledName() + ".unpadded"];
    } else if (!unpadded && _cachedLlvmStructTypes.find(structDecl->mangledName()) != _cachedLlvmStructTypes.end()) {
        return _cachedLlvmStructTypes[structDecl->mangledName()];
    } else {
        // TODO: Support `@packed` attribute...
        std::vector<llvm::Type*> elements;
        std::vector<llvm::Type*> elementsPadded;

        if (structDecl->baseStruct != nullptr) {
            llvm::Type* baseType = generateLlvmStructType(structDecl->baseStruct, true);

            elements.push_back(baseType);
            elementsPadded.push_back(baseType);
        }

        for (VariableDecl* dataMember : structDecl->memoryLayout) {
            llvm::Type* memberType = generateLlvmType(dataMember->type);

            elements.push_back(memberType);
            elementsPadded.push_back(memberType);
        }

        std::size_t structAlign = _target.alignofStruct();
        std::size_t alignPadding = 0;

        // `align` can't be zero, `n % 0` is illegal since `n / 0` is illegal
        if (structAlign != 0) {
            alignPadding = structAlign - (structDecl->dataSizeWithoutPadding % structAlign);

            // Rather than deal with casting to a signed type and rearrange the above algorithm to prevent
            // this from happening, we just check if the `alignPadding` is equal to the `align` and set
            // `alignPadding` to zero if it happens
            if (alignPadding == structAlign) {
                alignPadding = 0;
            }
        }

        // If the `alignPadding` isn't zero then we pad the end of the struct
        if (alignPadding > 0) {
            elementsPadded.push_back(llvm::ArrayType::get(llvm::Type::getInt8Ty(*_llvmContext),
                                                          alignPadding));
        }

        auto result = llvm::StructType::create(*_llvmContext, elements,
                                               structDecl->identifier().name() + ".unpadded", true);
        auto resultPadded = llvm::StructType::create(*_llvmContext, elementsPadded, structDecl->identifier().name(),
                                                     true);
        _cachedLlvmStructTypes.insert({structDecl->mangledName() + ".unpadded", result});
        _cachedLlvmStructTypes.insert({structDecl->mangledName(), resultPadded});

        if (unpadded) {
            return result;
        } else {
            return resultPadded;
        }
    }

    return nullptr;
}

std::uint64_t gulc::CodeGen::generateConstSize(gulc::Expr* constSize) {
    if (!llvm::isa<ValueLiteralExpr>(constSize)) {
        printError("[INTERNAL] `CodeGen::generateConstSize` received non-value literal!",
                   constSize->startPosition(), constSize->endPosition());
    }

    auto valueLiteralExpr = llvm::dyn_cast<ValueLiteralExpr>(constSize);

    // NOTE: If the literal has a suffix still by this point it means that the type wasn't converted to a proper size
    //       integer
    if (valueLiteralExpr->literalType() != ValueLiteralExpr::LiteralType::Integer || valueLiteralExpr->hasSuffix()) {
        printError("[INTERNAL] `CodeGen::generateConstSize` received non-integer literal!",
                   constSize->startPosition(), constSize->endPosition());
    }

    return std::stoull(valueLiteralExpr->value());
}

void gulc::CodeGen::generateDecl(gulc::Decl const* decl, bool isInternal) {
    switch (decl->getDeclKind()) {
        case Decl::Kind::Constructor:
            printError("[INTERNAL] constructor found outside of valid container declaration!",
                       decl->startPosition(), decl->endPosition());
            break;
        case Decl::Kind::Destructor:
            printError("[INTERNAL] destructor found outside of valid container declaration!",
                       decl->startPosition(), decl->endPosition());
            break;
        case Decl::Kind::Enum:
            generateEnumDecl(llvm::dyn_cast<EnumDecl>(decl), isInternal);
            break;
        case Decl::Kind::TemplateFunctionInst:
        case Decl::Kind::Function:
            generateFunctionDecl(llvm::dyn_cast<FunctionDecl>(decl), isInternal);
            break;
        case Decl::Kind::Namespace:
            generateNamespaceDecl(llvm::dyn_cast<NamespaceDecl>(decl));
            break;
        case Decl::Kind::Property:
            generatePropertyDecl(llvm::dyn_cast<PropertyDecl>(decl), isInternal);
            break;
        case Decl::Kind::TemplateStructInst:
        case Decl::Kind::Struct:
            generateStructDecl(llvm::dyn_cast<StructDecl>(decl), isInternal);
            break;
        case Decl::Kind::SubscriptOperator:
            printError("[INTERNAL] subscript operator found outside of valid container declaration!",
                       decl->startPosition(), decl->endPosition());
            break;
        case Decl::Kind::TemplateFunction:
            generateTemplateFunctionDecl(llvm::dyn_cast<TemplateFunctionDecl>(decl), isInternal);
            break;
        case Decl::Kind::TemplateStruct:
            generateTemplateStructDecl(llvm::dyn_cast<TemplateStructDecl>(decl), isInternal);
            break;
        case Decl::Kind::TemplateTrait:
            generateTemplateTraitDecl(llvm::dyn_cast<TemplateTraitDecl>(decl), isInternal);
            break;
        case Decl::Kind::TemplateTraitInst:
        case Decl::Kind::Trait:
            generateTraitDecl(llvm::dyn_cast<TraitDecl>(decl), isInternal);
            break;
        case Decl::Kind::Variable:
            generateVariableDecl(llvm::dyn_cast<VariableDecl>(decl), isInternal);
            break;
        default:
            printError("[INTERNAL] unsupported declaration found in `CodeGen::generateDecl`!",
                       decl->startPosition(), decl->endPosition());
            break;
    }
}

void gulc::CodeGen::generateConstructorDecl(gulc::ConstructorDecl const* constructorDecl, bool isInternal) {
    auto parentStruct = llvm::dyn_cast<StructDecl>(constructorDecl->container);

    std::vector<llvm::Type*> paramTypes = generateLlvmParamTypes(constructorDecl->parameters(), parentStruct);
    // All constructors return void. We construct the `this` parameter. Memory allocation for the struct is
    // NOT handled by the constructor
    llvm::Type* returnType = llvm::Type::getVoidTy(*_llvmContext);
    llvm::FunctionType* functionType = llvm::FunctionType::get(returnType, paramTypes, false);
    llvm::Function* function = _llvmModule->getFunction(constructorDecl->mangledName());

    if (!function) {
        auto linkageType = llvm::Function::LinkageTypes::ExternalLinkage;

        if (isInternal) {
            linkageType = llvm::Function::LinkageTypes::InternalLinkage;
        }

        function = llvm::Function::Create(functionType, linkageType, constructorDecl->mangledName(), _llvmModule);
    }

    // Generate the constructor that DOESN'T assign the vtable
    {
        llvm::BasicBlock *funcBody = llvm::BasicBlock::Create(*_llvmContext, "entry", function);
        _irBuilder->SetInsertPoint(funcBody);

        setCurrentFunction(function, constructorDecl);

        // If there is a base constructor we HAVE to call it as the first line of the constructor
        // TODO: Support the base constructors
//        if (constructorDecl->baseConstructor != nullptr) {
//            generateBaseConstructorCallExpr(constructorDecl->baseConstructorCall);
//        }

        // Generate the function body
        generateStmt(constructorDecl->body());

        verifyFunction(*function);
        _funcPassManager->run(*function);

        // Reset the insertion point (this probably isn't needed but oh well)
        _irBuilder->ClearInsertionPoint();
        _currentGhoulFunction = nullptr;
        _currentLlvmFunction = nullptr;
    }

    // If there is a vtable we generate a vtable constructor
    if (!parentStruct->vtable.empty()) {
        llvm::Function *functionVTable = _llvmModule->getFunction(constructorDecl->mangledNameVTable());

        if (!functionVTable) {
            auto linkageType = llvm::Function::LinkageTypes::ExternalLinkage;

            if (isInternal) {
                linkageType = llvm::Function::LinkageTypes::InternalLinkage;
            }

            functionVTable = llvm::Function::Create(functionType, linkageType, constructorDecl->mangledNameVTable(),
                                                    _llvmModule);
        }

        // Generate the constructor that DOES assign the vtable
        {
            gulc::StructType gulcVTableOwnerType(Type::Qualifier::Mut, parentStruct->vtableOwner, {}, {});
            llvm::Type *vtableOwnerType = generateLlvmType(&gulcVTableOwnerType);

            llvm::BasicBlock *funcBody = llvm::BasicBlock::Create(*_llvmContext, "entry", functionVTable);
            _irBuilder->SetInsertPoint(funcBody);

            setCurrentFunction(functionVTable, constructorDecl);

            // TODO: Assign vtable here
            {
                llvm::Value *refThis = _currentLlvmFunctionParameters[0];
                llvm::Value *derefThis = _irBuilder->CreateLoad(refThis);
                llvm::Value *vtableOwner = derefThis;

                // Cast to the vtable owner if we have to
                if (parentStruct != parentStruct->vtableOwner) {
                    vtableOwner = _irBuilder->CreateBitCast(vtableOwner, llvm::PointerType::getUnqual(vtableOwnerType));
                }

                // Get a reference to the vtable
                llvm::Value *vtableRef = _llvmModule->getGlobalVariable(parentStruct->vtableName, true);

                // Get a pointer to array
                llvm::Value *index0 = llvm::ConstantInt::get(*_llvmContext, llvm::APInt(32, 0, false));
                vtableRef = _irBuilder->CreateGEP(vtableRef, index0);

                // Cast the pointer to the correct type (void (...)**)
                llvm::Type *elementType = llvm::FunctionType::get(llvm::Type::getVoidTy(*_llvmContext), true);
                elementType = llvm::PointerType::get(elementType, 0);
                elementType = llvm::PointerType::get(elementType, 0);

                vtableRef = _irBuilder->CreateBitCast(vtableRef, elementType);

                // Grab the vtable member and set it
                llvm::Value *vtableMemberRef = _irBuilder->CreateStructGEP(vtableOwner, 0);
                _irBuilder->CreateStore(vtableRef, vtableMemberRef, false);
            }

            // If there is a base constructor we HAVE to call it as the first line of the constructor
            // TODO: Support base constructor call
//            if (constructorDecl->baseConstructor != nullptr) {
//                generateBaseConstructorCallExpr(constructorDecl->baseConstructorCall);
//            }

            // Generate the function body
            generateStmt(constructorDecl->body());

            verifyFunction(*functionVTable);
            _funcPassManager->run(*functionVTable);

            // Reset the insertion point (this probably isn't needed but oh well)
            _irBuilder->ClearInsertionPoint();
            _currentGhoulFunction = nullptr;
            _currentLlvmFunction = nullptr;
        }
    }
}

void gulc::CodeGen::generateDestructorDecl(gulc::DestructorDecl const* destructorDecl, bool isInternal) {
    auto parentStruct = llvm::dyn_cast<StructDecl>(destructorDecl->container);

    // Destructors DO NOT support parameters except for the single `this` parameter
    std::vector<llvm::Type*> paramTypes = generateLlvmParamTypes({}, parentStruct);
    // All constructors return void. We construct the `this` parameter. Memory allocation for the struct is
    // NOT handled by the constructor
    llvm::Type* returnType = llvm::Type::getVoidTy(*_llvmContext);
    llvm::FunctionType* functionType = llvm::FunctionType::get(returnType, paramTypes, false);
    llvm::Function* function = _llvmModule->getFunction(destructorDecl->mangledName());

    if (!function) {
        auto linkageType = llvm::Function::LinkageTypes::ExternalLinkage;

        if (isInternal) {
            linkageType = llvm::Function::LinkageTypes::InternalLinkage;
        }

        function = llvm::Function::Create(functionType, linkageType, destructorDecl->mangledName(), _llvmModule);
    }

    llvm::BasicBlock* funcBody = llvm::BasicBlock::Create(*_llvmContext, "entry", function);
    _irBuilder->SetInsertPoint(funcBody);

    setCurrentFunction(function, destructorDecl);

    // Generate the function body
    generateStmt(destructorDecl->body());

    verifyFunction(*function);
    _funcPassManager->run(*function);

    // Reset the insertion point (this probably isn't needed but oh well)
    _irBuilder->ClearInsertionPoint();
    _currentGhoulFunction = nullptr;
    _currentLlvmFunction = nullptr;
}

void gulc::CodeGen::generateEnumDecl(gulc::EnumDecl const* enumDecl, bool isInternal) {
    // TODO: Generate the functions same as you would in structs
}

void gulc::CodeGen::generateFunctionDecl(gulc::FunctionDecl const* functionDecl, bool isInternal) {
    gulc::StructDecl* parentStruct = nullptr;

    if (functionDecl->container != nullptr && llvm::isa<StructDecl>(functionDecl->container)) {
        parentStruct = llvm::dyn_cast<StructDecl>(functionDecl->container);
    }

    std::vector<llvm::Type*> paramTypes = generateLlvmParamTypes(functionDecl->parameters(), parentStruct);
    llvm::Type* returnType = generateLlvmType(functionDecl->returnType);
    llvm::FunctionType* functionType = llvm::FunctionType::get(returnType, paramTypes, false);
    llvm::Function* function = _llvmModule->getFunction(functionDecl->mangledName());

    if (!function) {
        auto linkageType = llvm::Function::LinkageTypes::ExternalLinkage;

        if (isInternal && !functionDecl->isMainEntry()) {
            linkageType = llvm::Function::LinkageTypes::InternalLinkage;
        }

        function = llvm::Function::Create(functionType, linkageType, functionDecl->mangledName(), _llvmModule);
    }

    llvm::BasicBlock* funcBody = llvm::BasicBlock::Create(*_llvmContext, "entry", function);
    _irBuilder->SetInsertPoint(funcBody);
    setCurrentFunction(function, functionDecl);

    // Generate the function body
    generateStmt(functionDecl->body());

    verifyFunction(*function);
    _funcPassManager->run(*function);

    // Reset the insertion point (this probably isn't needed but oh well)
    _irBuilder->ClearInsertionPoint();
    _currentGhoulFunction = nullptr;
    _currentLlvmFunction = nullptr;
}

void gulc::CodeGen::generateNamespaceDecl(gulc::NamespaceDecl const* namespaceDecl) {
    for (Decl const* decl : namespaceDecl->nestedDecls()) {
        generateDecl(decl, false);
    }
}

void gulc::CodeGen::generatePropertyDecl(gulc::PropertyDecl const* propertyDecl, bool isInternal) {
    // TODO:
}

void gulc::CodeGen::generateStructDecl(gulc::StructDecl const* structDecl, bool isInternal) {
    if (!structDecl->vtable.empty()) {
        // If the struct has a vtable we have to generate a global variable for it...
        llvm::Type* vtableEntryType =
                llvm::PointerType::get(
                        llvm::FunctionType::get(llvm::Type::getVoidTy(*_llvmContext), true), 0);

        std::vector<llvm::Constant*> vtableEntries;

        for (FunctionDecl* functionDecl : structDecl->vtable) {
            llvm::Function* vtableFunction;

            // TODO: Is `functionDecl == structDecl->destructor` correct? I believe we're actually putting the
            //       destructor in the vtable list like this.
            if (structDecl->destructor != nullptr && structDecl->destructor->isAnyVirtual() &&
                    functionDecl == structDecl->destructor) {
                // Destructors DO NOT support parameters except for the single `this` parameter
                std::vector<llvm::Type*> paramTypes = generateLlvmParamTypes({}, structDecl);
                // All constructors return void. We construct the `this` parameter. Memory allocation for the struct is
                // NOT handled by the constructor
                llvm::Type* returnType = llvm::Type::getVoidTy(*_llvmContext);
                llvm::FunctionType* functionType = llvm::FunctionType::get(returnType, paramTypes, false);
                vtableFunction = _llvmModule->getFunction(structDecl->destructor->mangledName());

                if (!vtableFunction) {
                    auto linkageType = llvm::Function::LinkageTypes::ExternalLinkage;

                    if (isInternal) {
                        linkageType = llvm::Function::LinkageTypes::InternalLinkage;
                    }

                    vtableFunction = llvm::Function::Create(functionType, linkageType,
                                                            structDecl->destructor->mangledName(), _llvmModule);
                }
            } else {
                vtableFunction = getFunction(functionDecl);
            }

            vtableEntries.push_back(llvm::ConstantExpr::getBitCast(vtableFunction, vtableEntryType));
        }

        llvm::ArrayType* vtableType = llvm::ArrayType::get(vtableEntryType, vtableEntries.size());

        llvm::Constant* llvmVTableEntries = llvm::ConstantArray::get(vtableType, vtableEntries);

        new llvm::GlobalVariable(*_llvmModule, vtableType, false,
                                 llvm::GlobalVariable::LinkageTypes::ExternalLinkage,
                                 llvmVTableEntries, structDecl->vtableName);
    }

    for (ConstructorDecl const* constructor : structDecl->constructors()) {
        // TODO: If the constructor is `private` then `isInternal` must be set to true even if our `isInternal` is false
        //  NOTE: We don't do this for `internal` since `internal` can still be accessed by other objects in the same
        //   project
        generateConstructorDecl(constructor, isInternal);
    }

    for (Decl const* decl : structDecl->ownedMembers()) {
        // TODO: We need to support the other member functions...
        if (llvm::isa<FunctionDecl>(decl)) {
            generateFunctionDecl(llvm::dyn_cast<FunctionDecl>(decl), isInternal);
        } else if (llvm::isa<TemplateFunctionDecl>(decl)) {
            generateTemplateFunctionDecl(llvm::dyn_cast<TemplateFunctionDecl>(decl), isInternal);
        }
    }

    if (structDecl->destructor != nullptr) {
        generateDestructorDecl(structDecl->destructor, isInternal);
    }
}

void gulc::CodeGen::generateTemplateFunctionDecl(gulc::TemplateFunctionDecl const* templateFunctionDecl,
                                                 bool isInternal) {
    // TODO: Is there anything else to do besides pass the instantiations to the normal generators?
    for (auto templateFunctionInst : templateFunctionDecl->templateInstantiations()) {
        generateFunctionDecl(templateFunctionInst, isInternal);
    }
}

void gulc::CodeGen::generateTemplateStructDecl(gulc::TemplateStructDecl const* templateStructDecl, bool isInternal) {
    // TODO: Is there anything else to do besides pass the instantiations to the normal generators?
    for (auto templateStructInst : templateStructDecl->templateInstantiations()) {
        generateStructDecl(templateStructInst, isInternal);
    }
}

void gulc::CodeGen::generateTemplateTraitDecl(gulc::TemplateTraitDecl const* templateTraitDecl, bool isInternal) {
    // TODO: Is there anything else to do besides pass the instantiations to the normal generators?
    for (auto templateTraitInst : templateTraitDecl->templateInstantiations()) {
        generateTraitDecl(templateTraitInst, isInternal);
    }
}

void gulc::CodeGen::generateTraitDecl(gulc::TraitDecl const* traitDecl, bool isInternal) {
    // TODO:
}

void gulc::CodeGen::generateVariableDecl(gulc::VariableDecl const* variableDecl, bool isInternal) {
    llvm::GlobalVariable* checkExtern = _llvmModule->getGlobalVariable(variableDecl->identifier().name(), true);

    llvm::Constant* initialValue = nullptr;

    if (variableDecl->hasInitialValue()) {
        initialValue = generateConstant(variableDecl->initialValue);
    }

    if (checkExtern) {
        if (initialValue) {
            checkExtern->setInitializer(initialValue);
        }

        return;
    } else {
        llvm::Type* llvmType = generateLlvmType(variableDecl->type);

        bool isConstant = variableDecl->type->qualifier() == Type::Qualifier::Immut;

        auto linkageType = llvm::Function::LinkageTypes::ExternalLinkage;

        if (isInternal) {
            linkageType = llvm::Function::LinkageTypes::InternalLinkage;
        }

        new llvm::GlobalVariable(*_llvmModule, llvmType, isConstant, linkageType, initialValue,
                                 variableDecl->mangledName());
    }
}

void gulc::CodeGen::setCurrentFunction(llvm::Function* currentFunction,
                                       gulc::FunctionDecl const* currentGhoulFunction) {
    if (_entryBlockBuilder) {
        delete _entryBlockBuilder;
        _entryBlockBuilder = nullptr;
    }

    _currentLlvmFunctionParameters.clear();
    _currentLlvmFunctionLocalVariables.clear();
    _currentLlvmFunctionLabels.clear();

    _currentLlvmFunction = currentFunction;
    _currentGhoulFunction = currentGhoulFunction;
    _entryBlockBuilder = new llvm::IRBuilder<>(&currentFunction->getEntryBlock(),
                                               currentFunction->getEntryBlock().begin());

    for (llvm::Argument& arg : currentFunction->args()) {
        llvm::AllocaInst* allocaInst = _entryBlockBuilder->CreateAlloca(arg.getType(), nullptr);

        _currentLlvmFunctionParameters.push_back(allocaInst);

        _entryBlockBuilder->CreateStore(&arg, allocaInst);
    }
}

llvm::Function* gulc::CodeGen::getFunction(gulc::FunctionDecl* functionDecl) {
    gulc::StructDecl* parentStruct = nullptr;

    if (functionDecl->container != nullptr && llvm::isa<StructDecl>(functionDecl->container)) {
        parentStruct = llvm::dyn_cast<StructDecl>(functionDecl->container);
    }

    std::vector<llvm::Type*> paramTypes = generateLlvmParamTypes(functionDecl->parameters(), parentStruct);
    llvm::Type* returnType = generateLlvmType(functionDecl->returnType);
    llvm::FunctionType* functionType = llvm::FunctionType::get(returnType, paramTypes, false);

    // TODO: Why does this return `llvm::Constant*` instead of `llvm::Function*`?
    return llvm::dyn_cast<llvm::Function>(_llvmModule->getOrInsertFunction(functionDecl->mangledName(), functionType));
}

// Statement Generation
void gulc::CodeGen::generateStmt(gulc::Stmt const* stmt, std::string const& stmtName) {
    if (!stmt->temporaryValues.empty()) {
        for (auto tempValLocalVar : stmt->temporaryValues) {
            generateVariableDeclExpr(tempValLocalVar);
        }
    }

    switch (stmt->getStmtKind()) {
        case Stmt::Kind::Break:
            generateBreakStmt(llvm::dyn_cast<BreakStmt>(stmt));
            break;
        case Stmt::Kind::Compound:
            generateCompoundStmt(llvm::dyn_cast<CompoundStmt>(stmt));
            break;
        case Stmt::Kind::Continue:
            generateContinueStmt(llvm::dyn_cast<ContinueStmt>(stmt));
            break;
        case Stmt::Kind::DoCatch:
            generateDoCatchStmt(llvm::dyn_cast<DoCatchStmt>(stmt));
            break;
        case Stmt::Kind::DoWhile:
            generateDoWhileStmt(llvm::dyn_cast<DoWhileStmt>(stmt), stmtName);
            break;
        case Stmt::Kind::For:
            generateForStmt(llvm::dyn_cast<ForStmt>(stmt), stmtName);
            break;
        case Stmt::Kind::Goto:
            generateGotoStmt(llvm::dyn_cast<GotoStmt>(stmt));
            break;
        case Stmt::Kind::If:
            generateIfStmt(llvm::dyn_cast<IfStmt>(stmt));
            break;
        case Stmt::Kind::Labeled:
            generateLabeledStmt(llvm::dyn_cast<LabeledStmt>(stmt));
            break;
        case Stmt::Kind::Return:
            generateReturnStmt(llvm::dyn_cast<ReturnStmt>(stmt));
            // NOTE: We `return` here instead of `break` because `generateReturnStmt` already handles the temporary
            //       values for us before returning.
            return;
        case Stmt::Kind::Switch:
            generateSwitchStmt(llvm::dyn_cast<SwitchStmt>(stmt));
            break;
        case Stmt::Kind::While:
            generateWhileStmt(llvm::dyn_cast<WhileStmt>(stmt), stmtName);
            break;

        case Stmt::Kind::Expr:
            generateExpr(llvm::dyn_cast<Expr>(stmt));
            break;
        default:
            printError("[INTERNAL] unsupported statement found in `CodeGen::generateStmt`!",
                       stmt->startPosition(), stmt->endPosition());
            break;
    }

    if (!stmt->temporaryValues.empty()) {
        cleanupTemporaryValues(stmt->temporaryValues);
    }
}

void gulc::CodeGen::generateBreakStmt(gulc::BreakStmt const* breakStmt) {
    if (!breakStmt->hasBreakLabel()) {
        for (Expr* deferredExpr : breakStmt->preBreakDeferred) {
            generateExpr(deferredExpr);
        }

        _irBuilder->CreateBr(_currentLoopBlockBreak);
    } else {
        llvm::BasicBlock* breakBlock = getBreakBlock(breakStmt->breakLabel().name());

        if (breakBlock == nullptr) {
            printError("[INTERNAL] block '" + breakStmt->breakLabel().name() + "' not found!",
                       breakStmt->startPosition(), breakStmt->endPosition());
            return;
        }

        for (Expr* deferredExpr : breakStmt->preBreakDeferred) {
            generateExpr(deferredExpr);
        }

        _irBuilder->CreateBr(breakBlock);
    }
}

void gulc::CodeGen::generateCompoundStmt(gulc::CompoundStmt const* compoundStmt) {
    auto oldLocalVariableCount = _currentLlvmFunctionLocalVariables.size();

    for (Stmt const* stmt : compoundStmt->statements) {
        generateStmt(stmt);
    }

    _currentLlvmFunctionLocalVariables.resize(oldLocalVariableCount);
}

void gulc::CodeGen::generateContinueStmt(gulc::ContinueStmt const* continueStmt) {
    if (continueStmt->hasContinueLabel()) {
        for (Expr* deferredExpr : continueStmt->preContinueDeferred) {
            generateExpr(deferredExpr);
        }

        _irBuilder->CreateBr(_currentLoopBlockContinue);
    } else {
        llvm::BasicBlock* continueBlock = getContinueBlock(continueStmt->continueLabel().name());

        if (continueBlock == nullptr) {
            printError("[INTERNAL] block '" + continueStmt->continueLabel().name() + "' not found!",
                       continueStmt->startPosition(), continueStmt->endPosition());
            return;
        }

        for (Expr* deferredExpr : continueStmt->preContinueDeferred) {
            generateExpr(deferredExpr);
        }

        _irBuilder->CreateBr(continueBlock);
    }
}

void gulc::CodeGen::generateDoCatchStmt(gulc::DoCatchStmt const* doCatchStmt) {
    printError("`do {} catch {}` statement not yet supported!",
               doCatchStmt->startPosition(), doCatchStmt->endPosition());
}

void gulc::CodeGen::generateDoWhileStmt(gulc::DoWhileStmt const* doWhileStmt, std::string const& loopName) {
    std::string doName;

    if (loopName.empty()) {
        doName = "loop" + std::to_string(_anonLoopNameNumber);
        ++_anonLoopNameNumber;
    } else {
        doName = loopName;
    }

    llvm::BasicBlock* loop = llvm::BasicBlock::Create(*_llvmContext, doName + "_loop", _currentLlvmFunction);
    llvm::BasicBlock* loopContinue = llvm::BasicBlock::Create(*_llvmContext, doName + "_continue");
    llvm::BasicBlock* loopBreak = llvm::BasicBlock::Create(*_llvmContext, doName + "_break");

    // For some reason we can't just fall through to the continue loop? We have to explicitly branch to it?
    _irBuilder->CreateBr(loop);
    // Set the insert point to the loop block and start adding the loop data...
    _irBuilder->SetInsertPoint(loop);

    // We make sure to back up and restore the old loop's break and continue blocks for our `break` and `continue` keywords
    auto oldLoopContinue = _currentLoopBlockContinue;
    auto oldLoopBreak = _currentLoopBlockBreak;

    _currentLoopBlockContinue = loopContinue;
    _currentLoopBlockBreak = loopBreak;

    // Generate the statement we loop on...
    std::size_t oldNestedLoopCount = 0;
    enterNestedLoop(loopContinue, loopBreak, &oldNestedLoopCount);
    generateStmt(doWhileStmt->body());
    leaveNestedLoop(oldNestedLoopCount);
    _irBuilder->CreateBr(loopContinue);

    _currentLoopBlockContinue = oldLoopContinue;
    _currentLoopBlockBreak = oldLoopBreak;

    // Add the loop continue block to the function and set it as the insert point...
    _currentLlvmFunction->getBasicBlockList().push_back(loopContinue);
    _irBuilder->SetInsertPoint(loopContinue);

    // Generate the condition and create the conditional branch
    llvm::Value* cond = generateExpr(doWhileStmt->condition);
    _irBuilder->CreateCondBr(cond, loop, loopBreak);

    // Add the loop break block to the function and set it as the insert point...
    _currentLlvmFunction->getBasicBlockList().push_back(loopBreak);
    _irBuilder->SetInsertPoint(loopBreak);
}

void gulc::CodeGen::generateForStmt(gulc::ForStmt const* forStmt, std::string const& loopName) {
    if (forStmt->init != nullptr) {
        generateExpr(forStmt->init);
    }

    std::string forName;

    if (loopName.empty()) {
        forName = "loop" + std::to_string(_anonLoopNameNumber);
        ++_anonLoopNameNumber;
    } else {
        forName = loopName;
    }

    llvm::BasicBlock* loop = llvm::BasicBlock::Create(*_llvmContext, forName + "_loop", _currentLlvmFunction);
    llvm::BasicBlock* hiddenContinueLoop = llvm::BasicBlock::Create(*_llvmContext, forName + "_hidden_continue");
    llvm::BasicBlock* continueLoop = llvm::BasicBlock::Create(*_llvmContext, forName + "_continue");
    llvm::BasicBlock* breakLoop = llvm::BasicBlock::Create(*_llvmContext, forName + "_break");

    // Set the loop as the current insert point
    _irBuilder->CreateBr(loop);
    _irBuilder->SetInsertPoint(loop);

    if (forStmt->condition != nullptr) {
        llvm::Value *cond = generateExpr(forStmt->condition);
        // If the condition is true we continue the loop, if not we break from the loop...
        _irBuilder->CreateCondBr(cond, hiddenContinueLoop, breakLoop);
    } else {
        _irBuilder->CreateBr(hiddenContinueLoop);
    }

    // Set the hidden continue loop as the current insert point
    _currentLlvmFunction->getBasicBlockList().push_back(hiddenContinueLoop);
    _irBuilder->SetInsertPoint(hiddenContinueLoop);

    // We make sure to back up and restore the old loop's break and continue blocks for our `break` and `continue` keywords
    llvm::BasicBlock* oldLoopContinue = _currentLoopBlockContinue;
    llvm::BasicBlock* oldLoopBreak = _currentLoopBlockBreak;

    _currentLoopBlockContinue = continueLoop;
    _currentLoopBlockBreak = breakLoop;

    // Generate the statement we loop on
    std::size_t oldNestedLoopCount = 0;
    enterNestedLoop(continueLoop, breakLoop, &oldNestedLoopCount);
    generateStmt(forStmt->body());
    leaveNestedLoop(oldNestedLoopCount);

    _currentLoopBlockContinue = oldLoopContinue;
    _currentLoopBlockBreak = oldLoopBreak;

    // Now we go to our actual continue block, the continue block has to be here so we apply the 'iterationExpr'
    _irBuilder->CreateBr(continueLoop);
    _currentLlvmFunction->getBasicBlockList().push_back(continueLoop);
    _irBuilder->SetInsertPoint(continueLoop);

    // Generate the iteration expression (usually `++i`)
    if (forStmt->iteration != nullptr) generateExpr(forStmt->iteration);

    // Branch back to the beginning of our loop...
    _irBuilder->CreateBr(loop);

    // And then finish off by adding the break point.
    _currentLlvmFunction->getBasicBlockList().push_back(breakLoop);
    _irBuilder->SetInsertPoint(breakLoop);

    // TODO: Post loop cleanup
//    for (Expr* postLoopCleanupExpr : forStmt->postLoopCleanup) {
//        generateExpr(postLoopCleanupExpr);
//    }
}

void gulc::CodeGen::generateGotoStmt(gulc::GotoStmt const* gotoStmt) {
    for (Expr* deferredExpr : gotoStmt->preGotoDeferred) {
        generateExpr(deferredExpr);
    }

    if (currentFunctionLabelsContains(gotoStmt->label().name())) {
        _irBuilder->CreateBr(_currentLlvmFunctionLabels[gotoStmt->label().name()]);
    } else {
        llvm::BasicBlock* newBasicBlock = llvm::BasicBlock::Create(*_llvmContext, gotoStmt->label().name());
        _irBuilder->CreateBr(newBasicBlock);
        addCurrentFunctionLabel(gotoStmt->label().name(), newBasicBlock);
    }
}

void gulc::CodeGen::generateIfStmt(gulc::IfStmt const* ifStmt) {
    llvm::Value* cond = generateExpr(ifStmt->condition);

    llvm::BasicBlock* trueBlock = llvm::BasicBlock::Create(*_llvmContext, "ifTrueBlock", _currentLlvmFunction);
    llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(*_llvmContext, "ifMerge");
    llvm::BasicBlock* falseBlock = nullptr;

    // If there isn't a false block we make the IR jump to the merge block on false, else we make an actual false block
    if (ifStmt->hasFalseBody()) {
        falseBlock = llvm::BasicBlock::Create(*_llvmContext, "ifFalseBlock");

        _irBuilder->CreateCondBr(cond, trueBlock, falseBlock);
    } else {
        _irBuilder->CreateCondBr(cond, trueBlock, mergeBlock);
    }

    // Set the insert point to our true block then generate the statement for it...
    _irBuilder->SetInsertPoint(trueBlock);
    generateStmt(ifStmt->trueBody());

    // If the last inserted instruction is a branch and we insert a branch after it to the merge block an LLVM pass
    // will mess up and remove the entire if statement...
    if (_irBuilder->GetInsertBlock()->getTerminator() == nullptr ||
        !llvm::isa<llvm::BranchInst>(_irBuilder->GetInsertBlock()->getTerminator())) {
        _irBuilder->CreateBr(mergeBlock);
    }

    // And then add a jump to the merge block if there is a false statement
    if (ifStmt->hasFalseBody()) {
        // Set the insert point to the false block and then generate the statement for it...
        _currentLlvmFunction->getBasicBlockList().push_back(falseBlock);
        _irBuilder->SetInsertPoint(falseBlock);
        generateStmt(ifStmt->falseBody());

        // If the last inserted instruction is a branch and we insert a branch after it to the merge block an LLVM pass
        // will mess up and remove the entire if statement...
        if (_irBuilder->GetInsertBlock()->getTerminator() == nullptr ||
            !llvm::isa<llvm::BranchInst>(_irBuilder->GetInsertBlock()->getTerminator())) {
            // Branch to merge when done...
            _irBuilder->CreateBr(mergeBlock);
        }
    }

    // Add the merge block to the function and then set the insert point to it
    _currentLlvmFunction->getBasicBlockList().push_back(mergeBlock);
    _irBuilder->SetInsertPoint(mergeBlock);
}

void gulc::CodeGen::generateLabeledStmt(gulc::LabeledStmt const* labeledStmt) {
    // Curious why we can't just use normal labels...
    llvm::BasicBlock* labelBody;

    if (currentFunctionLabelsContains(labeledStmt->label().name())) {
        labelBody = _currentLlvmFunctionLabels[labeledStmt->label().name()];
    } else {
        labelBody = llvm::BasicBlock::Create(*_llvmContext, labeledStmt->label().name());
        addCurrentFunctionLabel(labeledStmt->label().name(), labelBody);
    }

    // We have to explicitly branch to blocks for some reason...
    _irBuilder->CreateBr(labelBody);
    addBlockAndSetInsertionPoint(labelBody);

    generateStmt(labeledStmt->labeledStmt, labeledStmt->label().name());
}

void gulc::CodeGen::generateReturnStmt(gulc::ReturnStmt const* returnStmt) {
    if (returnStmt->returnValue != nullptr) {
        llvm::Value* returnValue = generateExpr(returnStmt->returnValue);

        for (Expr* deferredExpr : returnStmt->preReturnDeferred) {
            generateExpr(deferredExpr);
        }

        cleanupTemporaryValues(returnStmt->temporaryValues);

        _irBuilder->CreateRet(returnValue);
    } else {
        for (Expr* deferredExpr : returnStmt->preReturnDeferred) {
            generateExpr(deferredExpr);
        }

        cleanupTemporaryValues(returnStmt->temporaryValues);

        _irBuilder->CreateRetVoid();
    }
}

void gulc::CodeGen::generateSwitchStmt(gulc::SwitchStmt const* switchStmt) {
    // TODO: Make `switch` only support `CaseStmt` and `DefaultCaseStmt`. No more having a list of unknown statements,
    //       it's too hard to deal with.
    printError("`switch` statement not yet supported!",
               switchStmt->startPosition(), switchStmt->endPosition());
}

void gulc::CodeGen::generateWhileStmt(gulc::WhileStmt const* whileStmt, std::string const& loopName) {
    std::string whileName;

    if (loopName.empty()) {
        whileName = "loop" + std::to_string(_anonLoopNameNumber);
        ++_anonLoopNameNumber;
    } else {
        whileName = loopName;
    }

    llvm::BasicBlock* continueLoop = llvm::BasicBlock::Create(*_llvmContext, whileName + "_continue", _currentLlvmFunction);
    llvm::BasicBlock* loop = llvm::BasicBlock::Create(*_llvmContext, whileName + "_loop", _currentLlvmFunction);
    llvm::BasicBlock* breakLoop = llvm::BasicBlock::Create(*_llvmContext, whileName + "_break");

    // For some reason we can't just fall through to the continue loop? We have to explicitly branch to it?
    _irBuilder->CreateBr(continueLoop);

    // Set the insert point to the continue loop block and start adding the loop data...
    _irBuilder->SetInsertPoint(continueLoop);

    llvm::Value* cond = generateExpr(whileStmt->condition);
    // If the condition is true we continue the loop, if not we break from the loop...
    _irBuilder->CreateCondBr(cond, loop, breakLoop);

    // Set the insert point to the loop block for our actual statement...
    _irBuilder->SetInsertPoint(loop);

    // We make sure to back up and restore the old loop's break and continue blocks for our `break` and `continue` keywords
    llvm::BasicBlock* oldLoopContinue = _currentLoopBlockContinue;
    llvm::BasicBlock* oldLoopBreak = _currentLoopBlockBreak;

    _currentLoopBlockContinue = continueLoop;
    _currentLoopBlockBreak = breakLoop;

    // Generate the loop statement within the loop block then jump back to the continue block...
    std::size_t oldNestedLoopCount = 0;
    enterNestedLoop(continueLoop, breakLoop, &oldNestedLoopCount);
    generateStmt(whileStmt->body());
    leaveNestedLoop(oldNestedLoopCount);
    _irBuilder->CreateBr(continueLoop);

    _currentLoopBlockContinue = oldLoopContinue;
    _currentLoopBlockBreak = oldLoopBreak;

    // Finish by adding the break loop block and setting the insert point to it...
    _currentLlvmFunction->getBasicBlockList().push_back(breakLoop);
    _irBuilder->SetInsertPoint(breakLoop);
}

void gulc::CodeGen::enterNestedLoop(llvm::BasicBlock* continueLoop, llvm::BasicBlock* breakLoop,
                                    std::size_t* outOldNestedLoopCount) {
    *outOldNestedLoopCount = _nestedLoopContinues.size();

    _nestedLoopContinues.push_back(continueLoop);
    _nestedLoopBreaks.push_back(breakLoop);
}

void gulc::CodeGen::leaveNestedLoop(std::size_t oldNestedLoopCount) {
    _nestedLoopContinues.resize(oldNestedLoopCount);
    _nestedLoopBreaks.resize(oldNestedLoopCount);
}

void gulc::CodeGen::cleanupTemporaryValues(std::vector<VariableDeclExpr*> const& temporaryValues) {
    for (auto temporaryValue : temporaryValues) {
        llvm::AllocaInst* localVariableAlloca = getLocalVariableOrNull(temporaryValue->identifier().name());

        if (localVariableAlloca == nullptr) {
            printError("[INTERNAL] temporary value variable was not found!",
                       temporaryValue->startPosition(), temporaryValue->endPosition());
        }

        if (llvm::isa<StructType>(temporaryValue->type)) {
            auto structDecl = llvm::dyn_cast<StructType>(temporaryValue->type)->decl();

            // Because we only work on by value structs we don't have to deal with the vtable, we can know the
            // struct is the correct type.
            // TODO: Is this a correct assumption?
            auto destructorFunc = _llvmModule->getFunction(structDecl->destructor->mangledName());

            std::vector<llvm::Value*> llvmArgs {
                    localVariableAlloca
            };

            _irBuilder->CreateCall(destructorFunc, llvmArgs);
        }
    }
}

bool gulc::CodeGen::currentFunctionLabelsContains(std::string const& labelName) {
    return _currentLlvmFunctionLabels.find(labelName) != _currentLlvmFunctionLabels.end();
}

void gulc::CodeGen::addCurrentFunctionLabel(std::string const& labelName, llvm::BasicBlock* basicBlock) {
    if (_currentLlvmFunctionLabels.find(labelName) == _currentLlvmFunctionLabels.end()) {
        _currentLlvmFunctionLabels.insert({labelName, basicBlock});
    }
}

void gulc::CodeGen::addBlockAndSetInsertionPoint(llvm::BasicBlock* basicBlock) {
    _currentLlvmFunction->getBasicBlockList().push_back(basicBlock);
    _irBuilder->SetInsertPoint(basicBlock);
}

llvm::BasicBlock* gulc::CodeGen::getBreakBlock(std::string const& blockName) {
    std::string breakBlockName = blockName + "_break";

    for (llvm::BasicBlock* checkBlock : _nestedLoopBreaks) {
        if (checkBlock->getName() == breakBlockName) {
            return checkBlock;
        }
    }

    return nullptr;
}

llvm::BasicBlock* gulc::CodeGen::getContinueBlock(std::string const& blockName) {
    std::string continueBlockName = blockName + "_continue";

    for (llvm::BasicBlock* checkBlock : _nestedLoopContinues) {
        if (checkBlock->getName() == continueBlockName) {
            return checkBlock;
        }
    }

    return nullptr;
}

// Expression Generation
llvm::Constant* gulc::CodeGen::generateConstant(gulc::Expr const* expr) {
    if (llvm::isa<ValueLiteralExpr>(expr)) {
        auto valueLiteralExpr = llvm::dyn_cast<ValueLiteralExpr>(expr);

        switch (valueLiteralExpr->literalType()) {
            case ValueLiteralExpr::LiteralType::Integer: {
                llvm::Type* constantType = nullptr;

                if (valueLiteralExpr->valueType != nullptr) {
                    constantType = generateLlvmType(valueLiteralExpr->valueType);
                } else {
                    // We default to `i32`
                    constantType = llvm::Type::getInt32Ty(*_llvmContext);
                }

                // TODO: We need to split the value literals out into their own declarations...
                return llvm::ConstantInt::get(llvm::dyn_cast<llvm::IntegerType>(constantType),
                                              valueLiteralExpr->value(), 10);
            }
            case ValueLiteralExpr::LiteralType::Float: {
                llvm::Type* constantType = nullptr;

                if (valueLiteralExpr->valueType != nullptr) {
                    constantType = generateLlvmType(valueLiteralExpr->valueType);
                } else {
                    // We default to `f32`
                    constantType = llvm::Type::getFloatTy(*_llvmContext);
                }

                return llvm::ConstantFP::get(constantType, valueLiteralExpr->value());
            }
            case ValueLiteralExpr::LiteralType::Char: {
                printError("[INTERNAL] character literals not yet supported!",
                           valueLiteralExpr->startPosition(), valueLiteralExpr->endPosition());
                break;
            }
            case ValueLiteralExpr::LiteralType::String: {
                printError("[INTERNAL] string literals not yet supported!",
                           valueLiteralExpr->startPosition(), valueLiteralExpr->endPosition());
                break;
            }
        }
    }

    printError("unsupported constant in codegen!", expr->startPosition(), expr->endPosition());
    return nullptr;
}

llvm::Value* gulc::CodeGen::generateExpr(gulc::Expr const* expr) {
    switch (expr->getExprKind()) {
        case Expr::Kind::ArrayLiteral:
            return generateArrayLiteralExpr(llvm::dyn_cast<ArrayLiteralExpr>(expr));
        case Expr::Kind::As:
            return generateAsExpr(llvm::dyn_cast<AsExpr>(expr));
        case Expr::Kind::AssignmentOperator:
            return generateAssignmentOperatorExpr(llvm::dyn_cast<AssignmentOperatorExpr>(expr));
        case Expr::Kind::BoolLiteral:
            return generateBoolLiteralExpr(llvm::dyn_cast<BoolLiteralExpr>(expr));
        case Expr::Kind::ConstructorCall:
            return generateConstructorCallExpr(llvm::dyn_cast<ConstructorCallExpr>(expr));
        case Expr::Kind::CurrentSelf:
            return generateCurrentSelfExpr(llvm::dyn_cast<CurrentSelfExpr>(expr));
        case Expr::Kind::DestructorCall:
            return generateDestructorCallExpr(llvm::dyn_cast<DestructorCallExpr>(expr));
        case Expr::Kind::EnumConstRef:
            return generateEnumConstRefExpr(llvm::dyn_cast<EnumConstRefExpr>(expr));
        case Expr::Kind::FunctionCall:
            return generateFunctionCallExpr(llvm::dyn_cast<FunctionCallExpr>(expr));
        case Expr::Kind::ImplicitCast:
            return generateImplicitCastExpr(llvm::dyn_cast<ImplicitCastExpr>(expr));
        case Expr::Kind::InfixOperator:
            return generateInfixOperatorExpr(llvm::dyn_cast<InfixOperatorExpr>(expr));
        case Expr::Kind::LocalVariableRef:
            return generateLocalVariableRefExpr(llvm::dyn_cast<LocalVariableRefExpr>(expr));
        case Expr::Kind::LValueToRValue:
            return generateLValueToRValueExpr(llvm::dyn_cast<LValueToRValueExpr>(expr));
        case Expr::Kind::MemberFunctionCall:
            return generateMemberFunctionCallExpr(llvm::dyn_cast<MemberFunctionCallExpr>(expr));
        case Expr::Kind::MemberPostfixOperatorCall:
            return generateMemberPostfixOperatorCallExpr(llvm::dyn_cast<MemberPostfixOperatorCallExpr>(expr));
        case Expr::Kind::MemberPrefixOperatorCall:
            return generateMemberPrefixOperatorCallExpr(llvm::dyn_cast<MemberPrefixOperatorCallExpr>(expr));
        case Expr::Kind::MemberVariableRef:
            return generateMemberVariableRefExpr(llvm::dyn_cast<MemberVariableRefExpr>(expr));
        case Expr::Kind::ParameterRef:
            return generateParameterRefExpr(llvm::dyn_cast<ParameterRefExpr>(expr));
        case Expr::Kind::Paren:
            return generateParenExpr(llvm::dyn_cast<ParenExpr>(expr));
        case Expr::Kind::PostfixOperator:
            return generatePostfixOperatorExpr(llvm::dyn_cast<PostfixOperatorExpr>(expr));
        case Expr::Kind::PrefixOperator:
            return generatePrefixOperatorExpr(llvm::dyn_cast<PrefixOperatorExpr>(expr));
        case Expr::Kind::Ternary:
            return generateTernaryExpr(llvm::dyn_cast<TernaryExpr>(expr));
        case Expr::Kind::Try:
            return generateTryExpr(llvm::dyn_cast<TryExpr>(expr));
        case Expr::Kind::ValueLiteral:
            return generateValueLiteralExpr(llvm::dyn_cast<ValueLiteralExpr>(expr));
        case Expr::Kind::VariableDecl:
            return generateVariableDeclExpr(llvm::dyn_cast<VariableDeclExpr>(expr));
        case Expr::Kind::VariableRef:
            return generateVariableRefExpr(llvm::dyn_cast<VariableRefExpr>(expr));
        case Expr::Kind::VTableFunctionReference:
            return generateVTableFunctionReferenceExpr(llvm::dyn_cast<VTableFunctionReferenceExpr>(expr));
        default:
            printError("unsupported expression!",
                       expr->startPosition(), expr->endPosition());
            return nullptr;
    }
}

llvm::Value* gulc::CodeGen::generateArrayLiteralExpr(gulc::ArrayLiteralExpr const* arrayLiteralExpr) {
    printError("array literals not yet supported!",
               arrayLiteralExpr->startPosition(), arrayLiteralExpr->endPosition());
    return nullptr;
}

llvm::Value* gulc::CodeGen::generateAsExpr(gulc::AsExpr const* asExpr) {
    llvm::Value* value = generateExpr(asExpr->expr);

    // TODO: Should we be dereferencing any references here?
    castValue(asExpr->asType, asExpr->expr->valueType, value,
              asExpr->startPosition(), asExpr->endPosition());

    return value;
}

llvm::Value* gulc::CodeGen::generateAssignmentOperatorExpr(gulc::AssignmentOperatorExpr const* assignmentOperatorExpr) {
    llvm::Value* leftValue = generateExpr(assignmentOperatorExpr->leftValue);
    llvm::Value* rightValue = generateExpr(assignmentOperatorExpr->rightValue);

    if (assignmentOperatorExpr->hasNestedOperator()) {
        // TODO: Support operator overloading

        gulc::Type* leftType = assignmentOperatorExpr->leftValue->valueType;
        gulc::Type* rightType = assignmentOperatorExpr->rightValue->valueType;
        // This is the actual type we use for determining the operation we use.
        // TODO: Support pointer arithmetic
        Type* resultType = leftType;

        return generateBuiltInInfixOperator(
                assignmentOperatorExpr->nestedOperator(), resultType,
                leftType, leftValue, rightType, rightValue,
                assignmentOperatorExpr->startPosition(), assignmentOperatorExpr->endPosition()
            );
    }

    // Finish by storing either the original right value or the nested operator result into the left value
    _irBuilder->CreateStore(rightValue, leftValue, false);

    // We ALWAYS return the left value for assignments
    return leftValue;
}

llvm::Value* gulc::CodeGen::generateBoolLiteralExpr(gulc::BoolLiteralExpr const* boolLiteralExpr) {
    // TODO: Is this right? Is there no constant bool?
    return llvm::ConstantInt::get(llvm::IntegerType::getInt8Ty(*_llvmContext), boolLiteralExpr->value(), false);
}

llvm::Value* gulc::CodeGen::generateCallOperatorReferenceExpr(
        gulc::CallOperatorReferenceExpr const* callOperatorReferenceExpr) {
    return nullptr;
}

llvm::Value* gulc::CodeGen::generateConstructorCallExpr(gulc::ConstructorCallExpr const* constructorCallExpr) {
    llvm::Value* objectRef = generateExpr(constructorCallExpr->objectRef);

    auto constructorReferenceExpr = llvm::dyn_cast<gulc::ConstructorReferenceExpr>(
            constructorCallExpr->functionReference);

    // TODO: Should this be `vtable` or not?
    llvm::Function* constructorFunc =
            _llvmModule->getFunction(constructorReferenceExpr->constructor->mangledName());

    std::vector<llvm::Value*> llvmArgs{};
    llvmArgs.reserve(constructorCallExpr->arguments.size() + 1);

    llvmArgs.push_back(objectRef);

    for (LabeledArgumentExpr* argument : constructorCallExpr->arguments) {
        llvmArgs.push_back(generateExpr(argument->argument));
    }

    _irBuilder->CreateCall(constructorFunc, llvmArgs);

    return objectRef;
}

llvm::Value* gulc::CodeGen::generateCurrentSelfExpr(gulc::CurrentSelfExpr const* currentSelfExpr) {
    // NOTE: `self` is always parameter `0`. Error checking to make sure a `self` parameter exists should be performed
    //       in a prior check.
    return _currentLlvmFunctionParameters[0];
}

llvm::Value* gulc::CodeGen::generateDestructorCallExpr(gulc::DestructorCallExpr const* destructorCallExpr) {
    auto destructorReferenceExpr = llvm::dyn_cast<DestructorReferenceExpr>(destructorCallExpr->functionReference);
    llvm::Function* destructorFunc = _llvmModule->getFunction(destructorReferenceExpr->destructor->mangledName());

    std::vector<llvm::Value*> llvmArgs {
        generateExpr(destructorCallExpr->objectRef)
    };

    return _irBuilder->CreateCall(destructorFunc, llvmArgs);
}

llvm::Value* gulc::CodeGen::generateEnumConstRefExpr(gulc::EnumConstRefExpr const* enumConstRefExpr) {
    return generateExpr(enumConstRefExpr->enumConst()->constValue);
}

llvm::Value* gulc::CodeGen::generateFunctionCallExpr(gulc::FunctionCallExpr const* functionCallExpr) {
    llvm::Value* functionPointer = generateFunctionReferenceFromExpr(functionCallExpr->functionReference);

    std::vector<llvm::Value*> llvmArgs{};

    if (functionCallExpr->hasArguments()) {
        llvmArgs.reserve(functionCallExpr->arguments.size());

        for (gulc::LabeledArgumentExpr *labeledArgumentExpr : functionCallExpr->arguments) {
            llvmArgs.push_back(generateExpr(labeledArgumentExpr->argument));
        }
    }

    return _irBuilder->CreateCall(functionPointer, llvmArgs);
}

llvm::Value* gulc::CodeGen::generateFunctionReferenceFromExpr(gulc::Expr const* expr) {
    if (llvm::isa<FunctionReferenceExpr>(expr)) {
        auto functionReferenceExpr = llvm::dyn_cast<FunctionReferenceExpr>(expr);

        return _llvmModule->getFunction(functionReferenceExpr->functionDecl()->mangledName());
    } else {
        llvm::Value* functionPointerValue = generateExpr(expr);

        if (!llvm::isa<FunctionPointerType>(expr->valueType)) {
            printError("[INTERNAL] `CodeGen::generateFunctionReferenceFromExpr` received non function pointer!",
                       expr->startPosition(), expr->endPosition());
            return nullptr;
        }

        return functionPointerValue;
    }
}

llvm::Value* gulc::CodeGen::generateImplicitCastExpr(gulc::ImplicitCastExpr const* implicitCastExpr) {
    llvm::Value* value = generateExpr(implicitCastExpr->expr);
    castValue(implicitCastExpr->castToType, implicitCastExpr->expr->valueType, value,
              implicitCastExpr->startPosition(), implicitCastExpr->endPosition());
    return value;
}

llvm::Value* gulc::CodeGen::generateInfixOperatorExpr(gulc::InfixOperatorExpr const* infixOperatorExpr) {
    llvm::Value* leftValue = generateExpr(infixOperatorExpr->leftValue);
    llvm::Value* rightValue = generateExpr(infixOperatorExpr->rightValue);

    Type* leftType = infixOperatorExpr->leftValue->valueType;
    Type* rightType = infixOperatorExpr->rightValue->valueType;

    // This is the actual type we use for determining the operation we use.
    // TODO: Support pointer arithmetic
    Type* resultType = infixOperatorExpr->valueType;

    return generateBuiltInInfixOperator(
            infixOperatorExpr->infixOperator(), resultType,
            leftType, leftValue, rightType, rightValue,
            infixOperatorExpr->startPosition(), infixOperatorExpr->endPosition()
        );
}

llvm::Value* gulc::CodeGen::generateBuiltInInfixOperator(InfixOperators infixOperator, gulc::Type* operationType,
                                                         gulc::Type* leftType, llvm::Value* leftValue,
                                                         gulc::Type* rightType, llvm::Value* rightValue,
                                                         TextPosition startPosition, TextPosition endPosition) {
    bool isSigned = false;
    bool isFloat = false;

    if (llvm::isa<BuiltInType>(operationType)) {
        auto builtInType = llvm::dyn_cast<BuiltInType>(operationType);

        isFloat = builtInType->isFloating();
        isSigned = builtInType->isSigned();
    } else if (!llvm::isa<PointerType>(operationType)) {
        printError("unknown infix operator expression!",
                   startPosition, endPosition);
        return nullptr;
    }

    switch (infixOperator) {
        case InfixOperators::Add: // +
            if (isFloat) {
                return _irBuilder->CreateFAdd(leftValue, rightValue);
            } else {
                return _irBuilder->CreateAdd(leftValue, rightValue);
            }
        case InfixOperators::Subtract: // -
            if (isFloat) {
                return _irBuilder->CreateFSub(leftValue, rightValue);
            } else {
                return _irBuilder->CreateSub(leftValue, rightValue);
            }
        case InfixOperators::Multiply: // *
            if (isFloat) {
                return _irBuilder->CreateFMul(leftValue, rightValue);
            } else {
                return _irBuilder->CreateMul(leftValue, rightValue);
            }
        case InfixOperators::Divide: // /
            if (isFloat) {
                return _irBuilder->CreateFDiv(leftValue, rightValue);
            } else {
                // TODO: What are the `Exact` variants?
                if (isSigned) {
                    return _irBuilder->CreateSDiv(leftValue, rightValue);
                } else {
                    return _irBuilder->CreateUDiv(leftValue, rightValue);
                }
            }
        case InfixOperators::Remainder: // %
            if (isFloat) {
                return _irBuilder->CreateFRem(leftValue, rightValue);
            } else {
                // TODO: What are the `Exact` variants?
                if (isSigned) {
                    return _irBuilder->CreateSRem(leftValue, rightValue);
                } else {
                    return _irBuilder->CreateURem(leftValue, rightValue);
                }
            }
        case InfixOperators::Power: // ^^ (exponents)
            printError("infix operator `^^` does not have a built in operation, it must be overloaded to provide "
                       "functionality!",
                       startPosition, endPosition);
            return nullptr;

        case InfixOperators::BitwiseAnd: // &
        case InfixOperators::LogicalAnd: // &&
            // NOTE: In Ghoul since we don't implicitly convert from any type to `bool` both `&` and `&&` are
            //       technically the same once we reach code generation.
            return _irBuilder->CreateAnd(leftValue, rightValue);
        case InfixOperators::BitwiseOr: // |
        case InfixOperators::LogicalOr: // ||
            // NOTE: In Ghoul since we don't implicitly convert from any type to `bool` both `|` and `||` are
            //       technically the same once we reach code generation.
            return _irBuilder->CreateOr(leftValue, rightValue);
        case InfixOperators::BitwiseXor: // ^
            return _irBuilder->CreateXor(leftValue, rightValue);

        case InfixOperators::BitshiftLeft: // << (logical shift left)
            return _irBuilder->CreateShl(leftValue, rightValue);
        case InfixOperators::BitshiftRight: // >> (logical shift right OR arithmetic shift right, depending on the type)
            if (isSigned) {
                return _irBuilder->CreateAShr(leftValue, rightValue);
            } else {
                return _irBuilder->CreateLShr(leftValue, rightValue);
            }

        case InfixOperators::EqualTo: // ==
            if (isFloat) {
                return _irBuilder->CreateFCmpOEQ(leftValue, rightValue);
            } else {
                return _irBuilder->CreateICmpEQ(leftValue, rightValue);
            }
        case InfixOperators::NotEqualTo: // !=
            if (isFloat) {
                return _irBuilder->CreateFCmpONE(leftValue, rightValue);
            } else {
                return _irBuilder->CreateICmpNE(leftValue, rightValue);
            }

        case InfixOperators::GreaterThan: // >
            if (isFloat) {
                return _irBuilder->CreateFCmpOGT(leftValue, rightValue);
            } else if (isSigned) {
                return _irBuilder->CreateICmpSGT(leftValue, rightValue);
            } else {
                return _irBuilder->CreateICmpUGT(leftValue, rightValue);
            }
        case InfixOperators::LessThan: // <
            if (isFloat) {
                return _irBuilder->CreateFCmpOLT(leftValue, rightValue);
            } else if (isSigned) {
                return _irBuilder->CreateICmpSLT(leftValue, rightValue);
            } else {
                return _irBuilder->CreateICmpULT(leftValue, rightValue);
            }
        case InfixOperators::GreaterThanEqualTo: // >=
            if (isFloat) {
                return _irBuilder->CreateFCmpOGE(leftValue, rightValue);
            } else if (isSigned) {
                return _irBuilder->CreateICmpSGE(leftValue, rightValue);
            } else {
                return _irBuilder->CreateICmpUGE(leftValue, rightValue);
            }
        case InfixOperators::LessThanEqualTo: // <=
            if (isFloat) {
                return _irBuilder->CreateFCmpOLE(leftValue, rightValue);
            } else if (isSigned) {
                return _irBuilder->CreateICmpSLE(leftValue, rightValue);
            } else {
                return _irBuilder->CreateICmpULE(leftValue, rightValue);
            }
        default:
            printError("built in infix operator `" +
                       getInfixOperatorStringValue(infixOperator) +
                       "` does not exist for the provided types!",
                       startPosition, endPosition);
            return nullptr;
    }

    return nullptr;
}

llvm::Value* gulc::CodeGen::generateLocalVariableRefExpr(gulc::LocalVariableRefExpr const* localVariableRefExpr) {
    llvm::AllocaInst* localVariableAlloca = getLocalVariableOrNull(localVariableRefExpr->variableName());

    if (localVariableAlloca != nullptr) {
        return localVariableAlloca;
    } else {
        printError("[INTERNAL] local variable was not found!",
                   localVariableRefExpr->startPosition(), localVariableRefExpr->endPosition());
    }

    return nullptr;
}

llvm::Value* gulc::CodeGen::generateLValueToRValueExpr(gulc::LValueToRValueExpr const* lValueToRValueExpr) {
    llvm::Value* lvalue = generateExpr(lValueToRValueExpr->lvalue);
    return _irBuilder->CreateLoad(lvalue);
}

llvm::Value* gulc::CodeGen::generateMemberFunctionCallExpr(gulc::MemberFunctionCallExpr const* memberFunctionCallExpr) {
    printError("member function calling not yet supported!",
               memberFunctionCallExpr->startPosition(), memberFunctionCallExpr->endPosition());
    return nullptr;
}

llvm::Value* gulc::CodeGen::generateMemberPostfixOperatorCallExpr(
        gulc::MemberPostfixOperatorCallExpr const* memberPostfixOperatorCallExpr) {
    printError("member postfix operator calling not yet supported!",
               memberPostfixOperatorCallExpr->startPosition(), memberPostfixOperatorCallExpr->endPosition());
    return nullptr;
}

llvm::Value* gulc::CodeGen::generateMemberPrefixOperatorCallExpr(
        gulc::MemberPrefixOperatorCallExpr const* memberPrefixOperatorCallExpr) {
    printError("member prefix operator calling not yet supported!",
               memberPrefixOperatorCallExpr->startPosition(), memberPrefixOperatorCallExpr->endPosition());
    return nullptr;
}

llvm::Value* gulc::CodeGen::generateMemberVariableRefExpr(gulc::MemberVariableRefExpr const* memberVariableRefExpr) {
    llvm::Value* objectRef = generateExpr(memberVariableRefExpr->object);

    TypeCompareUtil typeCompareUtil;

    // If the types differ that means there is a compiler cast (one that can never be overridden)
    // We do this because of the way we lay out variables in a struct. Rather than re-layout the base structs
    // as part of the current struct we instead make the first variable of the current struct the unpadded version of
    // our base struct. Giving us all variables of the base struct laid out properly while still attempting to compact
    // the variables as much as possible (by filling in the unused padded area with our own variables when possible)
    if (!typeCompareUtil.compareAreSame(memberVariableRefExpr->object->valueType,
                                        memberVariableRefExpr->structType)) {
        objectRef = _irBuilder->CreateBitCast(objectRef,
                llvm::PointerType::getUnqual(generateLlvmType(memberVariableRefExpr->structType)));
    }

    std::vector<VariableDecl*>& memoryLayout = memberVariableRefExpr->structType->decl()->memoryLayout;
    unsigned int index = 0;
    bool elementFound = false;

    for (std::size_t i = 0; i < memoryLayout.size(); ++i) {
        // We check if the pointers are the same for equality...
        if (memoryLayout[i] == memberVariableRefExpr->variableDecl()) {
            index = i;
            elementFound = true;
            break;
        }
    }

    if (!elementFound) {
        printError("struct element '" + memberVariableRefExpr->variableDecl()->identifier().name() + "' was not found!",
                   memberVariableRefExpr->startPosition(), memberVariableRefExpr->endPosition());
    }

    // If the struct has a base type we increment it by one to account for the base class member
    if (memberVariableRefExpr->structType->decl()->baseStruct != nullptr) {
        index += 1;
    }

    // NOTE: Not exactly sure whats wrong here but we'll just let LLVM handle getting the type...
//    llvm::StructType* structType = getLlvmStructType(refStructMemberVariableExpr->structType->decl());
//    llvm::PointerType* structPointerType = llvm::PointerType::getUnqual(structType);
    return _irBuilder->CreateStructGEP(nullptr, objectRef, index);
}

llvm::Value* gulc::CodeGen::generateParameterRefExpr(gulc::ParameterRefExpr const* parameterRefExpr) {
    // If the function is a member function (i.e. has `self` argument) we need to add one to the param
    // index.
    std::size_t paramMod = _currentGhoulFunction->isMemberFunction() ? 1 : 0;
    std::size_t paramIndex = parameterRefExpr->parameterIndex() + paramMod;

    if (paramIndex < _currentLlvmFunctionParameters.size()) {
        return _currentLlvmFunctionParameters[paramIndex];
    }

    printError("[INTERNAL] parameter was not found!",
               parameterRefExpr->startPosition(), parameterRefExpr->endPosition());
    return nullptr;
}

llvm::Value* gulc::CodeGen::generateParenExpr(gulc::ParenExpr const* parenExpr) {
    return generateExpr(parenExpr->nestedExpr);
}

llvm::Value* gulc::CodeGen::generatePostfixOperatorExpr(gulc::PostfixOperatorExpr const* postfixOperatorExpr) {
    llvm::Value* refValue = generateExpr(postfixOperatorExpr->nestedExpr);
    llvm::Value* value = _irBuilder->CreateLoad(refValue);

    Type* valueType = postfixOperatorExpr->nestedExpr->valueType;

    bool isSigned = false;
    bool isFloat = false;
    std::size_t sizeInBytes = 0;

    if (llvm::isa<BuiltInType>(valueType)) {
        auto builtInType = llvm::dyn_cast<BuiltInType>(valueType);

        isFloat = builtInType->isFloating();
        isSigned = builtInType->isSigned();

        sizeInBytes = builtInType->sizeInBytes();
    } else if (llvm::isa<PointerType>(valueType)) {
        auto pointerType = llvm::dyn_cast<PointerType>(valueType);

        sizeInBytes = _target.sizeofPtr();
    } else {
        printError("unknown postfix operator expression!",
                   postfixOperatorExpr->startPosition(), postfixOperatorExpr->endPosition());
        return nullptr;
    }

    switch (postfixOperatorExpr->postfixOperator()) {
        case PostfixOperators::Increment:
            if (isFloat) {
                llvm::Value* newValue =
                        _irBuilder->CreateFAdd(
                                value,
                                llvm::ConstantFP::get(*_llvmContext, llvm::APFloat(1.0f))
                            );
                _irBuilder->CreateStore(newValue, refValue);
            } else {
                llvm::Value* newValue =
                        _irBuilder->CreateAdd(
                                value,
                                llvm::ConstantInt::get(*_llvmContext, llvm::APInt(sizeInBytes * 8, 1))
                            );
                _irBuilder->CreateStore(newValue, refValue);
            }

            break;
        case PostfixOperators::Decrement:
            if (isFloat) {
                llvm::Value* newValue =
                        _irBuilder->CreateFSub(
                                value,
                                llvm::ConstantFP::get(*_llvmContext, llvm::APFloat(1.0f))
                            );
                _irBuilder->CreateStore(newValue, refValue);
            } else {
                llvm::Value* newValue =
                        _irBuilder->CreateSub(
                                value,
                                llvm::ConstantInt::get(*_llvmContext, llvm::APInt(sizeInBytes * 8, 1)));
                _irBuilder->CreateStore(newValue, refValue);
            }

            break;
        default:
            printError("built in postfix operator `" +
                       getPostfixOperatorString(postfixOperatorExpr->postfixOperator()) +
                       "` does not exist for the provided type!",
                       postfixOperatorExpr->startPosition(), postfixOperatorExpr->endPosition());
            break;
    }

    return value;
}

llvm::Value* gulc::CodeGen::generatePrefixOperatorExpr(gulc::PrefixOperatorExpr const* prefixOperatorExpr) {
    llvm::Value* nestedValue = generateExpr(prefixOperatorExpr->nestedExpr);
    gulc::Type* checkType = prefixOperatorExpr->nestedExpr->valueType;

    PrefixOperators checkOperator = prefixOperatorExpr->prefixOperator();

    if (checkOperator == PrefixOperators::Increment || checkOperator == PrefixOperators::Decrement) {
        llvm::Value* derefValue = _irBuilder->CreateLoad(nestedValue);

        bool isSigned = false;
        bool isFloat = false;
        std::size_t sizeInBytes = 0;

        if (llvm::isa<BuiltInType>(checkType)) {
            auto builtInType = llvm::dyn_cast<BuiltInType>(checkType);

            isFloat = builtInType->isFloating();
            isSigned = builtInType->isSigned();

            sizeInBytes = builtInType->sizeInBytes();
        } else if (llvm::isa<PointerType>(checkType)) {
            auto pointerType = llvm::dyn_cast<PointerType>(checkType);

            sizeInBytes = _target.sizeofPtr();
        } else {
            printError("unknown postfix operator expression!",
                       prefixOperatorExpr->startPosition(), prefixOperatorExpr->endPosition());
            return nullptr;
        }

        if (checkOperator == PrefixOperators::Increment) {
            if (isFloat) {
                llvm::Value* newValue =
                        _irBuilder->CreateFAdd(
                                derefValue,
                                llvm::ConstantFP::get(*_llvmContext, llvm::APFloat(1.0f))
                            );
                _irBuilder->CreateStore(newValue, nestedValue);
            } else {
                llvm::Value* newValue =
                        _irBuilder->CreateAdd(
                                derefValue,
                                llvm::ConstantInt::get(*_llvmContext, llvm::APInt(sizeInBytes * 8, 1))
                            );
                _irBuilder->CreateStore(newValue, nestedValue);
            }
        } else if (checkOperator == PrefixOperators::Decrement) {
            if (isFloat) {
                llvm::Value* newValue =
                        _irBuilder->CreateFSub(
                                derefValue,
                                llvm::ConstantFP::get(*_llvmContext, llvm::APFloat(1.0f))
                            );
                _irBuilder->CreateStore(newValue, nestedValue);
            } else {
                llvm::Value* newValue =
                        _irBuilder->CreateSub(
                                derefValue,
                                llvm::ConstantInt::get(*_llvmContext, llvm::APInt(sizeInBytes * 8, 1))
                            );
                _irBuilder->CreateStore(newValue, nestedValue);
            }
        }

        return nestedValue;
    } else if (checkOperator == PrefixOperators::Dereference) {
        if (!llvm::isa<PointerType>(checkType)) {
            printError("`CodeGen::generatePrefixOperatorExpr` received non-pointer for dereference operator!",
                       prefixOperatorExpr->startPosition(), prefixOperatorExpr->endPosition());
        }

        return _irBuilder->CreateLoad(nestedValue);
    } else if (checkOperator == PrefixOperators::Reference) {
        // NOTE: For `reference` is is just a logical conversion for us to no longer implicitly dereference the
        //       reference. The only difference between a pointer and a reference is that pointers are managed manually
        // NOTE: All error checking for this should be performed before this step.
        return nestedValue;
    } else {
        bool isSigned = false;
        bool isFloat = false;

        if (llvm::isa<BuiltInType>(checkType)) {
            auto builtInType = llvm::dyn_cast<BuiltInType>(checkType);

            isFloat = builtInType->isFloating();
            isSigned = builtInType->isSigned();
        } else if (!llvm::isa<PointerType>(checkType)) {
            printError("unknown prefix operator expression!",
                       prefixOperatorExpr->startPosition(), prefixOperatorExpr->endPosition());
            return nullptr;
        }

        switch (prefixOperatorExpr->prefixOperator()) {
            case PrefixOperators::Positive:
                printError("prefix operator `+` does not have a built in operation, it must be overloaded to provide "
                           "functionality!",
                           prefixOperatorExpr->startPosition(), prefixOperatorExpr->endPosition());
                return nullptr;
            case PrefixOperators::Negative: {
                if (isFloat) {
                    return _irBuilder->CreateFNeg(nestedValue);
                } else {
                    return _irBuilder->CreateNeg(nestedValue);
                }
            }
            case PrefixOperators::LogicalNot: {
                // TODO: Is this correct? Do we need the compare or the `ZExt`?
                llvm::Value* inverted = _irBuilder->CreateNot(nestedValue);
                return _irBuilder->CreateZExt(inverted, inverted->getType());
//                return _irBuilder->CreateICmpEQ(
//                        inverted,
//                        llvm::ConstantInt::get(llvm::dyn_cast<llvm::IntegerType>(inverted->getType()), 0)
//                    );
            }
            case PrefixOperators::BitwiseNot:
                return _irBuilder->CreateNot(nestedValue);

            case PrefixOperators::SizeOf:
            case PrefixOperators::AlignOf:
            case PrefixOperators::OffsetOf:
            case PrefixOperators::NameOf:
            case PrefixOperators::TraitsOf:
                // NOTE: The above prefix operators should be entirely removed by now by the const solvers
                printError(
                        "[INTERNAL] `CodeGen::generatePrefixOperatorExpr` received unsolved `const` prefix operator `" +
                        getPrefixOperatorString(prefixOperatorExpr->prefixOperator()) +
                        "`!",
                        prefixOperatorExpr->startPosition(), prefixOperatorExpr->endPosition());
                return nullptr;
            default:
                printError("built in prefix operator `" +
                           getPrefixOperatorString(prefixOperatorExpr->prefixOperator()) +
                           "` does not exist for the provided type!",
                           prefixOperatorExpr->startPosition(), prefixOperatorExpr->endPosition());
                return nullptr;
        }
    }
    return nullptr;
}

llvm::Value* gulc::CodeGen::generateTernaryExpr(gulc::TernaryExpr const* ternaryExpr) {
    printError("ternary operator not yet supported!",
               ternaryExpr->startPosition(), ternaryExpr->endPosition());
    return nullptr;
}

llvm::Value* gulc::CodeGen::generateTryExpr(gulc::TryExpr const* tryExpr) {
    return generateExpr(tryExpr->nestedExpr);
}

llvm::Value* gulc::CodeGen::generateValueLiteralExpr(gulc::ValueLiteralExpr const* valueLiteralExpr) {
    if (valueLiteralExpr->hasSuffix()) {
        // TODO: Call suffix function
    } else {
        switch (valueLiteralExpr->literalType()) {
            case ValueLiteralExpr::LiteralType::Integer: {
                llvm::Type* constantType = nullptr;

                if (valueLiteralExpr->valueType != nullptr) {
                    constantType = generateLlvmType(valueLiteralExpr->valueType);
                } else {
                    // We default to `i32`
                    constantType = llvm::Type::getInt32Ty(*_llvmContext);
                }

                // TODO: We need to split the value literals out into their own declarations...
                return llvm::ConstantInt::get(llvm::dyn_cast<llvm::IntegerType>(constantType),
                                              valueLiteralExpr->value(), 10);
            }
            case ValueLiteralExpr::LiteralType::Float: {
                llvm::Type* constantType = nullptr;

                if (valueLiteralExpr->valueType != nullptr) {
                    constantType = generateLlvmType(valueLiteralExpr->valueType);
                } else {
                    // We default to `f32`
                    constantType = llvm::Type::getFloatTy(*_llvmContext);
                }

                return llvm::ConstantFP::get(constantType, valueLiteralExpr->value());
            }
            case ValueLiteralExpr::LiteralType::Char:
                printError("char literal not yet supported!",
                           valueLiteralExpr->startPosition(), valueLiteralExpr->endPosition());
                break;
            case ValueLiteralExpr::LiteralType::String:
                printError("string literal not yet supported!",
                           valueLiteralExpr->startPosition(), valueLiteralExpr->endPosition());
                break;
        }
    }

    return nullptr;
}

llvm::Value* gulc::CodeGen::generateVariableDeclExpr(gulc::VariableDeclExpr const* variableDeclExpr) {
    llvm::AllocaInst* newLocalVariable = addLocalVariable(variableDeclExpr->identifier().name(),
                                                          generateLlvmType(variableDeclExpr->type));

    // TODO: Are we going to do RAII? Or is `let x: i32` going to error saying it is unassigned?
    //       I think we should say it is unassigned. We do nothing unless you access `x`. If `x` hasn't been assigned
    //       at the first use then we error.
    //       To do something similar to RAII you would replace `let x: i32` with `let x = i32()`
    if (variableDeclExpr->initialValue != nullptr) {
        llvm::Value* initialValue = generateExpr(variableDeclExpr->initialValue);

        switch (variableDeclExpr->initialValueAssignmentType) {
            case InitialValueAssignmentType::Normal:
                // Return the assignment...
                return _irBuilder->CreateStore(initialValue, newLocalVariable, false);
            case InitialValueAssignmentType::Move: {
                auto moveConstructorFunc = getMoveConstructorForType(variableDeclExpr->type);
                std::vector<llvm::Value*> llvmArgs {
                    newLocalVariable,
                    initialValue
                };

                return _irBuilder->CreateCall(moveConstructorFunc, llvmArgs);
            }
            case InitialValueAssignmentType::Copy: {
                auto moveConstructorFunc = getCopyConstructorForType(variableDeclExpr->type);
                std::vector<llvm::Value*> llvmArgs {
                        newLocalVariable,
                        initialValue
                };

                return _irBuilder->CreateCall(moveConstructorFunc, llvmArgs);
            }
        }
    }

    return newLocalVariable;
}

llvm::Value* gulc::CodeGen::generateVariableRefExpr(gulc::VariableRefExpr const* variableRefExpr) {
    printError("referencing variables not yet supported!",
               variableRefExpr->startPosition(), variableRefExpr->endPosition());
    return nullptr;
}

llvm::Value* gulc::CodeGen::generateVTableFunctionReferenceExpr(
        gulc::VTableFunctionReferenceExpr const* vTableFunctionReferenceExpr) {
    printError("referencing vtable function not yet supported!",
               vTableFunctionReferenceExpr->startPosition(), vTableFunctionReferenceExpr->endPosition());
    return nullptr;
}

llvm::Value* gulc::CodeGen::dereferenceReference(llvm::Value* value) {
    return _irBuilder->CreateLoad(value);
}

void gulc::CodeGen::castValue(gulc::Type* to, gulc::Type* from, llvm::Value*& value,
                              gulc::TextPosition const& startPosition, gulc::TextPosition const& endPosition) {
    if (llvm::isa<BuiltInType>(from)) {
        auto fromBuiltIn = llvm::dyn_cast<BuiltInType>(from);

        if (llvm::isa<BuiltInType>(to)) {
            auto toBuiltIn = llvm::dyn_cast<BuiltInType>(to);

            if (fromBuiltIn->isFloating()) {
                if (toBuiltIn->isFloating()) {
                    if (toBuiltIn->sizeInBytes() > fromBuiltIn->sizeInBytes()) {
                        value = _irBuilder->CreateFPExt(value, generateLlvmType(to));
                        return;
                    } else {
                        value = _irBuilder->CreateFPTrunc(value, generateLlvmType(to));
                        return;
                    }
                } else if (toBuiltIn->isSigned()) {
                    value = _irBuilder->CreateFPToSI(value, generateLlvmType(to));
                    return;
                } else {
                    value = _irBuilder->CreateFPToUI(value, generateLlvmType(to));
                    return;
                }
            } else if (fromBuiltIn->isSigned()) {
                if (toBuiltIn->isFloating()) {
                    value = _irBuilder->CreateSIToFP(value, generateLlvmType(to));
                    return;
                } else {
                    // TODO: I'm not sure if the `isSigned` is meant for `value` or `destTy`? Assuming value...
                    value = _irBuilder->CreateIntCast(value, generateLlvmType(to), fromBuiltIn->isSigned());
                    return;
                }
            } else {
                if (toBuiltIn->isFloating()) {
                    value = _irBuilder->CreateUIToFP(value, generateLlvmType(to));
                    return;
                } else {
                    // TODO: I'm not sure if the `isSigned` is meant for `value` or `destTy`? Assuming value...
                    value = _irBuilder->CreateIntCast(value, generateLlvmType(to), fromBuiltIn->isSigned());
                    return;
                }
            }
        } else if (llvm::isa<PointerType>(to)) {
            if (fromBuiltIn->isFloating()) {
                printError("[INTERNAL] casting from a float to a pointer is NOT supported!",
                           startPosition, endPosition);
                return;
            }

            value = _irBuilder->CreateIntToPtr(value, generateLlvmType(to));
            return;
        }
    } else if (llvm::isa<ReferenceType>(from)) {
        if (llvm::isa<ReferenceType>(to)) {
            value = _irBuilder->CreateBitCast(value, generateLlvmType(to));
            return;
        } else {
            printError("[INTERNAL] reference '" + from->toString() + "' to non-reference '" + to->toString() + "' cast found in code gen!",
                       startPosition, endPosition);
        }
    } else if (llvm::isa<PointerType>(from)) {
        if (llvm::isa<PointerType>(to)) {
            value = _irBuilder->CreateBitCast(value, generateLlvmType(to));
            return;
        } else if (llvm::isa<BuiltInType>(to)) {
            auto toBuiltIn = llvm::dyn_cast<BuiltInType>(to);

            if (toBuiltIn->isFloating()) {
                printError("[INTERNAL] cannot convert from pointer to `float`!",
                           startPosition, endPosition);
            }

            value = _irBuilder->CreatePtrToInt(value, generateLlvmType(to));
            return;
        }
    }

    printError("casting to type `" + to->toString() + "` from type `" + from->toString() + "` is not supported!",
               startPosition, endPosition);
}

llvm::AllocaInst* gulc::CodeGen::addLocalVariable(std::string const& varName, llvm::Type* llvmType) {
    llvm::AllocaInst* allocaInst = _irBuilder->CreateAlloca(llvmType, nullptr, varName);

    _currentLlvmFunctionLocalVariables.push_back(allocaInst);

    return allocaInst;
}

llvm::AllocaInst* gulc::CodeGen::getLocalVariableOrNull(std::string const& varName) {
    // NOTE: I'm looping in reverse for two reasons:
    //        1. I'm considering making local variables shadowable so you can redeclare immutable local variables
    //           instead of overwriting them
    //        2. In my experience you're more likely to be working on variables you've recently declared, this could
    //           lead to a negligible speed improvement
    for (auto checkVariable : gulc::reverse(_currentLlvmFunctionLocalVariables)) {
        if (checkVariable->getName() == varName) {
            return checkVariable;
        }
    }

    return nullptr;
}

llvm::Function* gulc::CodeGen::getMoveConstructorForType(gulc::Type* type) {
    if (!llvm::isa<StructType>(type)) {
        printError("[INTERNAL] `CodeGen::getMoveConstructorForType` received non-struct type!",
                   type->startPosition(), type->endPosition());
    }

    auto structType = llvm::dyn_cast<StructType>(type);
    auto structDecl = structType->decl();

    if (structDecl->cachedMoveConstructor == nullptr ||
            structDecl->cachedMoveConstructor->constructorState == ConstructorDecl::ConstructorState::Deleted) {
        printError("[INTERNAL] `CodeGen::getMoveConstructorForType` received struct with deleted move constructor!",
                   type->startPosition(), type->endPosition());
    }

    return _llvmModule->getFunction(structDecl->cachedMoveConstructor->mangledName());
}

llvm::Function* gulc::CodeGen::getCopyConstructorForType(gulc::Type* type) {
    if (!llvm::isa<StructType>(type)) {
        printError("[INTERNAL] `CodeGen::getCopyConstructorForType` received non-struct type!",
                   type->startPosition(), type->endPosition());
    }

    auto structType = llvm::dyn_cast<StructType>(type);
    auto structDecl = structType->decl();

    if (structDecl->cachedCopyConstructor == nullptr ||
            structDecl->cachedCopyConstructor->constructorState == ConstructorDecl::ConstructorState::Deleted) {
        printError("[INTERNAL] `CodeGen::getCopyConstructorForType` received struct with deleted copy constructor!",
                   type->startPosition(), type->endPosition());
    }

    return _llvmModule->getFunction(structDecl->cachedCopyConstructor->mangledName());
}
