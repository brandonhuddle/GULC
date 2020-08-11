/*
 * Copyright (C) 2020 Brandon Huddle
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <ast/types/FunctionPointerType.hpp>
#include <ast/types/StructType.hpp>
#include <ast/types/TraitType.hpp>
#include <ast/decls/CallOperatorDecl.hpp>
#include <ast/types/ReferenceType.hpp>
#include "FunctorUtil.hpp"

gulc::SignatureComparer::ArgMatchResult gulc::FunctorUtil::checkValidFunctorCall(
        gulc::Type* checkType,
        std::vector<LabeledArgumentExpr*> const& arguments,
        Decl** outFoundDecl) {
    std::vector<Decl*>* searchDecls = nullptr;

    if (llvm::isa<ReferenceType>(checkType)) {
        checkType = llvm::dyn_cast<ReferenceType>(checkType)->nestedType;
    }

    if (llvm::isa<FunctionPointerType>(checkType)) {
        auto checkFuncPointer = llvm::dyn_cast<FunctionPointerType>(checkType);

        return SignatureComparer::compareArgumentsToParameters(checkFuncPointer->parameters, arguments);
    } else if (llvm::isa<StructType>(checkType)) {
        // NOTE: This does NOT handle constructors. This function only handles types that are instanced, not
        //       instancing types
        auto checkStruct = llvm::dyn_cast<StructType>(checkType)->decl();

        searchDecls = &checkStruct->allMembers;
    } else if (llvm::isa<TraitType>(checkType)) {
        auto checkTrait = llvm::dyn_cast<TraitType>(checkType)->decl();

        searchDecls = &checkTrait->allMembers;
    }

    if (searchDecls != nullptr) {
        Decl* foundDecl = nullptr;
        bool isExact = false;

        for (Decl* checkDecl : *searchDecls) {
            // Skip static and non-call operator members.
            if (checkDecl->isStatic() || !llvm::isa<CallOperatorDecl>(checkDecl)) continue;

            auto checkCallOperator = llvm::dyn_cast<CallOperatorDecl>(checkDecl);

            SignatureComparer::ArgMatchResult argMatchResult =
                    SignatureComparer::compareArgumentsToParameters(checkCallOperator->parameters(), arguments);

            if (argMatchResult != SignatureComparer::ArgMatchResult::Fail) {
                if (foundDecl == nullptr) {
                    foundDecl = checkDecl;
                    isExact = argMatchResult == SignatureComparer::ArgMatchResult::Match;
                } else {
                    // If there are multiple exact matches we return a fail
                    // TODO: We might want to make our own type that can return `ambiguous`
                    if (isExact) {
                        return SignatureComparer::ArgMatchResult::Fail;
                    }
                }
            }
        }

        if (foundDecl != nullptr) {
            *outFoundDecl = foundDecl;

            if (isExact) {
                return SignatureComparer::ArgMatchResult::Match;
            } else {
                return SignatureComparer::ArgMatchResult::Castable;
            }
        }
    }

    return gulc::SignatureComparer::Fail;
}
