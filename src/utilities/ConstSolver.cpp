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
#include <llvm/Support/Casting.h>
#include <ast/exprs/TypeExpr.hpp>
#include <ast/types/StructType.hpp>
#include <ast/types/TraitType.hpp>
#include <ast/decls/TraitPrototypeDecl.hpp>
#include <ast/decls/EnumConstDecl.hpp>
#include <ast/types/EnumType.hpp>
#include <ast/exprs/BoolLiteralExpr.hpp>
#include <ast/types/BoolType.hpp>
#include <ast/exprs/SolvedConstExpr.hpp>
#include "ConstSolver.hpp"
#include "TypeCompareUtil.hpp"

// TODO: This is very context specific. We will need to know what visibility levels are accessible to handle this
//       properly
gulc::SolvedConstExpr* gulc::ConstSolver::solveHasExpr(gulc::HasExpr* hasExpr) {
    if (llvm::isa<TypeExpr>(hasExpr->expr)) {
        auto typeExpr = llvm::dyn_cast<TypeExpr>(hasExpr->expr);
        // We preset the `solution` to `false` and then if we find the `trait` we set to `true` and stop all
        // searches.
        bool solution = false;

        // TODO: We'll have to account for extensions at some point. Which can add functions, traits, operators, props,
        //       subscripts, and traits to anything.
        if (llvm::isa<TraitPrototypeDecl>(hasExpr->decl)) {
            auto findTraitType = llvm::dyn_cast<TraitType>(
                    llvm::dyn_cast<TraitPrototypeDecl>(hasExpr->decl)->traitType);
            std::vector<Type*>* searchTypes;

            if (llvm::isa<StructType>(typeExpr->type)) {
                searchTypes = &llvm::dyn_cast<StructType>(typeExpr->type)->decl()->inheritedTypes();
            } else if (llvm::isa<TraitType>(typeExpr->type)) {
                auto checkTrait = llvm::dyn_cast<TraitType>(typeExpr->type);

                // For checking if `has trait` is true, we also return true when the type IS the trait. I.e.:
                //     Addable has Addable == true
                if (checkTrait->decl() == findTraitType->decl()) {
                    solution = true;
                    searchTypes = nullptr;
                } else {
                    searchTypes = &checkTrait->decl()->inheritedTypes();
                }
            } else {
                // TODO: Anything else we should do here?
                searchTypes = nullptr;
            }

            if (searchTypes != nullptr) {
                for (Type* checkType : *searchTypes) {
                    if (llvm::isa<TraitType>(checkType)) {
                        auto potentialTrait = llvm::dyn_cast<TraitType>(checkType);

                        if (potentialTrait->decl() == findTraitType->decl()) {
                            solution = true;
                            break;
                        }
                    }
                }
            }
        } else if (llvm::isa<ConstructorDecl>(hasExpr->decl)) {
            TypeCompareUtil typeCompareUtil;

            if (llvm::isa<StructType>(typeExpr->type)) {
                solution = containsMatchingConstructorDecl(llvm::dyn_cast<StructType>(typeExpr->type)->decl()->constructors(),
                                                           typeCompareUtil,
                                                           llvm::dyn_cast<ConstructorDecl>(hasExpr->decl));
            } else if (llvm::isa<TraitType>(typeExpr->type)) {
                // TODO: Can't `trait` contain `init` for when whatever implements the `trait` should be constructable
                //       that way? Where is the `constructors`?
            }
        } else if (llvm::isa<DestructorDecl>(hasExpr->decl)) {
            auto findDestructor = llvm::dyn_cast<DestructorDecl>(hasExpr->decl);

            if (llvm::isa<StructType>(typeExpr->type)) {
                auto checkStruct = llvm::dyn_cast<StructType>(typeExpr->type)->decl();

                if (checkStruct->destructor != nullptr) {
                    // If `find` is `virtual` then `check` must be too, else we just default to `true` since we found a
                    // valid destructor
                    if (findDestructor->isAnyVirtual()) {
                        solution = checkStruct->destructor->isAnyVirtual();
                    } else {
                        solution = true;
                    }
                }
            }
        } else if (llvm::isa<EnumConstDecl>(hasExpr->decl)) {
            auto findCase = llvm::dyn_cast<EnumConstDecl>(hasExpr->decl);

            if (llvm::isa<EnumType>(typeExpr->type)) {
                auto checkEnum = llvm::dyn_cast<EnumType>(typeExpr->type)->decl();

                for (EnumConstDecl* checkCase : checkEnum->enumConsts()) {
                    if (findCase->identifier().name() == checkCase->identifier().name()) {
                        solution = true;
                        break;
                    }
                }
            }
        } else {
            TypeCompareUtil typeCompareUtil;
            std::vector<Decl*>* searchDecls;

            if (llvm::isa<StructType>(typeExpr->type)) {
                searchDecls = &llvm::dyn_cast<StructType>(typeExpr->type)->decl()->allMembers;
            } else if (llvm::isa<TraitType>(typeExpr->type)) {
                searchDecls = &llvm::dyn_cast<TraitType>(typeExpr->type)->decl()->allMembers;
            } else {
                searchDecls = nullptr;
            }

            if (searchDecls != nullptr) {
                switch (hasExpr->decl->getDeclKind()) {
                    case Decl::Kind::Variable:
                        solution = containsMatchingVariableDecl(*searchDecls, typeCompareUtil,
                                                                llvm::dyn_cast<VariableDecl>(hasExpr->decl));
                        break;
                    case Decl::Kind::Property:
                        solution = containsMatchingPropertyDecl(*searchDecls, typeCompareUtil,
                                                                llvm::dyn_cast<PropertyDecl>(hasExpr->decl));
                        break;
                    case Decl::Kind::SubscriptOperator:
                        solution = containsMatchingSubscriptOperatorDecl(*searchDecls, typeCompareUtil,
                                                                         llvm::dyn_cast<SubscriptOperatorDecl>(hasExpr->decl));
                        break;
                    case Decl::Kind::Function:
                        solution = containsMatchingFunctionDecl(*searchDecls, typeCompareUtil,
                                                                llvm::dyn_cast<FunctionDecl>(hasExpr->decl));
                        break;
                    case Decl::Kind::Operator:
                        solution = containsMatchingOperatorDecl(*searchDecls, typeCompareUtil,
                                                                llvm::dyn_cast<OperatorDecl>(hasExpr->decl));
                        break;
                    case Decl::Kind::CallOperator:
                        solution = containsMatchingCallOperatorDecl(*searchDecls, typeCompareUtil,
                                                                    llvm::dyn_cast<CallOperatorDecl>(hasExpr->decl));
                        break;
                    default:
                        break;
                }
            }
        }

        auto boolResult = new BoolLiteralExpr(hasExpr->startPosition(), hasExpr->endPosition(), solution);
        boolResult->valueType = new BoolType(Type::Qualifier::Immut, {}, {});
        auto constResult = new SolvedConstExpr(hasExpr, boolResult);
        constResult->valueType = hasExpr->valueType->deepCopy();

        return constResult;
    } else {
        return nullptr;
    }
}

bool gulc::ConstSolver::containsMatchingVariableDecl(std::vector<Decl*> const& checkDecls,
                                                     gulc::TypeCompareUtil& typeCompareUtil,
                                                     gulc::VariableDecl* findVariable) {
    for (Decl* checkDecl : checkDecls) {
        if (!llvm::isa<VariableDecl>(checkDecl)) continue;

        auto checkVariable = llvm::dyn_cast<VariableDecl>(checkDecl);

        if (checkVariable->identifier().name() != findVariable->identifier().name()) continue;

        // At this point the identifiers and types are exactly the same, we can end the search.
        if (typeCompareUtil.compareAreSame(checkVariable->type, findVariable->type)) {
            return true;
        } else {
            // There can't be multiple variables of the same name so we can stop searching.
            break;
        }
    }

    return false;
}

bool gulc::ConstSolver::containsMatchingPropertyDecl(std::vector<Decl*> const& checkDecls,
                                                     gulc::TypeCompareUtil& typeCompareUtil,
                                                     gulc::PropertyDecl* findProperty) {
    for (Decl* checkDecl : checkDecls) {
        if (!llvm::isa<PropertyDecl>(checkDecl)) continue;

        auto checkProperty = llvm::dyn_cast<PropertyDecl>(checkDecl);

        if (checkProperty->identifier().name() != findProperty->identifier().name()) continue;

        // There can't be multiple properties of the same name so we stop searching if the types don't match.
        if (!typeCompareUtil.compareAreSame(checkProperty->type, findProperty->type)) break;

        // At this point we only have to verify the property has all property types specified in `findProperty`.
        // I.e. if `findProperty` is `{ get; set; get ref }` then `checkProperty` needs to have `{ get; get ref; set }`
        // NOTE: `checkProperty` can have more than specified but CANNOT have less.
        if (findProperty->hasSetter() && !checkProperty->hasSetter()) break;

        bool hasAllRequiredGetters = true;

        for (PropertyGetDecl* findGetter : findProperty->getters()) {
            bool foundMatchingGetter = false;

            // Search `checkProperty` for a matching getter
            for (PropertyGetDecl* checkGetter : checkProperty->getters()) {
                if (findGetter->isConstExpr() && !checkGetter->isConstExpr()) continue;
                if (findGetter->isAnyVirtual() && !checkGetter->isAnyVirtual()) continue;
                if (findGetter->isMutable() && !checkGetter->isMutable()) continue;
                if (findGetter->getResultType() != checkGetter->getResultType()) continue;

                // TODO: At this point they are the same, right? At least as "same" as we care about?
                // We've found an exact match for this getter.
                foundMatchingGetter = true;
                break;
            }

            if (!foundMatchingGetter) {
                hasAllRequiredGetters = false;
                // No point in continuing the search, this isn't recoverable.
                break;
            }
        }

        // At this point the name matches and the type matches, we only return `true` if all getters are also there
        return hasAllRequiredGetters;
    }

    return false;
}

bool gulc::ConstSolver::containsMatchingSubscriptOperatorDecl(std::vector<Decl*> const& checkDecls,
                                                              gulc::TypeCompareUtil& typeCompareUtil,
                                                              gulc::SubscriptOperatorDecl* findSubscriptOperator) {
    // Subscript operators can be overloaded. As such we have to find the best match before we do the final `get` and
    // `set` check.
    SubscriptOperatorDecl* foundSubscriptOperator = nullptr;
    bool foundIsExact = false;
    bool foundIsAmbiguous = false;

    for (Decl* checkDecl : checkDecls) {
        if (!llvm::isa<SubscriptOperatorDecl>(checkDecl)) continue;

        // We use the `isExact` to determine if we should `continue` or `break` on a failure.
        bool isExact = true;

        if (findSubscriptOperator->isAnyVirtual() && !checkDecl->isAnyVirtual()) continue;
        if (findSubscriptOperator->isStatic() != checkDecl->isStatic()) continue;

        // If the `find` is `mut` then `check` must be too. Else if they don't match then they're not exact
        if (findSubscriptOperator->isMutable() && !checkDecl->isMutable()) continue;
        else if (findSubscriptOperator->isMutable() != checkDecl->isMutable()) isExact = false;

        // If `find` is `const` then `check` must be too. Else if they don't match then they're not exact
        if (findSubscriptOperator->isConstExpr() && !checkDecl->isConstExpr()) continue;
        else if (findSubscriptOperator->isConstExpr() != checkDecl->isConstExpr()) isExact = false;

        auto checkSubscriptOperator = llvm::dyn_cast<SubscriptOperatorDecl>(checkDecl);

        // NOTE: If `find` has more parameters we skip it. `check` can have more as long as all properties past the
        //       length of `find` are optional.
        //       We have to continue searching because `check` can overload at this point.
        if (findSubscriptOperator->parameters().size() > checkSubscriptOperator->parameters().size()) continue;
        if (findSubscriptOperator->parameters().size() < checkSubscriptOperator->parameters().size()) {
            // NOTE: Because all parameters after an optional must also be optional we can safely just check the
            //       parameter at `find.size` for whether it is optional.
            if (checkSubscriptOperator->parameters()[findSubscriptOperator->parameters().size()] == nullptr) {
                // We continue searching, there could be an overloaded subscript operator still.
                continue;
            } else {
                // We set `isExact` to false to specify we could keep searching if there are any failures.
                isExact = false;
            }
        }

        bool paramsAreSame = true;

        for (size_t i = 0; i < findSubscriptOperator->parameters().size(); ++i) {
            if (!typeCompareUtil.compareAreSame(findSubscriptOperator->parameters()[i]->type,
                                                checkSubscriptOperator->parameters()[i]->type) ||
                    findSubscriptOperator->parameters()[i]->argumentLabel().name() !=
                        checkSubscriptOperator->parameters()[i]->argumentLabel().name()) {
                paramsAreSame = false;
                break;
            }
        }

        if (!paramsAreSame) {
            if (isExact) return false;
            else continue;
        }

        // We only set `found` if `found` is null or `foundIsExact` is `false` while our `isExact` is `true`
        if (foundSubscriptOperator == nullptr || (isExact && !foundIsExact)) {
            foundSubscriptOperator = checkSubscriptOperator;
            foundIsExact = isExact;
            foundIsAmbiguous = false;
        } else if (isExact == foundIsExact) {
            foundIsAmbiguous = true;
        }
    }

    // If the found subscript operator is ambiguous we return false, always. It is uncallable.
    if (foundIsAmbiguous) {
        return false;
    }

    if (foundSubscriptOperator != nullptr) {
        // At this point we only have to verify the subscript has all subscript types specified in
        // `findSubscriptOperator`.
        // I.e. if `findSubscriptOperator` is `{ get; set; get ref }` then `checkSubscriptOperator` needs to have
        //      `{ get; get ref; set }`
        // NOTE: `checkSubscriptOperator` can have more than specified but CANNOT have less.
        if (findSubscriptOperator->hasSetter() && !foundSubscriptOperator->hasSetter()) {
            return false;
        }

        bool hasAllRequiredGetters = true;

        for (SubscriptOperatorGetDecl* findGetter : findSubscriptOperator->getters()) {
            bool foundMatchingGetter = false;

            // Search `checkSubscriptOperator` for a matching getter
            for (SubscriptOperatorGetDecl* checkGetter : foundSubscriptOperator->getters()) {
                if (findGetter->isConstExpr() && !checkGetter->isConstExpr()) continue;
                if (findGetter->isAnyVirtual() && !checkGetter->isAnyVirtual()) continue;
                if (findGetter->isMutable() && !checkGetter->isMutable()) continue;
                if (findGetter->getResultType() != checkGetter->getResultType()) continue;

                // TODO: At this point they are the same, right? At least as "same" as we care about?
                // We've found an exact match for this getter.
                foundMatchingGetter = true;
                break;
            }

            if (!foundMatchingGetter) {
                hasAllRequiredGetters = false;
                // No point in continuing the search, this isn't recoverable.
                break;
            }
        }

        // At this point the name matches and the type matches, we only return `true` if all getters are also there
        return hasAllRequiredGetters;
    }

    return false;
}

bool gulc::ConstSolver::containsMatchingFunctionDecl(std::vector<Decl*> const& checkDecls,
                                                     gulc::TypeCompareUtil& typeCompareUtil,
                                                     gulc::FunctionDecl* findFunction) {
    FunctionDecl* foundFunction = nullptr;
    bool foundIsExact = false;
    bool foundIsAmbiguous = false;

    for (Decl* checkDecl : checkDecls) {
        // TODO: Should we consider templates for non-virtual finds? They are technically callable in the same way.
        if (!llvm::isa<FunctionDecl>(checkDecl)) continue;

        // We use the `isExact` to determine if we should `continue` or `break` on a failure.
        bool isExact = true;

        if (findFunction->isAnyVirtual() && !checkDecl->isAnyVirtual()) continue;
        if (findFunction->isStatic() != checkDecl->isStatic()) continue;

        // If the `find` is `mut` then `check` must be too. Else if they don't match then they're not exact
        if (findFunction->isMutable() && !checkDecl->isMutable()) continue;
        else if (findFunction->isMutable() != checkDecl->isMutable()) isExact = false;

        // If `find` is `const` then `check` must be too. Else if they don't match then they're not exact
        if (findFunction->isConstExpr() && !checkDecl->isConstExpr()) continue;
        else if (findFunction->isConstExpr() != checkDecl->isConstExpr()) isExact = false;

        auto checkFunction = llvm::dyn_cast<FunctionDecl>(checkDecl);

        if (findFunction->identifier().name() != checkFunction->identifier().name()) continue;

        // NOTE: If `find` has more parameters we skip it. `check` can have more as long as all properties past the
        //       length of `find` are optional.
        //       We have to continue searching because `check` can overload at this point.
        if (findFunction->parameters().size() > checkFunction->parameters().size()) continue;
        if (findFunction->parameters().size() < checkFunction->parameters().size()) {
            // NOTE: Because all parameters after an optional must also be optional we can safely just check the
            //       parameter at `find.size` for whether it is optional.
            if (checkFunction->parameters()[findFunction->parameters().size()] == nullptr) {
                // We continue searching, there could be an overloaded subscript operator still.
                continue;
            } else {
                // We set `isExact` to false to specify we could keep searching if there are any failures.
                isExact = false;
            }
        }

        bool paramsAreSame = true;

        for (size_t i = 0; i < findFunction->parameters().size(); ++i) {
            if (!typeCompareUtil.compareAreSame(findFunction->parameters()[i]->type,
                                                checkFunction->parameters()[i]->type) ||
                    findFunction->parameters()[i]->argumentLabel().name() !=
                            checkFunction->parameters()[i]->argumentLabel().name()) {
                paramsAreSame = false;
                break;
            }
        }

        if (!paramsAreSame) continue;

        // We only set `found` if `found` is null or `foundIsExact` is `false` while our `isExact` is `true`
        if (foundFunction == nullptr || (isExact && !foundIsExact)) {
            foundFunction = checkFunction;
            foundIsExact = isExact;
            foundIsAmbiguous = false;
        } else if (isExact == foundIsExact) {
            foundIsAmbiguous = true;
        }
    }

    // Ambiguities are failures for `has`, `has` requires that it is callable.
    if (foundIsAmbiguous) {
        return false;
    }

    // At this point as long as we have a `foundFunction` then `has` is `true`
    if (foundFunction != nullptr) {
        return true;
    }

    return false;
}

bool gulc::ConstSolver::containsMatchingOperatorDecl(std::vector<Decl*> const& checkDecls,
                                                     gulc::TypeCompareUtil& typeCompareUtil,
                                                     gulc::OperatorDecl* findOperator) {
    OperatorDecl* foundOperator = nullptr;
    bool foundIsExact = false;
    bool foundIsAmbiguous = false;

    for (Decl* checkDecl : checkDecls) {
        // TODO: Should we consider templates for non-virtual finds? They are technically callable in the same way.
        if (!llvm::isa<OperatorDecl>(checkDecl)) continue;

        // We use the `isExact` to determine if we should `continue` or `break` on a failure.
        bool isExact = true;

        if (findOperator->isAnyVirtual() && !checkDecl->isAnyVirtual()) continue;
        if (findOperator->isStatic() != checkDecl->isStatic()) continue;

        // If the `find` is `mut` then `check` must be too. Else if they don't match then they're not exact
        if (findOperator->isMutable() && !checkDecl->isMutable()) continue;
        else if (findOperator->isMutable() != checkDecl->isMutable()) isExact = false;

        // If `find` is `const` then `check` must be too. Else if they don't match then they're not exact
        if (findOperator->isConstExpr() && !checkDecl->isConstExpr()) continue;
        else if (findOperator->isConstExpr() != checkDecl->isConstExpr()) isExact = false;

        auto checkOperator = llvm::dyn_cast<OperatorDecl>(checkDecl);

        if (findOperator->operatorType() != checkOperator->operatorType()) continue;
        if (findOperator->operatorIdentifier().name() != checkOperator->operatorIdentifier().name()) continue;

        // NOTE: If `find` has more parameters we skip it. `check` can have more as long as all properties past the
        //       length of `find` are optional.
        //       We have to continue searching because `check` can overload at this point.
        if (findOperator->parameters().size() > checkOperator->parameters().size()) continue;
        if (findOperator->parameters().size() < checkOperator->parameters().size()) {
            // NOTE: Because all parameters after an optional must also be optional we can safely just check the
            //       parameter at `find.size` for whether it is optional.
            if (checkOperator->parameters()[findOperator->parameters().size()] == nullptr) {
                // We continue searching, there could be an overloaded subscript operator still.
                continue;
            } else {
                // We set `isExact` to false to specify we could keep searching if there are any failures.
                isExact = false;
            }
        }

        bool paramsAreSame = true;

        for (size_t i = 0; i < findOperator->parameters().size(); ++i) {
            if (!typeCompareUtil.compareAreSame(findOperator->parameters()[i]->type,
                                                checkOperator->parameters()[i]->type) ||
                    findOperator->parameters()[i]->argumentLabel().name() !=
                            checkOperator->parameters()[i]->argumentLabel().name()) {
                paramsAreSame = false;
                break;
            }
        }

        if (!paramsAreSame) continue;

        // We only set `found` if `found` is null or `foundIsExact` is `false` while our `isExact` is `true`
        if (foundOperator == nullptr || (isExact && !foundIsExact)) {
            foundOperator = checkOperator;
            foundIsExact = isExact;
            foundIsAmbiguous = false;
        } else if (isExact == foundIsExact) {
            foundIsAmbiguous = true;
        }
    }

    // Ambiguities are failures for `has`, `has` requires that it is callable.
    if (foundIsAmbiguous) {
        return false;
    }

    // At this point as long as we have a `foundOperator` then `has` is `true`
    if (foundOperator != nullptr) {
        return true;
    }

    return false;
}

bool gulc::ConstSolver::containsMatchingCallOperatorDecl(std::vector<Decl*> const& checkDecls,
                                                         gulc::TypeCompareUtil& typeCompareUtil,
                                                         gulc::CallOperatorDecl* findCallOperator) {
    CallOperatorDecl* foundCallOperator = nullptr;
    bool foundIsExact = false;
    bool foundIsAmbiguous = false;

    for (Decl* checkDecl : checkDecls) {
        // TODO: Should we consider templates for non-virtual finds? They are technically callable in the same way.
        if (!llvm::isa<CallOperatorDecl>(checkDecl)) continue;

        // We use the `isExact` to determine if we should `continue` or `break` on a failure.
        bool isExact = true;

        if (findCallOperator->isAnyVirtual() && !checkDecl->isAnyVirtual()) continue;

        // If the `find` is `mut` then `check` must be too. Else if they don't match then they're not exact
        if (findCallOperator->isMutable() && !checkDecl->isMutable()) continue;
        else if (findCallOperator->isMutable() != checkDecl->isMutable()) isExact = false;

        // If `find` is `const` then `check` must be too. Else if they don't match then they're not exact
        if (findCallOperator->isConstExpr() && !checkDecl->isConstExpr()) continue;
        else if (findCallOperator->isConstExpr() != checkDecl->isConstExpr()) isExact = false;

        auto checkCallOperator = llvm::dyn_cast<CallOperatorDecl>(checkDecl);

        // NOTE: If `find` has more parameters we skip it. `check` can have more as long as all properties past the
        //       length of `find` are optional.
        //       We have to continue searching because `check` can overload at this point.
        if (findCallOperator->parameters().size() > checkCallOperator->parameters().size()) continue;
        if (findCallOperator->parameters().size() < checkCallOperator->parameters().size()) {
            // NOTE: Because all parameters after an optional must also be optional we can safely just check the
            //       parameter at `find.size` for whether it is optional.
            if (checkCallOperator->parameters()[findCallOperator->parameters().size()] == nullptr) {
                // We continue searching, there could be an overloaded subscript operator still.
                continue;
            } else {
                // We set `isExact` to false to specify we could keep searching if there are any failures.
                isExact = false;
            }
        }

        bool paramsAreSame = true;

        for (size_t i = 0; i < findCallOperator->parameters().size(); ++i) {
            if (!typeCompareUtil.compareAreSame(findCallOperator->parameters()[i]->type,
                                                checkCallOperator->parameters()[i]->type) ||
                    findCallOperator->parameters()[i]->argumentLabel().name() !=
                            checkCallOperator->parameters()[i]->argumentLabel().name()) {
                paramsAreSame = false;
                break;
            }
        }

        if (!paramsAreSame) continue;

        // We only set `found` if `found` is null or `foundIsExact` is `false` while our `isExact` is `true`
        if (foundCallOperator == nullptr || (isExact && !foundIsExact)) {
            foundCallOperator = checkCallOperator;
            foundIsExact = isExact;
            foundIsAmbiguous = false;
        } else if (isExact == foundIsExact) {
            foundIsAmbiguous = true;
        }
    }

    // Ambiguities are failures for `has`, `has` requires that it is callable.
    if (foundIsAmbiguous) {
        return false;
    }

    // At this point as long as we have a `foundCallOperator` then `has` is `true`
    if (foundCallOperator != nullptr) {
        return true;
    }

    return false;
}

bool gulc::ConstSolver::containsMatchingConstructorDecl(std::vector<ConstructorDecl*> const& checkConstructors,
                                                        gulc::TypeCompareUtil& typeCompareUtil,
                                                        gulc::ConstructorDecl* findConstructor) {
    ConstructorDecl* foundConstructor = nullptr;
    bool foundIsExact = false;
    bool foundIsAmbiguous = false;

    for (ConstructorDecl* checkConstructor : checkConstructors) {
        if (findConstructor->constructorType() != checkConstructor->constructorType()) continue;

        // We use the `isExact` to determine if we should `continue` or `break` on a failure.
        bool isExact = true;

        // If `find` is `const` then `check` must be too. Else if they don't match then they're not exact
        if (findConstructor->isConstExpr() && !checkConstructor->isConstExpr()) continue;
        else if (findConstructor->isConstExpr() != checkConstructor->isConstExpr()) isExact = false;

        // NOTE: If `find` has more parameters we skip it. `check` can have more as long as all properties past the
        //       length of `find` are optional.
        //       We have to continue searching because `check` can overload at this point.
        if (findConstructor->parameters().size() > checkConstructor->parameters().size()) continue;
        if (findConstructor->parameters().size() < checkConstructor->parameters().size()) {
            // NOTE: Because all parameters after an optional must also be optional we can safely just check the
            //       parameter at `find.size` for whether it is optional.
            if (checkConstructor->parameters()[findConstructor->parameters().size()] == nullptr) {
                // We continue searching, there could be an overloaded subscript operator still.
                continue;
            } else {
                // We set `isExact` to false to specify we could keep searching if there are any failures.
                isExact = false;
            }
        }

        bool paramsAreSame = true;

        for (size_t i = 0; i < findConstructor->parameters().size(); ++i) {
            if (!typeCompareUtil.compareAreSame(findConstructor->parameters()[i]->type,
                                                checkConstructor->parameters()[i]->type) ||
                    findConstructor->parameters()[i]->argumentLabel().name() !=
                            checkConstructor->parameters()[i]->argumentLabel().name()) {
                paramsAreSame = false;
                break;
            }
        }

        if (!paramsAreSame) continue;

        // We only set `found` if `found` is null or `foundIsExact` is `false` while our `isExact` is `true`
        if (foundConstructor == nullptr || (isExact && !foundIsExact)) {
            foundConstructor = checkConstructor;
            foundIsExact = isExact;
            foundIsAmbiguous = false;
        } else if (isExact == foundIsExact) {
            foundIsAmbiguous = true;
        }
    }

    // Ambiguities are failures for `has`, `has` requires that it is callable.
    if (foundIsAmbiguous) {
        return false;
    }

    // At this point as long as we have a `foundConstructor` then `has` is `true`
    if (foundConstructor != nullptr) {
        return true;
    }

    return false;
}
