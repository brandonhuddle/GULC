#include <iostream>
#include <ast/types/StructType.hpp>
#include <utilities/TypeHelper.hpp>
#include <ast/types/TemplatedType.hpp>
#include "BaseResolver.hpp"

// TODO: To do this properly we should:
//       1. Only support literals for consts at first
//       2. Don't allow function calls in `where` clauses
//       3.

void gulc::BaseResolver::processFiles(std::vector<ASTFile>& files) {
    for (ASTFile& file : files) {
        _currentFile = &file;

        for (Decl* decl : file.declarations) {
            processDecl(decl);
        }
    }
}

void gulc::BaseResolver::printError(std::string const& message, gulc::TextPosition startPosition,
                                    gulc::TextPosition endPosition) const {
    std::cerr << "gulc base resolver error[" << _filePaths[_currentFile->sourceFileID] << ", "
                                         "{" << startPosition.line << ", " << startPosition.column << " "
                                         "to " << endPosition.line << ", " << endPosition.column << "}]: "
              << message << std::endl;
    std::exit(1);
}

void gulc::BaseResolver::printWarning(std::string const& message, gulc::TextPosition startPosition,
                                      gulc::TextPosition endPosition) const {
    std::cerr << "gulc base resolver warning[" << _filePaths[_currentFile->sourceFileID] << ", "
                                           "{" << startPosition.line << ", " << startPosition.column << " "
                                           "to " << endPosition.line << ", " << endPosition.column << "}]: "
              << message << std::endl;
}

bool gulc::BaseResolver::resolveType(gulc::Type*& type) const {
    bool result = TypeHelper::resolveType(type, _currentFile, _templateParameters, _containingDecls);

    if (result) {
        // If the result type is a `TemplatedType` we will solve everything required to solve the template type here...
        if (llvm::isa<TemplatedType>(type)) {
            asdasdasdssssadasdasdaaasdasdasdasddddsdsdsdsasdaasdasdasdasdasdsddsdsdasdasddsasdasdasdsdsdsdsssssssssdssssdsdsssdssssdsddsddsssddssssddsssssdddssssddssssdddddddddddddd
        }
    }

    return result;
}

void gulc::BaseResolver::processDecl(gulc::Decl* decl) {
    switch (decl->getDeclKind()) {
        case Decl::Kind::Import:
            // We skip imports, they're no longer useful here...
            break;

        case Decl::Kind::Namespace:
            processNamespaceDecl(llvm::dyn_cast<NamespaceDecl>(decl));
            break;
        case Decl::Kind::Struct:
            processStructDecl(llvm::dyn_cast<StructDecl>(decl));
            break;
        case Decl::Kind::TemplateStruct:
            processTemplateStructDecl(llvm::dyn_cast<TemplateStructDecl>(decl));
            break;

        default:
//            printError("unknown declaration found!",
//                       decl->startPosition(), decl->endPosition());
            // If we don't know the declaration we just skip it, we don't care in this pass
            break;
    }
}

void gulc::BaseResolver::processNamespaceDecl(gulc::NamespaceDecl* namespaceDecl) {
    for (Decl* decl : namespaceDecl->nestedDecls()) {
        processDecl(decl);
    }
}

void gulc::BaseResolver::processStructDecl(gulc::StructDecl* structDecl) {
    if (structDecl->baseWasResolved) {
        return;
    }

    for (Type*& inheritedType : structDecl->inheritedTypes()) {
        if (!resolveType(inheritedType)) {
            printError("could not resolve base type `" + inheritedType->toString() + "`!",
                       inheritedType->startPosition(), inheritedType->endPosition());
        }

        if (llvm::isa<StructType>(inheritedType)) {
            if (structDecl->baseStruct != nullptr) {
                printError("inheriting from multiple structs/classes is not supported!",
                           inheritedType->startPosition(), inheritedType->endPosition());
            }

            structDecl->baseStruct = llvm::dyn_cast<StructType>(inheritedType)->decl();
        }
    }

    structDecl->baseWasResolved = true;
}

void gulc::BaseResolver::processTemplateParameterDecl(gulc::TemplateParameterDecl* templateParameterDecl) const {

}

void gulc::BaseResolver::processTemplateStructDecl(gulc::TemplateStructDecl* templateStructDecl) {
    // TODO: We will have to append to the current template parameter list...
    processStructDecl(templateStructDecl);
}
