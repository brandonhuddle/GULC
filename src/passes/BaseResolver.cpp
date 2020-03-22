#include <iostream>
#include <ast/types/StructType.hpp>
#include <utilities/TypeHelper.hpp>
#include <ast/types/TemplatedType.hpp>
#include <ast/exprs/IdentifierExpr.hpp>
#include <ast/types/UnresolvedType.hpp>
#include <ast/exprs/TypeExpr.hpp>
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
            auto templatedType = llvm::dyn_cast<TemplatedType>(type);

            // Process the template arguments and try to resolve any potential types in the list...
            for (Expr*& templateArgument : templatedType->templateArguments()) {
                processExprTypeOrConst(templateArgument);
            }

            // TODO: Loop through the list of potential declarations and try to find a match. Also account for any
            //       ambiguous types.
            Decl* foundTemplateDecl = nullptr;
            bool isExact = false;
            bool isAmbiguous = false;

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
                                        break;
                                    }
                                } else {
                                    if (llvm::isa<TypeExpr>(templatedType->templateArguments()[i])) {
                                        // If the parameter is a `const` then it MUST NOT be a type...
                                        // TODO: Once we support literals in the template parameter list, we need to
                                        //       differentiate templates based on the literal types.
                                        //       I.e. `Example<const x: f32>` and `Example<const x: f64>`
                                        break;
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
            }

            // TODO: Else, the template type was found to an exact declaration...
            // NOTE: I'm not sure if there is a better way to do this
            //       I'm just re-detecting what the Decl is for the template. It could be better to
            //       Make a generate "TemplateDecl" or something that will handle holding everything for
            //       "TemplateStructDecl", "TemplateTraitDecl", etc.
            switch (foundTemplateDecl->getDeclKind()) {
                case Decl::Kind::TemplateStruct: {
                    
                    break;
                }
                default:
                    printWarning("[INTERNAL] unknown template declaration found in `BaseResolver::resolveType`!",
                                 foundTemplateDecl->startPosition(), foundTemplateDecl->endPosition());
                    break;
            }
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

void gulc::BaseResolver::processExprTypeOrConst(gulc::Expr*& expr) const {
    // TODO: Support more than just types
    if (llvm::isa<IdentifierExpr>(expr)) {
        auto identifierExpr = llvm::dyn_cast<IdentifierExpr>(expr);

        Type* tmpType = new UnresolvedType(Type::Qualifier::Unassigned, {},
                                           identifierExpr->identifier(),
                                           identifierExpr->templateArguments());

        if (resolveType(tmpType)) {
            // We clear them as they should now be used by `tmpType` OR no be deleted...
            identifierExpr->templateArguments().clear();

            expr = new TypeExpr(tmpType);

            // Delete the no longer needed identifier expression
            delete identifierExpr;
        } else {
            printError("type `" + identifierExpr->identifier().name() + "` was not found!",
                       identifierExpr->startPosition(), identifierExpr->endPosition());
        }
    } else {
        printError("currently only types are supported in template argument lists! (const expressions coming soon)",
                   expr->startPosition(), expr->endPosition());
    }
}
