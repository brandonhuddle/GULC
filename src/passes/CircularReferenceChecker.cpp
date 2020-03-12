#include <iostream>
#include <set>
#include "CircularReferenceChecker.hpp"

void gulc::CircularReferenceChecker::processFiles(std::vector<ASTFile>& files) {
    for (ASTFile& file : files) {
        _currentFile = &file;

        for (Decl* decl : file.declarations) {
            processDecl(decl);
        }
    }
}

void gulc::CircularReferenceChecker::printError(std::string const& message, gulc::TextPosition startPosition,
                                                gulc::TextPosition endPosition) const {
    std::cerr << "gulc resolver error[" << _filePaths[_currentFile->sourceFileID] << ", "
                                    "{" << startPosition.line << ", " << startPosition.column << " "
                                    "to " << endPosition.line << ", " << endPosition.column << "}]: "
              << message << std::endl;
    std::exit(1);
}

void gulc::CircularReferenceChecker::processDecl(gulc::Decl* decl) {
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

void gulc::CircularReferenceChecker::processNamespaceDecl(gulc::NamespaceDecl* namespaceDecl) {
    for (Decl* decl : namespaceDecl->nestedDecls()) {
        processDecl(decl);
    }
}

void gulc::CircularReferenceChecker::processStructDecl(gulc::StructDecl* structDecl) {
    // We only have to check the `baseStruct` since `trait`s cannot implement new members and cannot extend structs
    std::set<StructDecl*> inheritedStructs;

    if (structDecl->baseStruct == structDecl) {
        printError("structs cannot extend from themselves!",
                   structDecl->startPosition(), structDecl->endPosition());
    }

    StructDecl* lastBaseStruct = nullptr;

    // Loop through all base structs and check for any circular references
    for (StructDecl* baseStruct = structDecl->baseStruct; baseStruct != nullptr; baseStruct = baseStruct->baseStruct) {
        if (baseStruct == structDecl) {
            if (lastBaseStruct == nullptr) {
                lastBaseStruct = baseStruct;
            }

            // This branch is only here to try to improve on error message descriptiveness
            // If `baseStruct == structDecl` we want to print the name of the struct that came right before this struct
            // that is why we print `lastBaseStruct`
            printError("struct `" + structDecl->identifier().name() +
                       "` has an illegal circular reference caused by the struct `" + lastBaseStruct->identifier().name() + "`!",
                       structDecl->startPosition(), structDecl->endPosition());
        } else if (inheritedStructs.count(baseStruct) > 0) {
            printError("struct `" + structDecl->identifier().name() +
                       "` has an illegal circular reference caused by the struct `" + baseStruct->identifier().name() + "`!",
                       structDecl->startPosition(), structDecl->endPosition());
        } else {
            inheritedStructs.insert(baseStruct);
        }

        lastBaseStruct = baseStruct;
    }
}

void gulc::CircularReferenceChecker::processTemplateStructDecl(gulc::TemplateStructDecl* templateStructDecl) {
    // TODO: We will have to handle template types here too...
    processStructDecl(templateStructDecl);
}
