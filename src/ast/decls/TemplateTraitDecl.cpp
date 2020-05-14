#include <utilities/TemplateCopyUtil.hpp>
#include "TemplateTraitDecl.hpp"

gulc::Decl* gulc::TemplateTraitDecl::deepCopy() const {
    std::vector<Attr*> copiedAttributes;
    copiedAttributes.reserve(_attributes.size());
    std::vector<Type*> copiedInheritedTypes;
    copiedInheritedTypes.reserve(_inheritedTypes.size());
    std::vector<Cont*> copiedContracts;
    copiedContracts.reserve(_contracts.size());
    std::vector<Decl*> copiedOwnedMembers;
    copiedOwnedMembers.reserve(_ownedMembers.size());
    std::vector<TemplateParameterDecl*> copiedTemplateParameters;
    copiedTemplateParameters.reserve(_templateParameters.size());
//    std::vector<TemplateTraitInstDecl*> copiedTemplateInstantiations;
//    copiedTemplateInstantiations.reserve(_templateInstantiations.size());

    for (Attr* attribute : _attributes) {
        copiedAttributes.push_back(attribute->deepCopy());
    }

    for (Type* inheritedType : _inheritedTypes) {
        copiedInheritedTypes.push_back(inheritedType->deepCopy());
    }

    for (Cont* contract : _contracts) {
        copiedContracts.push_back(contract->deepCopy());
    }

    for (Decl* ownedMember : _ownedMembers) {
        copiedOwnedMembers.push_back(ownedMember->deepCopy());
    }

    for (TemplateParameterDecl* templateParameter : _templateParameters) {
        copiedTemplateParameters.push_back(llvm::dyn_cast<TemplateParameterDecl>(templateParameter->deepCopy()));
    }

    // TODO: I don't think we should copy instantiations
//    for (TemplateTraitInstDecl* templateTraitInst : _templateInstantiations) {
//        copiedTemplateInstantiations.push_back(llvm::dyn_cast<TemplateTraitInstDecl>(templateTraitInst->deepCopy()));
//    }

    auto result = new TemplateTraitDecl(_sourceFileID, copiedAttributes, _declVisibility, _isConstExpr,
                                        _identifier, _declModifiers,
                                        _startPosition, _endPosition,
                                        copiedInheritedTypes, copiedContracts, copiedOwnedMembers,
                                        copiedTemplateParameters, {});
    result->container = container;
    result->containedInTemplate = containedInTemplate;
    result->containerTemplateType = (containerTemplateType == nullptr ? nullptr : containerTemplateType->deepCopy());
    result->originalDecl = (originalDecl == nullptr ? this : originalDecl);

    // We have to loop through all nested AST Nodes to replace old template parameter references with the new
    // template parameters
    TemplateCopyUtil templateCopyUtil;
    templateCopyUtil.instantiateTemplateTraitCopy(&_templateParameters, result);

    return result;
}

bool gulc::TemplateTraitDecl::getInstantiation(const std::vector<Expr*>& templateArguments,
                                               gulc::TemplateTraitInstDecl** result) {
    // NOTE: We don't account for default parameters here. `templateArguments` MUST have the default values.
    if (templateArguments.size() != _templateParameters.size()) {
        std::cerr << "INTERNAL ERROR[`TemplateTraitDecl::getInstantiation`]: `templateArguments.size()` MUST BE equal to `_templateParameters.size()`!" << std::endl;
        std::exit(1);
    }

    // We don't check if the provided template arguments are valid UNLESS the isn't an existing match
    // the reason for this is if the template arguments are invalid that is an error and it is okay to take
    // slightly longer on failure, it is not okay to take longer on successes (especially when the check is
    // just a waste of time if it already exists)
    for (TemplateTraitInstDecl* templateTraitInst : _templateInstantiations) {
        // NOTE: We don't deal with "isExact" here, there are only matches and non-matches.
        bool isMatch = true;

        for (std::size_t i = 0; i < templateTraitInst->templateArguments().size(); ++i) {
            if (!ConstExprHelper::compareAreSame(templateTraitInst->templateArguments()[i], templateArguments[i])) {
                isMatch = false;
                break;
            }
        }

        if (isMatch) {
            *result = templateTraitInst;
            return false;
        }
    }

    // While checking the arguments are valid we will also create a copy of the arguments
    std::vector<Expr*> copiedTemplateArguments;
    copiedTemplateArguments.reserve(templateArguments.size());

    // Once we've reached this point it means no match was found. Before we do anything else we need to make
    // sure the provided arguments are valid for the provided parameters
    for (std::size_t i = 0; i < _templateParameters.size(); ++i) {
        if (_templateParameters[i]->templateParameterKind() == TemplateParameterDecl::TemplateParameterKind::Typename) {
            if (!llvm::isa<TypeExpr>(templateArguments[i])) {
                std::cerr << "INTERNAL ERROR: `TemplateTraitDecl::getInstantiation` received non-type argument where type was expected!" << std::endl;
                std::exit(1);
            }
        } else {
            // Const
            if (llvm::isa<TypeExpr>(templateArguments[i])) {
                std::cerr << "INTERNAL ERROR: `TemplateTraitDecl::getInstantiation` received type argument where const literal was expected!" << std::endl;
                std::exit(1);
            }
        }

        copiedTemplateArguments.push_back(templateArguments[i]->deepCopy());
    }

    std::vector<Attr*> copiedAttributes;
    copiedAttributes.reserve(_attributes.size());
    std::vector<Type*> copiedInheritedTypes;
    copiedInheritedTypes.reserve(_inheritedTypes.size());
    std::vector<Cont*> copiedContracts;
    copiedContracts.reserve(_contracts.size());
    std::vector<Decl*> copiedOwnedMembers;
    copiedOwnedMembers.reserve(_ownedMembers.size());

    for (Attr* attribute : _attributes) {
        copiedAttributes.push_back(attribute->deepCopy());
    }

    for (Type* inheritedType : _inheritedTypes) {
        copiedInheritedTypes.push_back(inheritedType->deepCopy());
    }

    for (Cont* contract : _contracts) {
        copiedContracts.push_back(contract->deepCopy());
    }

    for (Decl* ownedMember : _ownedMembers) {
        copiedOwnedMembers.push_back(ownedMember->deepCopy());
    }

    // Here we "steal" the `templateArguments` (since we don't have a way to deep copy)
    *result = new TemplateTraitInstDecl(_sourceFileID, copiedAttributes, _declVisibility, _isConstExpr,
                                        _identifier, _declModifiers,
                                        _startPosition, _endPosition,
                                        copiedInheritedTypes, copiedContracts, copiedOwnedMembers,
                                        this, copiedTemplateArguments);

    TemplateInstHelper templateInstHelper;
    templateInstHelper.instantiateTemplateTraitInstDecl((*result)->parentTemplateTrait(),
                                                        *result, false);

    _templateInstantiations.push_back(*result);

    return true;
}
