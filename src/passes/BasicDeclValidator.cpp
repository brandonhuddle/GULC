#include <ast/types/NestedType.hpp>
#include <ast/types/TemplateStructType.hpp>
#include <ast/types/TemplateTypenameRefType.hpp>
#include <ast/exprs/TemplateConstRefExpr.hpp>
#include <ast/types/TemplateTraitType.hpp>
#include <ast/types/StructType.hpp>
#include "BasicDeclValidator.hpp"

void gulc::BasicDeclValidator::processFiles(std::vector<ASTFile>& files) {
    for (ASTFile& file : files) {
        _currentFile = &file;

        validateImports(file.imports);

        for (Decl* decl : file.declarations) {
            validateDecl(decl);
        }
    }
}

void gulc::BasicDeclValidator::printError(const std::string& message, gulc::TextPosition startPosition,
                                          gulc::TextPosition endPosition) const {
    std::cerr << "gulc error[" << _filePaths[_currentFile->sourceFileID] << ", "
                           "{" << startPosition.line << ", " << startPosition.column << " "
                           "to " << endPosition.line << ", " << endPosition.column << "}]: "
              << message << std::endl;
    std::exit(1);
}

void gulc::BasicDeclValidator::printWarning(const std::string& message, gulc::TextPosition startPosition,
                                            gulc::TextPosition endPosition) const {
    std::cerr << "gulc warning[" << _filePaths[_currentFile->sourceFileID] << ", "
                             "{" << startPosition.line << ", " << startPosition.column << " "
                             "to " << endPosition.line << ", " << endPosition.column << "}]: "
              << message << std::endl;
}

void gulc::BasicDeclValidator::validateImports(std::vector<ImportDecl*>& imports) const {
    for (ImportDecl* importDecl : imports) {
        bool importFound = false;

        std::string const& findName = importDecl->importPath()[0].name();
        NamespaceDecl* foundNamespace = nullptr;

        for (NamespaceDecl* checkNamespace : _namespacePrototypes) {
            if (resolveImport(importDecl->importPath(), 0, checkNamespace, &foundNamespace)) {
                importFound = true;
                break;
            }
        }

        if (!importFound) {
            printError("import path `" + importDecl->importPathToString() + "` was not found!",
                       importDecl->startPosition(), importDecl->endPosition());
        }

        importDecl->pointToNamespace = foundNamespace;

        if (importDecl->hasAlias()) {
            // Check other imports first for alias reuse, then check _currentFile->declarations
            for (ImportDecl* checkImport : imports) {
                if (checkImport == importDecl) continue;

                if (checkImport->hasAlias()) {
                    if (checkImport->importAlias().name() == importDecl->importAlias().name()) {
                        printError("import alias `" + importDecl->importAlias().name() + "` redefinition found!",
                                   checkImport->startPosition(), checkImport->endPosition());
                    }
                }
            }

            // Since only `_currentFile` will be set at this point we can use `getRedefinition` and it will handle
            // searching `_currentFile->declarations` for us
            Decl* redefinition = getRedefinition(importDecl->importAlias().name(), importDecl);

            if (redefinition != nullptr) {
                printError("import alias `" + importDecl->importAlias().name() + "` redefinition found!",
                           redefinition->startPosition(), redefinition->endPosition());
            }
        }
    }
}

bool gulc::BasicDeclValidator::resolveImport(const std::vector<Identifier>& importPath, std::size_t pathIndex,
                                             gulc::NamespaceDecl* checkNamespace,
                                             NamespaceDecl** foundNamespace) const {
    if (importPath[pathIndex].name() != checkNamespace->identifier().name()) {
        return false;
    } else if (pathIndex == importPath.size() - 1) {
        // We've reached the end, `checkNamespace` is the found namespace
        *foundNamespace = checkNamespace;
        return true;
    }

    for (Decl* checkDecl : checkNamespace->nestedDecls()) {
        if (llvm::isa<NamespaceDecl>(checkDecl)) {
            auto checkNestedNamespace = llvm::dyn_cast<NamespaceDecl>(checkDecl);

            // Check if this is the namespace we want. If so we return true until we exit `resolveImport`
            if (resolveImport(importPath, pathIndex + 1, checkNestedNamespace, foundNamespace)) {
                return true;
            }
        }
    }

    return false;
}

gulc::Decl* gulc::BasicDeclValidator::getRedefinition(std::string const& findName, gulc::Decl* skipDecl) const {
    // `_` is treated the same as it is in Swift.
    if (findName == "_") return nullptr;

    std::vector<Decl*>* searchDecls = nullptr;

    if (_currentContainerDecl == nullptr) {
        searchDecls = &_currentFile->declarations;
    } else {
        if (llvm::isa<NamespaceDecl>(_currentContainerDecl)) {
            // Namespaces are a special case where we have to grab their prototype for them to work properly
            // NOTE: We don't check if `prototype` is null because the prototype will be set by this point
            searchDecls = &llvm::dyn_cast<NamespaceDecl>(_currentContainerDecl)->prototype->nestedDecls();
        } else if (llvm::isa<StructDecl>(_currentContainerDecl)) {
            searchDecls = &llvm::dyn_cast<StructDecl>(_currentContainerDecl)->ownedMembers();
        } else if (llvm::isa<TraitDecl>(_currentContainerDecl)) {
            searchDecls = &llvm::dyn_cast<TraitDecl>(_currentContainerDecl)->ownedMembers();
        } else if (llvm::isa<TemplateStructDecl>(_currentContainerDecl)) {
            searchDecls = &llvm::dyn_cast<TemplateStructDecl>(_currentContainerDecl)->ownedMembers();
        } else if (llvm::isa<TemplateTraitDecl>(_currentContainerDecl)) {
            searchDecls = &llvm::dyn_cast<TemplateTraitDecl>(_currentContainerDecl)->ownedMembers();
        } else {
            printError("[INTERNAL] unknown containing decl found in `BasicDeclValidator::getRedefinition`!",
                       _currentContainerDecl->startPosition(), _currentContainerDecl->endPosition());
        }
    }

    for (Decl* checkDecl : *searchDecls) {
        if (checkDecl == skipDecl) continue;

        // We're unable to check for redefinitions on any `FunctionDecl` based nodes yet (due to not having types
        // resolved)
        if (llvm::isa<FunctionDecl>(checkDecl)) continue;
        // We're unable to handle `SubscriptOperatorDecl` since it too has parameters (which will need types resolved)
        if (llvm::isa<SubscriptOperatorDecl>(checkDecl)) continue;
        // We can't process any templates either. Template functions have already been skipped at this point
        if (llvm::isa<TemplateStructDecl>(checkDecl) || llvm::isa<TemplateTraitDecl>(checkDecl)) continue;
        // Finally, we also need to skip and template typealias declarations
        if (llvm::isa<TypeAliasDecl>(checkDecl)) {
            auto typeAlias = llvm::dyn_cast<TypeAliasDecl>(checkDecl);

            if (typeAlias->hasTemplateParameters()) continue;
        }

        if (checkDecl->identifier().name() == findName) return checkDecl;
    }

    return nullptr;
}

void gulc::BasicDeclValidator::validateParameters(const std::vector<ParameterDecl*>& parameters) const {
    bool mustBeOptional = false;

    // Check for duplicates and optionals (we can't validate anything else at this stage in compilation)
    for (ParameterDecl* checkParameter : parameters) {
        for (ParameterDecl* checkDuplicate : parameters) {
            if (checkDuplicate == checkParameter) {
                // Skip `checkParameter`
                continue;
            }

            if (checkDuplicate->identifier().name() == checkParameter->identifier().name()) {
                printError("redefinition of parameter `" + checkDuplicate->identifier().name() + "`!",
                           checkDuplicate->identifier().startPosition(), checkDuplicate->identifier().endPosition());
            }
        }

        // If there is an optional parameter then all parameters after the optional parameter must also be optional
        if (mustBeOptional) {
            if (checkParameter->defaultValue == nullptr) {
                printError("optional parameters must come after required parameters!",
                           checkParameter->startPosition(), checkParameter->endPosition());
            }
        } else {
            if (checkParameter->defaultValue != nullptr) {
                mustBeOptional = true;
            }
        }
    }
}

void gulc::BasicDeclValidator::validateTemplateParameters(const std::vector<TemplateParameterDecl*>& templateParameters) const {
    // Check for duplicates (we can't validate anything else at this stage in compilation)
    for (TemplateParameterDecl* checkTemplateParameter : templateParameters) {
        for (TemplateParameterDecl* checkTemplateDuplicate : templateParameters) {
            if (checkTemplateDuplicate == checkTemplateParameter) {
                // Skip `checkTemplateParameter`
                continue;
            }

            if (checkTemplateDuplicate->identifier().name() == checkTemplateParameter->identifier().name()) {
                printError("redefinition of template parameter `" + checkTemplateDuplicate->identifier().name() + "`!",
                           checkTemplateDuplicate->identifier().startPosition(),
                           checkTemplateDuplicate->identifier().endPosition());
            }
        }

        // Check to make sure the template parameter doesn't shadow a container template parameter
        // (we don't allow template parameter shadowing)
        for (std::vector<TemplateParameterDecl*>* checkContainerTemplateParameters : _templateParameters) {
            for (TemplateParameterDecl* checkTemplateShadow : *checkContainerTemplateParameters) {
                if (checkTemplateShadow->identifier().name() == checkTemplateParameter->identifier().name()) {
                    printError("template parameter `" + checkTemplateParameter->identifier().name() + "` shadows a "
                               "container template parameter!",
                               checkTemplateParameter->startPosition(), checkTemplateParameter->endPosition());
                }
            }
        }
    }
}

void gulc::BasicDeclValidator::validateDecl(gulc::Decl* decl, bool isGlobal) {
    decl->container = _currentContainerDecl;
    decl->containedInTemplate = !_templateParameters.empty();

    switch (decl->getDeclKind()) {
        case Decl::Kind::CallOperator:
            validateCallOperatorDecl(llvm::dyn_cast<CallOperatorDecl>(decl));
            break;
        case Decl::Kind::Constructor:
            validateConstructorDecl(llvm::dyn_cast<ConstructorDecl>(decl));
            break;
        case Decl::Kind::Destructor:
            validateDestructorDecl(llvm::dyn_cast<DestructorDecl>(decl));
            break;
        case Decl::Kind::Enum:
            validateEnumDecl(llvm::dyn_cast<EnumDecl>(decl));
            break;
        case Decl::Kind::Extension:
            validateExtensionDecl(llvm::dyn_cast<ExtensionDecl>(decl));
            break;
        case Decl::Kind::Function:
            validateFunctionDecl(llvm::dyn_cast<FunctionDecl>(decl));
            break;
        case Decl::Kind::Import:
            // Imports have already been validated.
            break;
        case Decl::Kind::Namespace:
            validateNamespaceDecl(llvm::dyn_cast<NamespaceDecl>(decl));
            break;
        case Decl::Kind::Operator:
            validateOperatorDecl(llvm::dyn_cast<OperatorDecl>(decl));
            break;
        case Decl::Kind::Parameter:
            // There isn't anything we can do for single parameters here.
            break;
        case Decl::Kind::Property:
            validatePropertyDecl(llvm::dyn_cast<PropertyDecl>(decl), isGlobal);
            break;
        case Decl::Kind::PropertyGet:
            validatePropertyGetDecl(llvm::dyn_cast<PropertyGetDecl>(decl));
            break;
        case Decl::Kind::PropertySet:
            validatePropertySetDecl(llvm::dyn_cast<PropertySetDecl>(decl));
            break;
        case Decl::Kind::Struct:
            validateStructDecl(llvm::dyn_cast<StructDecl>(decl), true, true);
            break;
        case Decl::Kind::SubscriptOperator:
            validateSubscriptOperatorDecl(llvm::dyn_cast<SubscriptOperatorDecl>(decl), isGlobal);
            break;
        case Decl::Kind::SubscriptOperatorGet:
            validateSubscriptOperatorGetDecl(llvm::dyn_cast<SubscriptOperatorGetDecl>(decl));
            break;
        case Decl::Kind::SubscriptOperatorSet:
            validateSubscriptOperatorSetDecl(llvm::dyn_cast<SubscriptOperatorSetDecl>(decl));
            break;
        case Decl::Kind::TemplateFunction:
            validateTemplateFunctionDecl(llvm::dyn_cast<TemplateFunctionDecl>(decl));
            break;
        case Decl::Kind::TemplateParameter:
            // There isn't anything we can do for single template parameters here.
            break;
        case Decl::Kind::TemplateStruct:
            validateTemplateStructDecl(llvm::dyn_cast<TemplateStructDecl>(decl));
            break;
        case Decl::Kind::TemplateStructInst:
            // Template struct instantiations shouldn't exist yet here.
            break;
        case Decl::Kind::TemplateTrait:
            validateTemplateTraitDecl(llvm::dyn_cast<TemplateTraitDecl>(decl));
            break;
        case Decl::Kind::TemplateTraitInst:
            // Template trait instantiations shouldn't exist yet here.
            break;
        case Decl::Kind::Trait:
            validateTraitDecl(llvm::dyn_cast<TraitDecl>(decl), true, true);
            break;
        case Decl::Kind::TypeAlias:
            validateTypeAliasDecl(llvm::dyn_cast<TypeAliasDecl>(decl));
            break;
        case Decl::Kind::TypeSuffix:
            validateTypeSuffixDecl(llvm::dyn_cast<TypeSuffixDecl>(decl), isGlobal);
            break;
        case Decl::Kind::Variable:
            validateVariableDecl(llvm::dyn_cast<VariableDecl>(decl), isGlobal);
            break;
        default:
            printError("[INTERNAL ERROR] unsupported Decl found in `BasicDeclValidator::processDecl`!",
                       decl->startPosition(), decl->endPosition());
            break;
    }
}

void gulc::BasicDeclValidator::validateCallOperatorDecl(gulc::CallOperatorDecl* callOperatorDecl) const {
    // `call` cannot be `static` because the syntax would be too ambiguous when paired with the constructor syntax
    // (i.e. is `ExampleType()` a `call` or an `init`?)
    if (callOperatorDecl->isStatic()) {
        printError("`call` cannot be marked `static`!",
                   callOperatorDecl->startPosition(), callOperatorDecl->endPosition());
    }

    validateFunctionDecl(callOperatorDecl);
}

void gulc::BasicDeclValidator::validateConstructorDecl(gulc::ConstructorDecl* constructorDecl) const {
    if (constructorDecl->isAnyVirtual()) {
        printError("`init` cannot be marked `virtual`, `abstract`, or `override`!",
                   constructorDecl->startPosition(), constructorDecl->endPosition());
    }

    if (constructorDecl->isStatic()) {
        printError("`init` cannot be marked `static`!",
                   constructorDecl->startPosition(), constructorDecl->endPosition());
    }

    if (constructorDecl->isMutable()) {
        printError("`init` cannot be marked `mut`, `init` is implicitly `mut` by default!",
                   constructorDecl->startPosition(), constructorDecl->endPosition());
    }

    validateFunctionDecl(constructorDecl);
}

void gulc::BasicDeclValidator::validateDestructorDecl(gulc::DestructorDecl* destructorDecl) const {
    if (destructorDecl->isMutable()) {
        printError("`deinit` cannot be marked `mut`, `deinit` is implicitly `mut` by default!",
                   destructorDecl->startPosition(), destructorDecl->endPosition());
    }

    if (destructorDecl->isStatic()) {
        printError("`deinit` cannot be marked `static`!",
                   destructorDecl->startPosition(), destructorDecl->endPosition());
    }

    validateFunctionDecl(destructorDecl);
}

void gulc::BasicDeclValidator::validateEnumDecl(gulc::EnumDecl* enumDecl) const {
    if (enumDecl->isConstExpr()) {
        printError("`enum` cannot be marked `const`, enums are `const` by default!",
                   enumDecl->startPosition(), enumDecl->endPosition());
    }

    if (enumDecl->isStatic()) {
        printError("enums cannot be marked `static`!",
                   enumDecl->startPosition(), enumDecl->endPosition());
    }

    if (enumDecl->isMutable()) {
        printError("enums cannot be marked `mut`!",
                   enumDecl->startPosition(), enumDecl->endPosition());
    }

    if (enumDecl->isAnyVirtual()) {
        printError("enums cannot be marked `virtual`, `abstract`, or `override`!",
                   enumDecl->startPosition(), enumDecl->endPosition());
    }

    if (enumDecl->isVolatile()) {
        printError("enums cannot be marked `override`!",
                   enumDecl->startPosition(), enumDecl->endPosition());
    }

    if (enumDecl->isExtern()) {
        printError("enums cannot be marked `extern`!",
                   enumDecl->startPosition(), enumDecl->endPosition());
    }

    enumDecl->containerTemplateType = _currentContainerTemplateType != nullptr ?
                                      _currentContainerTemplateType->deepCopy() : nullptr;

    for (EnumConstDecl* enumConst : enumDecl->enumConsts()) {
        enumConst->container = enumDecl;
        enumConst->containedInTemplate = enumDecl->containedInTemplate;

        for (EnumConstDecl* checkDuplicate : enumDecl->enumConsts()) {
            if (checkDuplicate == enumConst) continue;

            if (enumDecl->identifier().name() == checkDuplicate->identifier().name()) {
                printError("enum `" + enumDecl->identifier().name() + "` contains multiple definitions of "
                           "const `" + checkDuplicate->identifier().name() + "`!",
                           checkDuplicate->startPosition(), checkDuplicate->endPosition());
            }
        }
    }

    Decl* redefinition = getRedefinition(enumDecl->identifier().name(), enumDecl);

    if (redefinition != nullptr) {
        printError("redefinition of symbol `" + enumDecl->identifier().name() + "` detected!",
                   redefinition->startPosition(), redefinition->endPosition());
    }
}

void gulc::BasicDeclValidator::validateExtensionDecl(gulc::ExtensionDecl* extensionDecl) {
    if (extensionDecl->isConstExpr()) {
        printError("extensions cannot be marked `const`!",
                   extensionDecl->startPosition(), extensionDecl->endPosition());
    }

    if (extensionDecl->isStatic()) {
        printError("extensions cannot be marked `static`!",
                   extensionDecl->startPosition(), extensionDecl->endPosition());
    }

    if (extensionDecl->isMutable()) {
        printError("extensions cannot be marked `mut`!",
                   extensionDecl->startPosition(), extensionDecl->endPosition());
    }

    if (extensionDecl->isAnyVirtual()) {
        printError("extensions cannot be marked `virtual`, `abstract`, or `override`!",
                   extensionDecl->startPosition(), extensionDecl->endPosition());
    }

    validateTemplateParameters(extensionDecl->templateParameters());

    extensionDecl->containerTemplateType = _currentContainerTemplateType != nullptr ?
                                           _currentContainerTemplateType->deepCopy() : nullptr;

    Decl* oldContainer = _currentContainerDecl;
    _currentContainerDecl = extensionDecl;

    _templateParameters.push_back(&extensionDecl->templateParameters());

    for (Decl* checkDecl : extensionDecl->ownedMembers()) {
        if (llvm::isa<NamespaceDecl>(checkDecl)) {
            printError("`namespace` cannot be contained within extensions!",
                       checkDecl->startPosition(), checkDecl->endPosition());
        }

        if (llvm::isa<ImportDecl>(checkDecl)) {
            printError("`import` cannot be contained within extensions!",
                       checkDecl->startPosition(), checkDecl->endPosition());
        }

        if (llvm::isa<VariableDecl>(checkDecl)) {
            if (!checkDecl->isConstExpr() && !checkDecl->isStatic()) {
                printError("extensions cannot contain data members unless they are `const` or `static`!",
                           checkDecl->startPosition(), checkDecl->endPosition());
            }
        }

        validateDecl(checkDecl, false);

        // TODO: Can extensions contain `override`? I think they should be able to but I'm not entirely decided.
        if (checkDecl->isAbstract()) {
            printError("extensions cannot contain `abstract` members!",
                       checkDecl->startPosition(), checkDecl->endPosition());
        }

        if (checkDecl->isVirtual()) {
            printError("extensions cannot contain `virtual` members!",
                       checkDecl->startPosition(), checkDecl->endPosition());
        }
    }

    _templateParameters.pop_back();

    _currentContainerDecl = oldContainer;
}

void gulc::BasicDeclValidator::validateFunctionDecl(gulc::FunctionDecl* functionDecl) const {
    validateParameters(functionDecl->parameters());

    if (functionDecl->isConstExpr()) {
        printError("[INTERNAL] `const` is not yet supported!",
                   functionDecl->startPosition(), functionDecl->endPosition());
    }

    if (functionDecl->isExtern() && !functionDecl->isPrototype()) {
        printError("`extern` functions cannot have a provided body!",
                   functionDecl->startPosition(), functionDecl->endPosition());
    }

    if (functionDecl->isAbstract() && !functionDecl->isPrototype()) {
        printError("`abstract` functions cannot have a provided body!",
                   functionDecl->startPosition(), functionDecl->endPosition());
    }
}

void gulc::BasicDeclValidator::validateNamespaceDecl(gulc::NamespaceDecl* namespaceDecl) {
    Decl* oldContainer = _currentContainerDecl;
    _currentContainerDecl = namespaceDecl;

    for (Decl* nestedDecl : namespaceDecl->nestedDecls()) {
        if (llvm::isa<ImportDecl>(nestedDecl)) {
            printError("`import` cannot be contained within a `namespace`!",
                       nestedDecl->startPosition(), nestedDecl->endPosition());
        }

        validateDecl(nestedDecl, true);
    }

    _currentContainerDecl = oldContainer;
}

void gulc::BasicDeclValidator::validateOperatorDecl(gulc::OperatorDecl* operatorDecl) const {
    if (operatorDecl->isStatic()) {
        printError("`operator` cannot be marked `static`!",
                   operatorDecl->startPosition(), operatorDecl->endPosition());
    }

    validateFunctionDecl(operatorDecl);
}

void gulc::BasicDeclValidator::validatePropertyDecl(gulc::PropertyDecl* propertyDecl, bool isGlobal) {
    if (isGlobal) {
        printError("properties cannot appear outside of a trait, union, struct, or class!",
                   propertyDecl->startPosition(), propertyDecl->endPosition());
    }

    for (PropertyGetDecl* getter : propertyDecl->getters()) {
        getter->container = propertyDecl;
        getter->containedInTemplate = propertyDecl->containedInTemplate;

        validatePropertyGetDecl(getter);

        if (propertyDecl->isAbstract() && !getter->isPrototype()) {
            printError("abstract properties cannot contain `get` declarations with a body!",
                       propertyDecl->startPosition(), propertyDecl->endPosition());
        }

        if (propertyDecl->isExtern() && !getter->isPrototype()) {
            printError("extern properties cannot contain `get` declarations with a body!",
                       propertyDecl->startPosition(), propertyDecl->endPosition());
        }
    }

    if (propertyDecl->hasSetter()) {
        propertyDecl->setter()->container = propertyDecl;
        propertyDecl->setter()->containedInTemplate = propertyDecl->containedInTemplate;

        validatePropertySetDecl(propertyDecl->setter());

        if (propertyDecl->isAbstract() && !propertyDecl->setter()->isPrototype()) {
            printError("abstract properties cannot contain a `set` declaration with a body!",
                       propertyDecl->startPosition(), propertyDecl->endPosition());
        }

        if (propertyDecl->isExtern() && !propertyDecl->setter()->isPrototype()) {
            printError("extern properties cannot contain a `set` declaration with a body!",
                       propertyDecl->startPosition(), propertyDecl->endPosition());
        }
    }

    for (PropertyGetDecl* checkGetter : propertyDecl->getters()) {
        for (PropertyGetDecl* checkDuplicate : propertyDecl->getters()) {
            if (checkGetter->getResultType() == checkDuplicate->getResultType() &&
                    checkGetter->isMutable() == checkDuplicate->isMutable()) {
                printError("duplicate `get` definition found for property `" + propertyDecl->identifier().name() + "`!",
                           checkDuplicate->startPosition(), checkDuplicate->endPosition());
            }
        }
    }

    if (propertyDecl->isConstExpr()) {
        printError("[INTERNAL] `const` is not yet supported!",
                   propertyDecl->startPosition(), propertyDecl->endPosition());
    }
}

void gulc::BasicDeclValidator::validatePropertyGetDecl(gulc::PropertyGetDecl* propertyGetDecl) {
    if (propertyGetDecl->isStatic()) {
        printError("property `get` cannot be marked `static`!",
                   propertyGetDecl->startPosition(), propertyGetDecl->endPosition());
    }

    if (propertyGetDecl->isAnyVirtual()) {
        printError("property `get` cannot be marked `virtual`, `abstract`, or `override`; "
                   "mark the `property` as `virtual`, `abstract`, or `override` instead!",
                   propertyGetDecl->startPosition(), propertyGetDecl->endPosition());
    }

    if (propertyGetDecl->isConstExpr()) {
        printError("[INTERNAL] `const` is not yet supported!",
                   propertyGetDecl->startPosition(), propertyGetDecl->endPosition());
    }
}

void gulc::BasicDeclValidator::validatePropertySetDecl(gulc::PropertySetDecl* propertySetDecl) {
    if (propertySetDecl->isStatic()) {
        printError("property `set` cannot be marked `static`!",
                   propertySetDecl->startPosition(), propertySetDecl->endPosition());
    }

    if (propertySetDecl->isAnyVirtual()) {
        printError("property `set` cannot be marked `virtual`, `abstract`, or `override`; "
                   "mark the `property` as `virtual`, `abstract`, or `override` instead!",
                   propertySetDecl->startPosition(), propertySetDecl->endPosition());
    }

    if (propertySetDecl->isConstExpr()) {
        printError("[INTERNAL] `const` is not yet supported!",
                   propertySetDecl->startPosition(), propertySetDecl->endPosition());
    }
}

void gulc::BasicDeclValidator::validateStructDecl(gulc::StructDecl* structDecl, bool checkForRedefinition,
                                                  bool setTemplateContainer) {
    if (structDecl->isMutable()) {
        printError("`" + structDecl->structKindName() + " " + structDecl->identifier().name() + "` cannot be marked `mut`!",
                   structDecl->startPosition(), structDecl->endPosition());
    }

    if (structDecl->isOverride()) {
        printError("`" + structDecl->structKindName() + " " + structDecl->identifier().name() + "` cannot be marked `override`!",
                   structDecl->startPosition(), structDecl->endPosition());
    }

    if (structDecl->isVirtual()) {
        printError("`" + structDecl->structKindName() + " " + structDecl->identifier().name() + "` cannot be marked `virtual`!",
                   structDecl->startPosition(), structDecl->endPosition());
    }

    if (structDecl->isExtern()) {
        printError("`" + structDecl->structKindName() + " " + structDecl->identifier().name() + "` cannot be marked `extern`!",
                   structDecl->startPosition(), structDecl->endPosition());
    }

    if (structDecl->isVolatile()) {
        printError("`" + structDecl->structKindName() + " " + structDecl->identifier().name() + "` cannot be marked `volatile`!",
                   structDecl->startPosition(), structDecl->endPosition());
    }

    if (structDecl->structKind() == StructDecl::Kind::Union) {
        // Unions cannot be `static` or `abstract`
        if (structDecl->isStatic()) {
            printError("unions cannot be marked `static`!",
                       structDecl->startPosition(), structDecl->endPosition());
        }

        if (structDecl->isAbstract()) {
            printError("unions cannot be marked `abstract`!",
                       structDecl->startPosition(), structDecl->endPosition());
        }
    }

    if (structDecl->isStatic()) {
        // Static structs CANNOT have constructors or destructors
        if (!structDecl->constructors().empty()) {
            printError("`static " + structDecl->structKindName() + "` cannot have constructors!",
                       structDecl->startPosition(), structDecl->endPosition());
        }

        if (structDecl->destructor != nullptr) {
            printError("`static " + structDecl->structKindName() + "` cannot have a destructor!",
                       structDecl->startPosition(), structDecl->endPosition());
        }
    }

    if (structDecl->isConstExpr()) {
        printError("[INTERNAL] `const` is not yet supported!",
                   structDecl->startPosition(), structDecl->endPosition());
    }

    structDecl->containerTemplateType = _currentContainerTemplateType != nullptr ?
                                        _currentContainerTemplateType->deepCopy() : nullptr;

    Type* oldContainerTemplateType = _currentContainerTemplateType;

    if (setTemplateContainer && _currentContainerTemplateType != nullptr) {
        _currentContainerTemplateType = new NestedType(Type::Qualifier::Unassigned,
                                                       _currentContainerTemplateType, structDecl->identifier(),
                                                       {}, {}, {});
    }

    Decl* oldContainer = _currentContainerDecl;
    _currentContainerDecl = structDecl;

    for (Decl* checkDecl : structDecl->ownedMembers()) {
        if (llvm::isa<NamespaceDecl>(checkDecl)) {
            printError("`namespace` cannot be contained within `" + structDecl->structKindName() + "`!",
                       checkDecl->startPosition(), checkDecl->endPosition());
        }

        if (llvm::isa<ImportDecl>(checkDecl)) {
            printError("`import` cannot be contained within `" + structDecl->structKindName() + "`!",
                       checkDecl->startPosition(), checkDecl->endPosition());
        }

        validateDecl(checkDecl, false);

        if (structDecl->isStatic()) {
            if (llvm::isa<FunctionDecl>(checkDecl) || llvm::isa<PropertyDecl>(checkDecl) ||
                llvm::isa<SubscriptOperatorDecl>(checkDecl) || llvm::isa<VariableDecl>(checkDecl)) {
                if (!checkDecl->isStatic()) {
                    printError("`static " + structDecl->structKindName() + "` can only contain static members!",
                               checkDecl->startPosition(), checkDecl->endPosition());
                }
            } else if (llvm::isa<CallOperatorDecl>(checkDecl)) {
                printError("`static " + structDecl->structKindName() + "` cannot contain a `call` definition!",
                           checkDecl->startPosition(), checkDecl->endPosition());
            }
        }
    }

    _currentContainerDecl = oldContainer;
    _currentContainerTemplateType = oldContainerTemplateType;

    if (checkForRedefinition) {
        Decl* redefinition = getRedefinition(structDecl->identifier().name(), structDecl);

        if (redefinition != nullptr) {
            printError("redefinition of symbol `" + structDecl->identifier().name() + "` detected!",
                       redefinition->startPosition(), redefinition->endPosition());
        }
    }
}

void gulc::BasicDeclValidator::validateSubscriptOperatorDecl(gulc::SubscriptOperatorDecl* subscriptOperatorDecl,
                                                             bool isGlobal) {
    if (isGlobal) {
        printError("properties cannot appear outside of a trait, union, struct, or class!",
                   subscriptOperatorDecl->startPosition(), subscriptOperatorDecl->endPosition());
    }

    validateParameters(subscriptOperatorDecl->parameters());

    for (SubscriptOperatorGetDecl* getter : subscriptOperatorDecl->getters()) {
        getter->container = subscriptOperatorDecl;
        getter->containedInTemplate = subscriptOperatorDecl->containedInTemplate;

        validateSubscriptOperatorGetDecl(getter);

        if (subscriptOperatorDecl->isAbstract() && !getter->isPrototype()) {
            printError("abstract subscripts cannot contain `get` declarations with a body!",
                       subscriptOperatorDecl->startPosition(), subscriptOperatorDecl->endPosition());
        }

        if (subscriptOperatorDecl->isExtern() && !getter->isPrototype()) {
            printError("extern subscripts cannot contain `get` declarations with a body!",
                       subscriptOperatorDecl->startPosition(), subscriptOperatorDecl->endPosition());
        }
    }

    if (subscriptOperatorDecl->hasSetter()) {
        subscriptOperatorDecl->setter()->container = subscriptOperatorDecl;
        subscriptOperatorDecl->setter()->containedInTemplate = subscriptOperatorDecl->containedInTemplate;

        validateSubscriptOperatorSetDecl(subscriptOperatorDecl->setter());

        if (subscriptOperatorDecl->isAbstract() && !subscriptOperatorDecl->setter()->isPrototype()) {
            printError("abstract subscripts cannot contain a `set` declaration with a body!",
                       subscriptOperatorDecl->startPosition(), subscriptOperatorDecl->endPosition());
        }

        if (subscriptOperatorDecl->isExtern() && !subscriptOperatorDecl->setter()->isPrototype()) {
            printError("extern subscripts cannot contain a `set` declaration with a body!",
                       subscriptOperatorDecl->startPosition(), subscriptOperatorDecl->endPosition());
        }
    }

    for (SubscriptOperatorGetDecl* checkGetter : subscriptOperatorDecl->getters()) {
        for (SubscriptOperatorGetDecl* checkDuplicate : subscriptOperatorDecl->getters()) {
            if (checkGetter->getResultType() == checkDuplicate->getResultType() &&
                checkGetter->isMutable() == checkDuplicate->isMutable()) {
                printError("duplicate `get` definition found for subscript!",
                           checkDuplicate->startPosition(), checkDuplicate->endPosition());
            }
        }
    }

    if (subscriptOperatorDecl->isConstExpr()) {
        printError("[INTERNAL] `const` is not yet supported!",
                   subscriptOperatorDecl->startPosition(), subscriptOperatorDecl->endPosition());
    }
}

void gulc::BasicDeclValidator::validateSubscriptOperatorGetDecl(gulc::SubscriptOperatorGetDecl* subscriptOperatorGetDecl) {
    if (subscriptOperatorGetDecl->isStatic()) {
        printError("subscript `get` cannot be marked `static`!",
                   subscriptOperatorGetDecl->startPosition(), subscriptOperatorGetDecl->endPosition());
    }

    if (subscriptOperatorGetDecl->isAnyVirtual()) {
        printError("subscript `get` cannot be marked `virtual`, `abstract`, or `override`; "
                   "mark the `subscript` as `virtual`, `abstract`, or `override` instead!",
                   subscriptOperatorGetDecl->startPosition(), subscriptOperatorGetDecl->endPosition());
    }

    if (subscriptOperatorGetDecl->isConstExpr()) {
        printError("[INTERNAL] `const` is not yet supported!",
                   subscriptOperatorGetDecl->startPosition(), subscriptOperatorGetDecl->endPosition());
    }
}

void gulc::BasicDeclValidator::validateSubscriptOperatorSetDecl(gulc::SubscriptOperatorSetDecl* subscriptOperatorSetDecl) {
    if (subscriptOperatorSetDecl->isStatic()) {
        printError("subscript `set` cannot be marked `static`!",
                   subscriptOperatorSetDecl->startPosition(), subscriptOperatorSetDecl->endPosition());
    }

    if (subscriptOperatorSetDecl->isAnyVirtual()) {
        printError("subscript `set` cannot be marked `virtual`, `abstract`, or `override`; "
                   "mark the `subscript` as `virtual`, `abstract`, or `override` instead!",
                   subscriptOperatorSetDecl->startPosition(), subscriptOperatorSetDecl->endPosition());
    }

    if (subscriptOperatorSetDecl->isConstExpr()) {
        printError("[INTERNAL] `const` is not yet supported!",
                   subscriptOperatorSetDecl->startPosition(), subscriptOperatorSetDecl->endPosition());
    }
}

void gulc::BasicDeclValidator::validateTemplateFunctionDecl(gulc::TemplateFunctionDecl* templateFunctionDecl) {
    validateTemplateParameters(templateFunctionDecl->templateParameters());

    _templateParameters.push_back(&templateFunctionDecl->templateParameters());

    validateFunctionDecl(templateFunctionDecl);

    _templateParameters.pop_back();
}

void gulc::BasicDeclValidator::validateTemplateStructDecl(gulc::TemplateStructDecl* templateStructDecl) {
    validateTemplateParameters(templateStructDecl->templateParameters());

    Type* oldContainerTemplateType = _currentContainerTemplateType;

    // Create the list of template arguments from the template parameters
    std::vector<Expr*> containerTemplateArguments;
    containerTemplateArguments.reserve(templateStructDecl->templateParameters().size());

    for (TemplateParameterDecl* templateParameter : templateStructDecl->templateParameters()) {
        if (templateParameter->templateParameterKind() == TemplateParameterDecl::TemplateParameterKind::Typename) {
            containerTemplateArguments.push_back(new TypeExpr(new TemplateTypenameRefType(Type::Qualifier::Unassigned,
                                                                                          templateParameter, {}, {})));
        } else {
            containerTemplateArguments.push_back(new TemplateConstRefExpr(templateParameter));
        }
    }

    if (_currentContainerTemplateType != nullptr) {
        _currentContainerTemplateType = new NestedType(Type::Qualifier::Unassigned,
                                                       _currentContainerTemplateType, templateStructDecl->identifier(),
                                                       containerTemplateArguments, {}, {});
    } else {
        _currentContainerTemplateType = new TemplateStructType(Type::Qualifier::Unassigned,
                                                               containerTemplateArguments,
                                                               templateStructDecl, {}, {});
    }

    _templateParameters.push_back(&templateStructDecl->templateParameters());

    validateStructDecl(templateStructDecl, false, false);

    // We set `containerTemplateType` AFTER `validateStructDecl` since `validateStructDecl` will set it to
    // `_currentContainerTemplateType` which we do NOT want.
    templateStructDecl->containerTemplateType = oldContainerTemplateType != nullptr ?
                                                oldContainerTemplateType->deepCopy() : nullptr;

    _templateParameters.pop_back();

    _currentContainerTemplateType = oldContainerTemplateType;
}

void gulc::BasicDeclValidator::validateTemplateTraitDecl(gulc::TemplateTraitDecl* templateTraitDecl) {
    validateTemplateParameters(templateTraitDecl->templateParameters());

    Type* oldContainerTemplateType = _currentContainerTemplateType;

    // Create the list of template arguments from the template parameters
    std::vector<Expr*> containerTemplateArguments;
    containerTemplateArguments.reserve(templateTraitDecl->templateParameters().size());

    for (TemplateParameterDecl* templateParameter : templateTraitDecl->templateParameters()) {
        if (templateParameter->templateParameterKind() == TemplateParameterDecl::TemplateParameterKind::Typename) {
            containerTemplateArguments.push_back(new TypeExpr(new TemplateTypenameRefType(Type::Qualifier::Unassigned,
                                                                                          templateParameter, {}, {})));
        } else {
            containerTemplateArguments.push_back(new TemplateConstRefExpr(templateParameter));
        }
    }

    if (_currentContainerTemplateType != nullptr) {
        _currentContainerTemplateType = new NestedType(Type::Qualifier::Unassigned,
                                                       _currentContainerTemplateType, templateTraitDecl->identifier(),
                                                       containerTemplateArguments, {}, {});
    } else {
        _currentContainerTemplateType = new TemplateTraitType(Type::Qualifier::Unassigned,
                                                               containerTemplateArguments,
                                                               templateTraitDecl, {}, {});
    }

    _templateParameters.push_back(&templateTraitDecl->templateParameters());

    validateTraitDecl(templateTraitDecl, false, false);

    // We set `containerTemplateType` AFTER `validateTraitDecl` since `validateTraitDecl` will set it to
    // `_currentContainerTemplateType` which we do NOT want.
    templateTraitDecl->containerTemplateType = oldContainerTemplateType != nullptr ?
                                               oldContainerTemplateType->deepCopy() : nullptr;

    _templateParameters.pop_back();

    _currentContainerTemplateType = oldContainerTemplateType;
}

void gulc::BasicDeclValidator::validateTraitDecl(gulc::TraitDecl* traitDecl, bool checkForRedefinition,
                                                 bool setTemplateContainer) {
    if (traitDecl->isAbstract()) {
        printError("traits cannot be marked `abstract`!",
                   traitDecl->startPosition(), traitDecl->endPosition());
    }

    if (traitDecl->isStatic()) {
        printError("traits cannot be marked `static`!",
                   traitDecl->startPosition(), traitDecl->endPosition());
    }

    if (traitDecl->isVirtual()) {
        printError("traits cannot be marked `virtual`!",
                   traitDecl->startPosition(), traitDecl->endPosition());
    }

    if (traitDecl->isOverride()) {
        printError("traits cannot be marked `override`!",
                   traitDecl->startPosition(), traitDecl->endPosition());
    }

    if (traitDecl->isMutable()) {
        printError("traits cannot be marked `mut`!",
                   traitDecl->startPosition(), traitDecl->endPosition());
    }

    if (traitDecl->isExtern()) {
        printError("traits cannot be marked `extern`!",
                   traitDecl->startPosition(), traitDecl->endPosition());
    }

    if (traitDecl->isVolatile()) {
        printError("traits cannot be marked `volatile`!",
                   traitDecl->startPosition(), traitDecl->endPosition());
    }

    if (traitDecl->isConstExpr()) {
        printError("[INTERNAL] `const` is not yet supported!",
                   traitDecl->startPosition(), traitDecl->endPosition());
    }

    traitDecl->containerTemplateType = _currentContainerTemplateType != nullptr ?
                                       _currentContainerTemplateType->deepCopy() : nullptr;

    Type* oldContainerTemplateType = _currentContainerTemplateType;

    if (setTemplateContainer && _currentContainerTemplateType != nullptr) {
        _currentContainerTemplateType = new NestedType(Type::Qualifier::Unassigned,
                                                       _currentContainerTemplateType, traitDecl->identifier(),
                                                       {}, {}, {});
    }

    Decl* oldContainer = _currentContainerDecl;
    _currentContainerDecl = traitDecl;

    for (Decl* checkDecl : traitDecl->ownedMembers()) {
        if (llvm::isa<NamespaceDecl>(checkDecl)) {
            printError("`namespace` cannot be contained within traits!",
                       checkDecl->startPosition(), checkDecl->endPosition());
        }

        if (llvm::isa<ImportDecl>(checkDecl)) {
            printError("`import` cannot be contained within traits!",
                       checkDecl->startPosition(), checkDecl->endPosition());
        }

        if (llvm::isa<VariableDecl>(checkDecl)) {
            auto checkVariable = llvm::dyn_cast<VariableDecl>(checkDecl);

            // Traits cannot contain data members
            if (!checkVariable->isConstExpr() && !checkVariable->isStatic()) {
                printError("traits cannot contain variables that are not marked `static` or `const`!",
                           checkVariable->startPosition(), checkVariable->endPosition());
            }
        }

        if (checkDecl->isAnyVirtual()) {
            printError("trait members cannot be marked `abstract`, `virtual`, or `override`!",
                       checkDecl->startPosition(), checkDecl->endPosition());
        }

        validateDecl(checkDecl, false);
    }

    _currentContainerDecl = oldContainer;
    _currentContainerTemplateType = oldContainerTemplateType;

    if (checkForRedefinition) {
        Decl* redefinition = getRedefinition(traitDecl->identifier().name(), traitDecl);

        if (redefinition != nullptr) {
            printError("redefinition of symbol `" + traitDecl->identifier().name() + "` detected!",
                       redefinition->startPosition(), redefinition->endPosition());
        }
    }
}

void gulc::BasicDeclValidator::validateTypeAliasDecl(gulc::TypeAliasDecl* typeAliasDecl) {
    typeAliasDecl->containerTemplateType = _currentContainerTemplateType != nullptr ?
                                           _currentContainerTemplateType->deepCopy() : nullptr;

    // The only thing we can validate here is if the type alias is redefined OR the template parameters are duplicates
    if (typeAliasDecl->hasTemplateParameters()) {
        validateTemplateParameters(typeAliasDecl->templateParameters());
    } else {
        Decl* redefinition = getRedefinition(typeAliasDecl->identifier().name(), typeAliasDecl);

        if (redefinition != nullptr) {
            printError("redefinition of symbol `" + typeAliasDecl->identifier().name() + "` detected!",
                       redefinition->startPosition(), redefinition->endPosition());
        }
    }
}

void gulc::BasicDeclValidator::validateTypeSuffixDecl(gulc::TypeSuffixDecl* typeSuffixDecl, bool isGlobal) {
    if (!isGlobal) {
        printError("`typesuffix` can only be declared in a global context! (they cannot appear within structs, traits, etc.)",
                   typeSuffixDecl->startPosition(), typeSuffixDecl->endPosition());
    }

    if (typeSuffixDecl->isAnyVirtual()) {
        printError("`typesuffix` cannot be marked `virtual`, `abstract`, or `override`!",
                   typeSuffixDecl->startPosition(), typeSuffixDecl->endPosition());
    }

    if (typeSuffixDecl->isMutable()) {
        printError("`typesuffix` cannot be marked `mut`!",
                   typeSuffixDecl->startPosition(), typeSuffixDecl->endPosition());
    }

    if (typeSuffixDecl->isStatic()) {
        printError("`typesuffix` cannot be marked `static`!",
                   typeSuffixDecl->startPosition(), typeSuffixDecl->endPosition());
    }

    validateFunctionDecl(typeSuffixDecl);

    switch (typeSuffixDecl->identifier().name()[0]) {
        case 'x':
            printWarning("typesuffixes starting with 'x' may cause issues with some numbers! "
                         "(e.g. `0xfce` will be interpreted as the hex value `FCE` instead of xfce)",
                         typeSuffixDecl->startPosition(), typeSuffixDecl->endPosition());
            break;
        case 'b':
            printWarning("typesuffixes starting with 'b' may cause issues with some numbers!",
                         typeSuffixDecl->startPosition(), typeSuffixDecl->endPosition());
            break;
        case 'o':
            printWarning("typesuffixes starting with 'o' may cause issues with some numbers!",
                         typeSuffixDecl->startPosition(), typeSuffixDecl->endPosition());
            break;
        default:
            break;
    }
}

void gulc::BasicDeclValidator::validateVariableDecl(gulc::VariableDecl* variableDecl, bool isGlobal) const {
    if (isGlobal) {
        if (!variableDecl->isConstExpr() && !variableDecl->isStatic()) {
            printError("global variables must be marked `const` or `static`!",
                       variableDecl->startPosition(), variableDecl->endPosition());
        }
    }

    if (variableDecl->type == nullptr) {
        printError("variables outside of function bodies and similar MUST have a type specified!",
                   variableDecl->startPosition(), variableDecl->endPosition());
    }
}
