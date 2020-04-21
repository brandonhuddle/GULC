#include <ast/exprs/TypeExpr.hpp>
#include <llvm/Support/Casting.h>
#include <iostream>
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
            default:
                // TODO: We need to support comparing literals
                std::cerr << "FATAL ERROR: `gulc::ConstExprHelper::compareAreSame` has unknown const expr type!" << std::endl;
                std::exit(1);
                break;
        }
    }

    return false;
}
