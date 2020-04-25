#include <ast/exprs/TypeExpr.hpp>
#include <llvm/Support/Casting.h>
#include <iostream>
#include <ast/exprs/ValueLiteralExpr.hpp>
#include "ConstExprHelper.hpp"
#include "TypeHelper.hpp"

bool gulc::ConstExprHelper::compareAreSame(const gulc::Expr* left, const gulc::Expr* right) {
    // NOTE: Should this always be correct? Are there any scenarios where the kinds could differ?
    if (left->getExprKind() == right->getExprKind()) {
        switch (left->getExprKind()) {
            case Expr::Kind::Type: {
                auto leftType = llvm::dyn_cast<TypeExpr>(left);
                auto rightType = llvm::dyn_cast<TypeExpr>(right);

                return gulc::TypeHelper::compareAreSame(leftType->type, rightType->type);
            }
            case Expr::Kind::ValueLiteral: {
                auto leftValueLiteral = llvm::dyn_cast<ValueLiteralExpr>(left);
                auto rightValueLiteral = llvm::dyn_cast<ValueLiteralExpr>(right);

                // For `ConstExprHelper` we only compare two literals of the same type. The `const` expression should
                // be solved before calling us
                if (leftValueLiteral->literalType() != rightValueLiteral->literalType() ||
                    !TypeHelper::compareAreSame(leftValueLiteral->valueType, rightValueLiteral->valueType)) {
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
