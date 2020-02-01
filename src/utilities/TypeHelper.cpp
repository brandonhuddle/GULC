#include <llvm/Support/Casting.h>
#include <ast/types/DimensionType.hpp>
#include <ast/types/PointerType.hpp>
#include <ast/types/ReferenceType.hpp>
#include <ast/types/UnresolvedType.hpp>
#include <ast/types/BuiltInType.hpp>
#include <make_reverse_iterator.hpp>
#include <ast/types/TemplateTypenameRefType.hpp>
#include <ast/decls/StructDecl.hpp>
#include <ast/types/StructType.hpp>
#include <ast/decls/TemplateStructDecl.hpp>
#include <ast/types/TemplatedType.hpp>
#include "TypeHelper.hpp"

bool gulc::TypeHelper::resolveType(gulc::Type*& type, ASTFile const* currentFile,
                                   std::vector<std::vector<TemplateParameterDecl*>*> const& templateParameters,
                                   std::vector<gulc::Decl*> const& containingDecls) {
    switch (type->getTypeKind()) {
        case Type::Kind::BuiltIn:
            return true;
        case Type::Kind::Dimension: {
            auto dimensionType = llvm::dyn_cast<DimensionType>(type);
            return resolveType(dimensionType->nestedType, currentFile, templateParameters, containingDecls);
        }
        case Type::Kind::Pointer: {
            auto pointerType = llvm::dyn_cast<PointerType>(type);
            return resolveType(pointerType->nestedType, currentFile, templateParameters, containingDecls);
        }
        case Type::Kind::Reference: {
            auto referenceType = llvm::dyn_cast<ReferenceType>(type);
            return resolveType(referenceType->nestedType, currentFile, templateParameters, containingDecls);
        }
        case Type::Kind::Unresolved: {
            auto unresolvedType = llvm::dyn_cast<UnresolvedType>(type);

            if (!unresolvedType->namespacePath().empty()) {
                return false;
            }

            std::string const& checkName = unresolvedType->identifier().name();
            bool templated = unresolvedType->hasTemplateArguments();
            std::vector<Decl*> potentialTemplates;

            if (!templated) {
                // First check if it is a built in type
                if (BuiltInType::isBuiltInType(checkName)) {
                    auto result = BuiltInType::get(unresolvedType->qualifier(), checkName,
                                                   unresolvedType->startPosition(), unresolvedType->endPosition());
                    result->setIsLValue(unresolvedType->isLValue());

                    delete unresolvedType;

                    type = result;

                    return true;
                }
            }

            // Then if there are template parameters we'll check them
            // NOTE: Template parameter types cannot be templated themselves
            if (!templated && !templateParameters.empty()) {
                for (std::vector<TemplateParameterDecl*>* checkTemplateParameters :
                        gulc::reverse(templateParameters)) {
                    for (TemplateParameterDecl* templateParameter : *checkTemplateParameters) {
                        if (templateParameter->templateParameterKind() ==
                            TemplateParameterDecl::TemplateParameterKind::Typename) {
                            if (checkName == templateParameter->identifier().name()) {
                                auto result = new TemplateTypenameRefType(unresolvedType->qualifier(),
                                                                          templateParameter,
                                                                          unresolvedType->startPosition(),
                                                                          unresolvedType->endPosition());
                                result->setIsLValue(unresolvedType->isLValue());

                                delete unresolvedType;

                                type = result;

                                return true;
                            }
                        }
                    }
                }
            }

            // Then check our containing decls
            if (!containingDecls.empty()) {
                for (Decl* checkContainer : gulc::reverse(containingDecls)) {
                    if (templated) {
                        if (llvm::isa<TemplateStructDecl>(checkContainer)) {
                            auto checkTemplateStruct = llvm::dyn_cast<TemplateStructDecl>(checkContainer);

                            if (checkTemplateStruct->identifier().name() == checkName) {
                                // TODO: Support optional template parameters
                                if (checkTemplateStruct->templateParameters().size() == unresolvedType->templateArguments.size()) {
                                    potentialTemplates.push_back(checkTemplateStruct);
                                    // We keep searching as this might be the wrong type...
                                }
                            }
                        }
                    } else {
                        if (llvm::isa<StructDecl>(checkContainer)) {
                            auto checkStruct = llvm::dyn_cast<StructDecl>(checkContainer);

                            if (checkStruct->identifier().name() == checkName) {
                                auto result = new StructType(unresolvedType->qualifier(), checkStruct,
                                                             unresolvedType->startPosition(),
                                                             unresolvedType->endPosition());
                                result->setIsLValue(unresolvedType->isLValue());

                                delete unresolvedType;

                                type = result;

                                return true;
                            }
                        }
                    }
                }
            }


            // Then check our file
            for (Decl* checkContainer : currentFile->declarations) {
                if (templated) {
                    if (llvm::isa<TemplateStructDecl>(checkContainer)) {
                        auto checkTemplateStruct = llvm::dyn_cast<TemplateStructDecl>(checkContainer);

                        if (checkTemplateStruct->identifier().name() == checkName) {
                            // TODO: Support optional template parameters
                            if (checkTemplateStruct->templateParameters().size() == unresolvedType->templateArguments.size()) {
                                potentialTemplates.push_back(checkTemplateStruct);
                                // We keep searching as this might be the wrong type...
                            }
                        }
                    }
                } else {
                    if (llvm::isa<StructDecl>(checkContainer)) {
                        auto checkStruct = llvm::dyn_cast<StructDecl>(checkContainer);

                        if (checkStruct->identifier().name() == checkName) {
                            auto result = new StructType(unresolvedType->qualifier(), checkStruct,
                                                         unresolvedType->startPosition(),
                                                         unresolvedType->endPosition());
                            result->setIsLValue(unresolvedType->isLValue());

                            delete unresolvedType;

                            type = result;

                            return true;
                        }
                    }
                }
            }

            // Then check our imports

            if (templated && !potentialTemplates.empty()) {
                auto result = new TemplatedType(unresolvedType->qualifier(), potentialTemplates,
                                                std::move(unresolvedType->templateArguments),
                                                unresolvedType->startPosition(), unresolvedType->endPosition());
                result->setIsLValue(unresolvedType->isLValue());

                delete unresolvedType;

                type = result;

                return true;
            } else {
                return false;
            }
        }
        default:
            // If we don't know what the type is we can't resolve it
            return false;
    }
}

bool gulc::TypeHelper::typeIsConstExpr(gulc::Type* resolvedType) {
    switch (resolvedType->getTypeKind()) {
        case Type::Kind::BuiltIn:
            return true;
        case Type::Kind::Dimension:
            // TODO: We cannot know if `[]` is const or not...
            return false;
        case Type::Kind::Pointer: {
            auto pointerType = llvm::dyn_cast<PointerType>(resolvedType);
            return typeIsConstExpr(pointerType->nestedType);
        }
        case Type::Kind::Reference: {
            auto referenceType = llvm::dyn_cast<ReferenceType>(resolvedType);
            return typeIsConstExpr(referenceType->nestedType);
        }
        case Type::Kind::Struct: {
            auto structType = llvm::dyn_cast<StructType>(resolvedType);
            return structType->decl()->isConstExpr();
        }
        case Type::Kind::Templated: {
            // NOTE: A templated type means it is a template that isn't 100% resolved. We can't know if it is `const`
            //       or not so we lie and say it is. If it turns out it isn't that must be handled in a pass
            return true;
        }
        case Type::Kind::TemplateTypenameRef: {
            // NOTE: A template typename is considered const-qualified in this pass.
            //       If this is being used in a situation where `const` is required then the argument will be checked
            //       to be a valid `const` instead.
            return true;
        }
        default:
            return false;
    }
}
