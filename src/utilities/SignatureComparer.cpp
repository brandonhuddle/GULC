#include <ast/types/ReferenceType.hpp>
#include "SignatureComparer.hpp"
#include "TypeHelper.hpp"

using namespace gulc;

SignatureComparer::CompareResult SignatureComparer::compareFunctions(const FunctionDecl* left,
                                                                     const FunctionDecl* right,
                                                                     bool checkSimilar) {
    if (left->isStatic() != right->isStatic() || left->isMutable() != right->isMutable() ||
        left->identifier().name() != right->identifier().name()) {
        return CompareResult::Different;
    }

    // If `checkSimilar` is enabled then we will still check similarity even if the functions have different numbers of
    // parameters
    if (left->parameters().size() != right->parameters().size() && !checkSimilar) {
        // They are only `Different` if `checkSimilar` is false
        return CompareResult::Different;
    }

    std::size_t parameterSize = std::max(left->parameters().size(), right->parameters().size());

    for (std::size_t i = 0; i < parameterSize; ++i) {
        if (i >= left->parameters().size()) {
            if (!checkSimilar) {
                return CompareResult::Different;
            }

            // If the right parameter is optional then the functions are similar
            if (right->parameters()[i]->defaultValue != nullptr) {
                return CompareResult::Similar;
            } else {
                return CompareResult::Different;
            }
        } else if (i >= right->parameters().size()) {
            if (!checkSimilar) {
                return CompareResult::Different;
            }

            // If the left parameter is optional then the functions are similar
            if (left->parameters()[i]->defaultValue != nullptr) {
                return CompareResult::Similar;
            } else {
                return CompareResult::Different;
            }
        } else {
            // If one side is optional and the other isn't then they are different BUT they could still be similar
            // so we only return `Different` if we're not checking for similarities
            if (!checkSimilar && (left->parameters()[i]->defaultValue == nullptr) !=
                                 (right->parameters()[i]->defaultValue == nullptr)) {
                return CompareResult::Different;
            }

            auto leftParam = left->parameters()[i];
            auto rightParam = right->parameters()[i];

            // If the labels aren't the same then the functions aren't the same. ("_" must be for both for that to
            // affect it)
            if (leftParam->argumentLabel().name() != rightParam->argumentLabel().name()) {
                return CompareResult::Different;
            }

            Type* leftType = leftParam->type;
            Type* rightType = rightParam->type;

            // NOTE: `ref int` == `int` are the same BUT `ref mut int` != `int` and `ref mut int` != `ref int`
            //       This is because `func ex(_ param: ref int);` is callable as `ex(12)`
            if (leftType->qualifier() != Type::Qualifier::Mut && llvm::isa<ReferenceType>(leftType)) {
                leftType = llvm::dyn_cast<ReferenceType>(leftType)->nestedType;
            }

            if (rightType->qualifier() != Type::Qualifier::Mut && llvm::isa<ReferenceType>(rightType)) {
                rightType = llvm::dyn_cast<ReferenceType>(rightType)->nestedType;
            }

            if (!TypeHelper::compareAreSame(leftType, rightType)) {
                return CompareResult::Different;
            }
        }
    }

    // If we make it to this point the functions are exactly the same
    return CompareResult::Exact;
}
