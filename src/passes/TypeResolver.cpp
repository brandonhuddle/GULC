#include <iostream>
#include <ast/types/DimensionType.hpp>
#include <ast/types/PointerType.hpp>
#include <ast/types/ReferenceType.hpp>
#include <ast/types/UnresolvedType.hpp>
#include <ast/types/BuiltInType.hpp>
#include <make_reverse_iterator.hpp>
#include <ast/types/TemplateTypenameRefType.hpp>
#include <ast/types/StructType.hpp>
#include <utilities/TypeHelper.hpp>
#include "TypeResolver.hpp"

void gulc::TypeResolver::processFiles(std::vector<ASTFile>& files) {
    for (ASTFile& file : files) {
        _currentFile = &file;

        for (Decl* decl : file.declarations) {
            processDecl(decl, true);
        }
    }
}

void gulc::TypeResolver::printError(std::string const& message, gulc::TextPosition startPosition,
                                    gulc::TextPosition endPosition) const {
    std::cerr << "gulc resolver error[" << _filePaths[_currentFile->sourceFileID] << ", "
                                    "{" << startPosition.line << ", " << startPosition.column << " "
                                    "to " << endPosition.line << ", " << endPosition.column << "}]: "
              << message << std::endl;
    std::exit(1);
}

void gulc::TypeResolver::printWarning(std::string const& message, gulc::TextPosition startPosition,
                                      gulc::TextPosition endPosition) const {
    std::cerr << "gulc resolver warning[" << _filePaths[_currentFile->sourceFileID] << ", "
                                      "{" << startPosition.line << ", " << startPosition.column << " "
                                      "to " << endPosition.line << ", " << endPosition.column << "}]: "
              << message << std::endl;
}

bool gulc::TypeResolver::resolveType(gulc::Type*& type) const {
    return TypeHelper::resolveType(type, _currentFile, _templateParameters, _containingDecls);
}

void gulc::TypeResolver::processDecl(gulc::Decl* decl, bool isGlobal) {
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

void gulc::TypeResolver::processFunctionDecl(gulc::FunctionDecl* functionDecl) {
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

void gulc::TypeResolver::processNamespaceDecl(gulc::NamespaceDecl* namespaceDecl) {
    _containingDecls.push_back(namespaceDecl);

    for (Decl* decl : namespaceDecl->nestedDecls()) {
        processDecl(decl, true);
    }

    _containingDecls.pop_back();
}

void gulc::TypeResolver::processParameterDecl(gulc::ParameterDecl* parameterDecl) const {
    if (!resolveType(parameterDecl->type)) {
        printError("parameter type `" + parameterDecl->type->toString() + "` was not found!",
                   parameterDecl->type->startPosition(), parameterDecl->type->endPosition());
    }
}

void gulc::TypeResolver::processStructDecl(gulc::StructDecl* structDecl) {
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

void gulc::TypeResolver::processTemplateFunctionDecl(gulc::TemplateFunctionDecl* templateFunctionDecl) {
    for (TemplateParameterDecl* templateParameter : templateFunctionDecl->templateParameters()) {
        processTemplateParameterDecl(templateParameter);
    }

    _templateParameters.push_back(&templateFunctionDecl->templateParameters());

    processFunctionDecl(templateFunctionDecl);

    _templateParameters.pop_back();
}

void gulc::TypeResolver::processTemplateParameterDecl(gulc::TemplateParameterDecl* templateParameterDecl) const {
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

void gulc::TypeResolver::processTemplateStructDecl(gulc::TemplateStructDecl* templateStructDecl) {
    for (TemplateParameterDecl* templateParameter : templateStructDecl->templateParameters()) {
        processTemplateParameterDecl(templateParameter);
    }

    _templateParameters.push_back(&templateStructDecl->templateParameters());

    processStructDecl(templateStructDecl);

    _templateParameters.pop_back();
}

void gulc::TypeResolver::processVariableDecl(gulc::VariableDecl* variableDecl, bool isGlobal) const {
    if (isGlobal) {
        if (!variableDecl->isConstExpr() && !variableDecl->isStatic()) {
            printError("global variables must be marked `const` or `static`!",
                       variableDecl->startPosition(), variableDecl->endPosition());
        }
    }

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
