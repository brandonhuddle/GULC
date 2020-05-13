#include <llvm/Support/Casting.h>
#include <ast/types/AliasType.hpp>
#include <ast/types/BuiltInType.hpp>
#include <ast/types/EnumType.hpp>
#include <ast/types/PointerType.hpp>
#include <ast/types/ReferenceType.hpp>
#include <ast/types/StructType.hpp>
#include <ast/types/TraitType.hpp>
#include <ast/types/TemplateStructType.hpp>
#include <ast/types/TemplateTraitType.hpp>
#include "TypeCompareUtil.hpp"

bool gulc::TypeCompareUtil::compareAreSame(const gulc::Type* left, const gulc::Type* right) {
    // TODO: THis might be better as an option? There may be some scenarios where we want to know if the types are the
    //       same WITHOUT qualifiers (or even same without top-level qualifiers)
    if (left->qualifier() != right->qualifier() ||
        left->getTypeKind() != right->getTypeKind()) {
        return false;
    }

    switch (left->getTypeKind()) {
        case Type::Kind::Alias: {
            auto leftAlias = llvm::dyn_cast<AliasType>(left);
            auto rightAlias = llvm::dyn_cast<AliasType>(right);

            return compareAreSame(leftAlias->decl()->typeValue, rightAlias->decl()->typeValue);
        }
        case Type::Kind::BuiltIn: {
            auto leftBuiltIn = llvm::dyn_cast<BuiltInType>(left);
            auto rightBuiltIn = llvm::dyn_cast<BuiltInType>(right);

            // NOTE: `typealias int = i32;` will mean `i32 == int` so they are the same type
            //       `typealias` will be its own type that references a `BuiltInType`
            return leftBuiltIn->name() == rightBuiltIn->name();
        }
        case Type::Kind::Dimension:
            // TODO: Account for dimension types?
            std::cerr << "FATAL ERROR: Dimension types not yet supported!" << std::endl;
            std::exit(1);
        case Type::Kind::Enum: {
            auto leftEnum = llvm::dyn_cast<EnumType>(left);
            auto rightEnum = llvm::dyn_cast<EnumType>(right);

            return leftEnum->decl() == rightEnum->decl();
        }
        case Type::Kind::Pointer: {
            auto leftPointer = llvm::dyn_cast<PointerType>(left);
            auto rightPointer = llvm::dyn_cast<PointerType>(right);

            return compareAreSame(leftPointer->nestedType, rightPointer->nestedType);
        }
        case Type::Kind::Reference: {
            auto leftReference = llvm::dyn_cast<ReferenceType>(left);
            auto rightReference = llvm::dyn_cast<ReferenceType>(right);

            return compareAreSame(leftReference->nestedType, rightReference->nestedType);
        }
        case Type::Kind::Struct: {
            auto leftStruct = llvm::dyn_cast<StructType>(left);
            auto rightStruct = llvm::dyn_cast<StructType>(right);

            return leftStruct->decl() == rightStruct->decl();
        }
        case Type::Kind::Templated: {
            // TODO: How will we account for this? Should we just return false? Fatal Error?
            //       We COULD compare this by checking if the lists of potential Decls are the same and the arguments
            //       are the same BUT this could miss types that are the same and might not be worth the effort
            //       (it would be easier to just enforce this function not being usable with `Templated`)
            std::cerr << "FATAL ERROR: Uninstantiated template types CANNOT be compared!" << std::endl;
            std::exit(1);
        }
        case Type::Kind::TemplateTypenameRef: {
            auto leftTemplateTypenameRef = llvm::dyn_cast<TemplateTypenameRefType>(left);
            auto rightTemplateTypenameRef = llvm::dyn_cast<TemplateTypenameRefType>(right);

            return leftTemplateTypenameRef->refTemplateParameter() == rightTemplateTypenameRef->refTemplateParameter();
        }
        case Type::Kind::Trait: {
            auto leftTrait = llvm::dyn_cast<TraitType>(left);
            auto rightTrait = llvm::dyn_cast<TraitType>(right);

            return leftTrait->decl() == rightTrait->decl();
        }
        case Type::Kind::Unresolved: {
            std::cerr << "FATAL ERROR: Unresolved types cannot be compared!" << std::endl;
            std::exit(1);
        }
        default:
            std::cerr << "FATAL ERROR: Unknown `Type::Kind` found in `gulc::TypeHelper::compareAreSame`!" << std::endl;
            std::exit(1);
    }

    return false;
}

bool gulc::TypeCompareUtil::compareAreSameOrInherits(const gulc::Type* checkType, const gulc::Type* extendsType) {
    if (compareAreSame(checkType, extendsType)) {
        return true;
    }

    // TODO: We MIGHT have the possibility for circular references here...
    switch (checkType->getTypeKind()) {
        case Type::Kind::Struct: {
            auto structType = llvm::dyn_cast<StructType>(checkType);

            for (Type const* inheritedType : structType->decl()->inheritedTypes()) {
                if (compareAreSameOrInherits(inheritedType, extendsType)) {
                    return true;
                }
            }

            break;
        }
        case Type::Kind::Trait: {
            auto traitType = llvm::dyn_cast<TraitType>(checkType);

            for (Type const* inheritedType : traitType->decl()->inheritedTypes()) {
                if (compareAreSameOrInherits(inheritedType, extendsType)) {
                    return true;
                }
            }

            break;
        }
        // TODO: I'm sure there are niche-case scenarios where this won't work but for now we will just go with it
        //       until these situations present themselves
        case Type::Kind::TemplateStruct: {
            auto templateStructType = llvm::dyn_cast<TemplateStructType>(checkType);

            std::vector<TemplateParameterDecl*> const* oldTemplateParameters = _templateParameters;
            std::vector<Expr*> const* oldTemplateArguments = _templateArguments;

            _templateParameters = &templateStructType->decl()->templateParameters();
            _templateArguments = &templateStructType->templateArguments();

            for (Type const* inheritedType : templateStructType->decl()->inheritedTypes()) {
                if (compareAreSameOrInherits(inheritedType, extendsType)) {
                    return true;
                }
            }

            _templateArguments = oldTemplateArguments;
            _templateParameters = oldTemplateParameters;

            break;
        }
        case Type::Kind::TemplateTrait: {
            auto templateTraitType = llvm::dyn_cast<TemplateTraitType>(checkType);

            std::vector<TemplateParameterDecl*> const* oldTemplateParameters = _templateParameters;
            std::vector<Expr*> const* oldTemplateArguments = _templateArguments;

            _templateParameters = &templateTraitType->decl()->templateParameters();
            _templateArguments = &templateTraitType->templateArguments();

            for (Type const* inheritedType : templateTraitType->decl()->inheritedTypes()) {
                if (compareAreSameOrInherits(inheritedType, extendsType)) {
                    return true;
                }
            }

            _templateArguments = oldTemplateArguments;
            _templateParameters = oldTemplateParameters;

            break;
        }
        case Type::Kind::TemplateTypenameRef: {
            auto templateTypenameRefType = llvm::dyn_cast<TemplateTypenameRefType>(checkType);

            if (_templateParameters != nullptr) {
                for (std::size_t i = 0; i < _templateParameters->size(); ++i) {
                    if ((*_templateParameters)[i] == templateTypenameRefType->refTemplateParameter()) {
                        if (!llvm::isa<TypeExpr>((*_templateArguments)[i])) {
                            std::cerr << "[INTERNAL ERROR] expected `TypeExpr`!" << std::endl;
                            std::exit(1);
                        }

                        auto typeExpr = llvm::dyn_cast<TypeExpr>((*_templateArguments)[i]);

                        return compareAreSameOrInherits(typeExpr->type, extendsType);
                    }
                }
            }

            for (Type const* inheritedType : templateTypenameRefType->refTemplateParameter()->inheritedTypes) {
                if (compareAreSameOrInherits(inheritedType, extendsType)) {
                    return true;
                }
            }

            break;
        }
        default:
            break;
    }

    return false;
}

const gulc::Type* gulc::TypeCompareUtil::getTemplateTypenameArg(gulc::TemplateTypenameRefType* templateTypenameRefType) const {
    if (_templateParameters != nullptr) {
        for (std::size_t i = 0; i < _templateParameters->size(); ++i) {
            if ((*_templateParameters)[i] == templateTypenameRefType->refTemplateParameter()) {
                if (!llvm::isa<TypeExpr>((*_templateArguments)[i])) {
                    std::cerr << "[INTERNAL ERROR] expected `TypeExpr`!" << std::endl;
                    std::exit(1);
                }

                auto typeExpr = llvm::dyn_cast<TypeExpr>((*_templateArguments)[i]);

                return typeExpr->type;
            }
        }
    }

    return templateTypenameRefType;
}
