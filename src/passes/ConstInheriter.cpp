#include <iostream>
#include "ConstInheriter.hpp"

void gulc::ConstInheriter::processFiles(std::vector<ASTFile>& files) {
    for (ASTFile& file : files) {
        _currentFile = &file;

        for (Decl* decl : file.declarations) {
            processDecl(decl, true);
        }
    }
}

void gulc::ConstInheriter::printError(std::string const& message, gulc::TextPosition startPosition,
                                      gulc::TextPosition endPosition) const {
    std::cerr << "gulc const resolver error[" << _filePaths[_currentFile->sourceFileID] << ", "
                                          "{" << startPosition.line << ", " << startPosition.column << " "
                                          "to " << endPosition.line << ", " << endPosition.column << "}]: "
              << message << std::endl;
    std::exit(1);
}

void gulc::ConstInheriter::printWarning(std::string const& message, gulc::TextPosition startPosition,
                                        gulc::TextPosition endPosition) const {
    std::cerr << "gulc const resolver warning[" << _filePaths[_currentFile->sourceFileID] << ", "
                                            "{" << startPosition.line << ", " << startPosition.column << " "
                                            "to " << endPosition.line << ", " << endPosition.column << "}]: "
              << message << std::endl;
}

bool gulc::ConstInheriter::resolveType(gulc::Type*& type) const {
    return false;
}

void gulc::ConstInheriter::processDecl(gulc::Decl* decl, bool isGlobal) {
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

void gulc::ConstInheriter::processFunctionDecl(gulc::FunctionDecl* functionDecl) {
    
}

void gulc::ConstInheriter::processNamespaceDecl(gulc::NamespaceDecl* namespaceDecl) {

}

void gulc::ConstInheriter::processParameterDecl(gulc::ParameterDecl* parameterDecl) const {

}

void gulc::ConstInheriter::processStructDecl(gulc::StructDecl* structDecl) {

}

void gulc::ConstInheriter::processTemplateFunctionDecl(gulc::TemplateFunctionDecl* templateFunctionDecl) {

}

void gulc::ConstInheriter::processTemplateParameterDecl(gulc::TemplateParameterDecl* templateParameterDecl) const {

}

void gulc::ConstInheriter::processTemplateStructDecl(gulc::TemplateStructDecl* templateStructDecl) {

}

void gulc::ConstInheriter::processVariableDecl(gulc::VariableDecl* variableDecl, bool isGlobal) const {

}
