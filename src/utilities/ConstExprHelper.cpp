#include <ast/exprs/TypeExpr.hpp>
#include <llvm/Support/Casting.h>
#include <iostream>
#include <ast/exprs/ValueLiteralExpr.hpp>
#include <ast/types/DimensionType.hpp>
#include <ast/types/FlatArrayType.hpp>
#include <ast/types/PointerType.hpp>
#include <ast/types/ReferenceType.hpp>
#include "ConstExprHelper.hpp"
#include "TypeCompareUtil.hpp"

bool gulc::ConstExprHelper::compareAreSame(const gulc::Expr* left, const gulc::Expr* right) {
    // NOTE: Should this always be correct? Are there any scenarios where the kinds could differ?
    if (left->getExprKind() == right->getExprKind()) {
        switch (left->getExprKind()) {
            case Expr::Kind::Type: {
                auto leftType = llvm::dyn_cast<TypeExpr>(left);
                auto rightType = llvm::dyn_cast<TypeExpr>(right);

                gulc::TypeCompareUtil typeCompareUtil;
                return typeCompareUtil.compareAreSame(leftType->type, rightType->type);
            }
            case Expr::Kind::ValueLiteral: {
                auto leftValueLiteral = llvm::dyn_cast<ValueLiteralExpr>(left);
                auto rightValueLiteral = llvm::dyn_cast<ValueLiteralExpr>(right);

                gulc::TypeCompareUtil typeCompareUtil;

                // For `ConstExprHelper` we only compare two literals of the same type. The `const` expression should
                // be solved before calling us
                if (leftValueLiteral->literalType() != rightValueLiteral->literalType() ||
                    !typeCompareUtil.compareAreSame(leftValueLiteral->valueType, rightValueLiteral->valueType)) {
                    return false;
                }

                // TODO: Does this work for all situations?
                return leftValueLiteral->value() == rightValueLiteral->value();
            }
            default:
                std::cerr << "FATAL ERROR: `gulc::ConstExprHelper::compareAreSame` has unknown const expr type!" << std::endl;
                std::exit(1);
                break;
        }
    }

    return false;
}

bool gulc::ConstExprHelper::templateArgumentsAreSolved(std::vector<Expr*>& templateArguments) {
    for (Expr* checkArgument : templateArguments) {
        if (!templateArgumentIsSolved(checkArgument)) {
            return false;
        }
    }

    return true;
}

bool gulc::ConstExprHelper::templateArgumentIsSolved(gulc::Expr* checkArgument) {
    switch (checkArgument->getExprKind()) {
        case Expr::Kind::TemplateConstRefExpr:
            return false;
        case Expr::Kind::Type: {
            auto typeExpr = llvm::dyn_cast<TypeExpr>(checkArgument);

            return templateTypeArgumentIsSolved(typeExpr->type);
        }
        case Expr::Kind::ValueLiteral:
            return true;
        default:
            return false;
    }
}

bool gulc::ConstExprHelper::templateTypeArgumentIsSolved(gulc::Type* checkType) {
    switch (checkType->getTypeKind()) {
        case Type::Kind::BuiltIn:
            return true;
        case Type::Kind::Dimension: {
            auto dimensionType = llvm::dyn_cast<DimensionType>(checkType);

            return templateTypeArgumentIsSolved(dimensionType->nestedType);
        }
        case Type::Kind::Enum:
            return true;
        case Type::Kind::FlatArray: {
            auto flatArrayType = llvm::dyn_cast<FlatArrayType>(checkType);

            return templateTypeArgumentIsSolved(flatArrayType->indexType) &&
                   templateArgumentIsSolved(flatArrayType->length);
        }
        case Type::Kind::Pointer: {
            auto pointerType = llvm::dyn_cast<PointerType>(checkType);

            return templateTypeArgumentIsSolved(pointerType->nestedType);
        }
        case Type::Kind::Reference: {
            auto referenceType = llvm::dyn_cast<ReferenceType>(checkType);

            return templateTypeArgumentIsSolved(referenceType->nestedType);
        }
        case Type::Kind::Struct:
            return true;
        case Type::Kind::Trait:
            return true;
        default:
            return false;
    }
}
