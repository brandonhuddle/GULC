#include <iostream>
#include <utilities/TypeHelper.hpp>
#include <ast/types/BuiltInType.hpp>
#include <ast/types/StructType.hpp>
#include "ConstTypeResolver.hpp"

void gulc::ConstTypeResolver::processFiles(std::vector<ASTFile>& files) {
    for (ASTFile& file : files) {
        _currentFile = &file;

        for (Decl* decl : file.declarations) {
            processDecl(decl, true);
        }
    }
}

void gulc::ConstTypeResolver::printError(std::string const& message, gulc::TextPosition startPosition,
                                         gulc::TextPosition endPosition) const {
    std::cerr << "gulc const resolver error[" << _filePaths[_currentFile->sourceFileID] << ", "
                                          "{" << startPosition.line << ", " << startPosition.column << " "
                                          "to " << endPosition.line << ", " << endPosition.column << "}]: "
              << message << std::endl;
    std::exit(1);
}

void gulc::ConstTypeResolver::printWarning(std::string const& message, gulc::TextPosition startPosition,
                                           gulc::TextPosition endPosition) const {
    std::cerr << "gulc const resolver warning[" << _filePaths[_currentFile->sourceFileID] << ", "
                                            "{" << startPosition.line << ", " << startPosition.column << " "
                                            "to " << endPosition.line << ", " << endPosition.column << "}]: "
              << message << std::endl;
}

bool gulc::ConstTypeResolver::resolveType(gulc::Type*& type) const {
    if (TypeHelper::resolveType(type, _currentFile, _templateParameters, _containingDecls)) {
        if (!TypeHelper::typeIsConstExpr(type)) {
            printError("type `" + type->toString() + "` is not a valid `const` type!",
                       type->startPosition(), type->endPosition());
            return false;
        }

        return true;
    } else {
        return false;
    }
}

void gulc::ConstTypeResolver::processDecl(gulc::Decl* decl, bool isGlobal) {
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
            printError("unknown declaration found!",
                       decl->startPosition(), decl->endPosition());
    }
}

void gulc::ConstTypeResolver::processFunctionDecl(gulc::FunctionDecl* functionDecl) {
    if (!functionDecl->isConstExpr()) return;

    for (ParameterDecl* parameter : functionDecl->parameters()) {
        processParameterDecl(parameter);
    }

    if (functionDecl->returnType == nullptr) {
        functionDecl->returnType = BuiltInType::get(Type::Qualifier::Unassigned, "void", {}, {});
    } else {
        if (!resolveType(functionDecl->returnType)) {
            printError("return type `" + functionDecl->returnType->toString() + "` was not found!",
                       functionDecl->returnType->startPosition(), functionDecl->returnType->endPosition());
        }
    }
}

void gulc::ConstTypeResolver::processNamespaceDecl(gulc::NamespaceDecl* namespaceDecl) {
    _containingDecls.push_back(namespaceDecl);

    for (Decl* decl : namespaceDecl->nestedDecls()) {
        processDecl(decl, true);
    }

    _containingDecls.pop_back();
}

void gulc::ConstTypeResolver::processParameterDecl(gulc::ParameterDecl* parameterDecl) const {
    if (!resolveType(parameterDecl->type)) {
        printError("parameter type `" + parameterDecl->type->toString() + "` was not found!",
                   parameterDecl->type->startPosition(), parameterDecl->type->endPosition());
    }
}

void gulc::ConstTypeResolver::processStructDecl(gulc::StructDecl* structDecl) {
    if (!structDecl->isConstExpr()) return;

    std::string errorName = structDecl->isClass() ? "class" : "struct";

    // TODO: Set the base struct reference
    for (Type*& inheritedType : structDecl->inheritedTypes()) {
        if (!resolveType(inheritedType)) {
            printError("inherited type `" + inheritedType->toString() + "` was not found!",
                       inheritedType->startPosition(), inheritedType->endPosition());
        }

        if (!llvm::isa<StructType>(inheritedType)) {
            printError("type `" + inheritedType->toString() + "` can not be inherited from, it is not a class or struct!",
                       inheritedType->startPosition(), inheritedType->endPosition());
        }
    }

    _containingDecls.push_back(structDecl);

    for (Decl* member : structDecl->allMembers()) {
        processDecl(member, false);
    }

    _containingDecls.pop_back();
}

void gulc::ConstTypeResolver::processTemplateFunctionDecl(gulc::TemplateFunctionDecl* templateFunctionDecl) {
    if (!templateFunctionDecl->isConstExpr()) return;

    for (TemplateParameterDecl* templateParameter : templateFunctionDecl->templateParameters()) {
        processTemplateParameterDecl(templateParameter);
    }

    _templateParameters.push_back(&templateFunctionDecl->templateParameters());

    processFunctionDecl(templateFunctionDecl);

    _templateParameters.pop_back();
}

void gulc::ConstTypeResolver::processTemplateParameterDecl(gulc::TemplateParameterDecl* templateParameterDecl) const {
    switch (templateParameterDecl->templateParameterKind()) {
        case TemplateParameterDecl::TemplateParameterKind::Typename:
            // Nothing needs to be done here (unless we add default values)
            break;
        case TemplateParameterDecl::TemplateParameterKind::Const:
            if (templateParameterDecl->constType == nullptr) {
                printError("const template parameter is missing a type!",
                           templateParameterDecl->startPosition(), templateParameterDecl->endPosition());
            }

            if (!resolveType(templateParameterDecl->constType)) {
                printError("const template parameter type `" + templateParameterDecl->constType->toString() + "` was not found!",
                           templateParameterDecl->constType->startPosition(), templateParameterDecl->constType->endPosition());
            }
            break;
        default:
            printError("unknown `TemplateParameterKind`!",
                       templateParameterDecl->startPosition(), templateParameterDecl->endPosition());
    }
}

void gulc::ConstTypeResolver::processTemplateStructDecl(gulc::TemplateStructDecl* templateStructDecl) {
    if (!templateStructDecl->isConstExpr()) return;

    for (TemplateParameterDecl* templateParameter : templateStructDecl->templateParameters()) {
        processTemplateParameterDecl(templateParameter);
    }

    _templateParameters.push_back(&templateStructDecl->templateParameters());

    processStructDecl(templateStructDecl);

    _templateParameters.pop_back();
}

void gulc::ConstTypeResolver::processVariableDecl(gulc::VariableDecl* variableDecl, bool isGlobal) const {
    // We only check `const` on variables outside of `struct`
    if (isGlobal && !variableDecl->isConstExpr()) return;
    // If we're in a struct and the variable is `static` we also return, we can't process those here.
    if (!isGlobal && variableDecl->isStatic()) return;

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
