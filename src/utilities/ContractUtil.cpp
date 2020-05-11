#include <iostream>
#include <ast/types/TemplateTypenameRefType.hpp>
#include <llvm/Support/Casting.h>
#include <ast/exprs/TypeExpr.hpp>
#include <ast/types/StructType.hpp>
#include <ast/types/TraitType.hpp>
#include "ContractUtil.hpp"

bool gulc::ContractUtil::checkWhereCont(gulc::WhereCont* whereCont) {
    switch (whereCont->condition->getExprKind()) {
        case Expr::Kind::CheckExtendsType:

            break;
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
                // TODO: START HERE ---------------------------------------------------------------------------------------
                asdasdasdasdasdasdasdasd
                break;
            }
            case Type::Kind::Trait: {
                auto traitType = llvm::dyn_cast<TraitType>(argType);
                break;
            }
            case Type::Kind::TemplateStruct:
            case Type::Kind::TemplateTrait:
                // TODO: We need to support `TemplateStruct` and `TemplateTrait`...
            case Type::Kind::TemplateTypenameRef:
                // TODO: We need to check the typename ref to see if it has preexisting rules for this type...
            default:
                printError("unsupported type found on left side of `:` check!",
                           checkExtendsTypeExpr->startPosition(), checkExtendsTypeExpr->endPosition());
                break;
        }
    } else {
        printError("`:` can only be used on template types within this context!",
                   checkExtendsTypeExpr->startPosition(), checkExtendsTypeExpr->endPosition());
    }

    return false;
}
