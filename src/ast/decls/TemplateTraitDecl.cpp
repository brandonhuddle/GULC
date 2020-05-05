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

    // We have to loop through all nested AST Nodes to replace old template parameter references with the new
    // template parameters
    TemplateCopyUtil templateCopyUtil;
    templateCopyUtil.instantiateTemplateTraitCopy(&_templateParameters, result);

    return result;
}
