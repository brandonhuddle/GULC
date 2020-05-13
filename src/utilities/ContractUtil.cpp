#include <iostream>
#include <ast/types/TemplateTypenameRefType.hpp>
#include <llvm/Support/Casting.h>
#include <ast/exprs/TypeExpr.hpp>
#include <ast/types/StructType.hpp>
#include <ast/types/TraitType.hpp>
#include <ast/types/TemplateStructType.hpp>
#include <ast/types/TemplateTraitType.hpp>
#include "ContractUtil.hpp"
#include "TypeCompareUtil.hpp"

bool gulc::ContractUtil::checkWhereCont(gulc::WhereCont* whereCont) {
    switch (whereCont->condition->getExprKind()) {
        case Expr::Kind::CheckExtendsType:
            return checkCheckExtendsTypeExpr(llvm::dyn_cast<CheckExtendsTypeExpr>(whereCont->condition));
        default:
            printError("unsupported expression found in `where` clause!",
                       whereCont->startPosition(), whereCont->endPosition());
            break;
    }
    return false;
}

void gulc::ContractUtil::printError(const std::string& message, gulc::TextPosition startPosition,
                                    gulc::TextPosition endPosition) const {
    std::cerr << "gulc error[" << _fileName << ", "
                            "{" << startPosition.line << ", " << startPosition.column << " "
                            "to " << endPosition.line << ", " << endPosition.column << "}]: "
              << message << std::endl;
    std::exit(1);
}

void gulc::ContractUtil::printWarning(const std::string& message, gulc::TextPosition startPosition,
                                      gulc::TextPosition endPosition) const {
    std::cerr << "gulc warning[" << _fileName << ", "
                              "{" << startPosition.line << ", " << startPosition.column << " "
                              "to " << endPosition.line << ", " << endPosition.column << "}]: "
              << message << std::endl;
}

const gulc::Type* gulc::ContractUtil::getTemplateTypeArgument(const gulc::TemplateTypenameRefType* templateTypenameRefType) const {
    for (std::size_t i = 0; i < _templateParameters->size(); ++i) {
        if ((*_templateParameters)[i] == templateTypenameRefType->refTemplateParameter()) {
            auto argument = (*_templateArguments)[i];

            if (!llvm::isa<TypeExpr>(argument)) {
                printError("[INTERNAL ERROR] expected `TypeExpr`!",
                           argument->startPosition(), argument->endPosition());
            }

            return llvm::dyn_cast<TypeExpr>(argument)->type;
        }
    }

    // If we didn't find the parameter we return the typename ref type, it might reference a template type from another
    // template that contains us.
    return templateTypenameRefType;
}

bool gulc::ContractUtil::checkCheckExtendsTypeExpr(gulc::CheckExtendsTypeExpr* checkExtendsTypeExpr) const {
    // TODO: Is there anything else we should support here?
    if (llvm::isa<TemplateTypenameRefType>(checkExtendsTypeExpr->checkType)) {
        auto templateTypenameRefType = llvm::dyn_cast<TemplateTypenameRefType>(checkExtendsTypeExpr->checkType);

        // TODO:
        //       1. Grab the `argument` using the template parameter above
        //       2. Using the `arg` check if that type inherits from the `extendsType`
        //       3. Return result
        // Grab the actual type argument for the parameter reference
        Type const* argType = getTemplateTypeArgument(templateTypenameRefType);

        // TODO: We need to support checking if the type has an extension that adds the trait...
        switch (argType->getTypeKind()) {
            case Type::Kind::Struct: {
                auto structType = llvm::dyn_cast<StructType>(argType);

                if (!structType->decl()->inheritedTypesIsInitialized) {
                    printError("[INTERNAL ERROR] uninitialized struct found in type extension check!",
                               structType->startPosition(), structType->endPosition());
                }

                TypeCompareUtil typeCompareUtil;

                // We check `allInheritedTypes` as it holds all types, even the ones not explicitly stated by the
                // currently being checked struct decl
                for (Type const* checkType : structType->decl()->inheritedTypes()) {
                    if (typeCompareUtil.compareAreSameOrInherits(checkType, checkExtendsTypeExpr->extendsType)) {
                        return true;
                    }
                }

                break;
            }
            case Type::Kind::Trait: {
                auto traitType = llvm::dyn_cast<TraitType>(argType);

                if (!traitType->decl()->inheritedTypesIsInitialized) {
                    printError("[INTERNAL ERROR] uninitialized trait found in type extension check!",
                               traitType->startPosition(), traitType->endPosition());
                }

                TypeCompareUtil typeCompareUtil;

                // We check `allInheritedTypes` as it holds all types, even the ones not explicitly stated by the
                // currently being checked struct decl
                for (Type const* checkType : traitType->decl()->inheritedTypes()) {
                    if (typeCompareUtil.compareAreSameOrInherits(checkType, checkExtendsTypeExpr->extendsType)) {
                        return true;
                    }
                }

                break;
            }
            case Type::Kind::TemplateStruct: {
                auto templateStructType = llvm::dyn_cast<TemplateStructType>(argType);

                if (!templateStructType->decl()->inheritedTypesIsInitialized) {
                    printError("[INTERNAL ERROR] uninitialized struct found in type extension check!",
                               templateStructType->startPosition(), templateStructType->endPosition());
                }

                TypeCompareUtil typeCompareUtil(&templateStructType->decl()->templateParameters(),
                                                &templateStructType->templateArguments());

                // We check `allInheritedTypes` as it holds all types, even the ones not explicitly stated by the
                // currently being checked struct decl
                for (Type const* checkType : templateStructType->decl()->inheritedTypes()) {
                    if (typeCompareUtil.compareAreSameOrInherits(checkType, checkExtendsTypeExpr->extendsType)) {
                        return true;
                    }
                }

                break;
            }
            case Type::Kind::TemplateTrait: {
                auto templateTraitType = llvm::dyn_cast<TemplateTraitType>(argType);

                if (!templateTraitType->decl()->inheritedTypesIsInitialized) {
                    printError("[INTERNAL ERROR] uninitialized trait found in type extension check!",
                               templateTraitType->startPosition(), templateTraitType->endPosition());
                }

                TypeCompareUtil typeCompareUtil(&templateTraitType->decl()->templateParameters(),
                                                &templateTraitType->templateArguments());

                // We check `allInheritedTypes` as it holds all types, even the ones not explicitly stated by the
                // currently being checked struct decl
                for (Type const* checkType : templateTraitType->decl()->inheritedTypes()) {
                    if (typeCompareUtil.compareAreSameOrInherits(checkType, checkExtendsTypeExpr->extendsType)) {
                        return true;
                    }
                }

                break;
            }
            case Type::Kind::TemplateTypenameRef:
                // TODO: We need to check the typename ref to see if it has preexisting rules for this type...
            default: {
                TypeCompareUtil typeCompareUtil;

                // To allow for `T : i32`...
                if (typeCompareUtil.compareAreSameOrInherits(argType, checkExtendsTypeExpr->extendsType)) {
                    return true;
                }

                break;
            }
        }
    } else {
        printError("`:` can only be used on template types within this context!",
                   checkExtendsTypeExpr->startPosition(), checkExtendsTypeExpr->endPosition());
    }

    return false;
}
