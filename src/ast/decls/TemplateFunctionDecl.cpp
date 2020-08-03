#include <utilities/TemplateCopyUtil.hpp>
#include "TemplateFunctionDecl.hpp"

gulc::Decl* gulc::TemplateFunctionDecl::deepCopy() const  {
    std::vector<Attr*> copiedAttributes;
    copiedAttributes.reserve(_attributes.size());
    std::vector<ParameterDecl*> copiedParameters;
    copiedParameters.reserve(_parameters.size());
    std::vector<Cont*> copiedContracts;
    copiedContracts.reserve(_contracts.size());
    std::vector<TemplateParameterDecl*> copiedTemplateParameters;
    copiedTemplateParameters.reserve(_templateParameters.size());

    for (Attr* attribute : _attributes) {
        copiedAttributes.push_back(attribute->deepCopy());
    }

    for (ParameterDecl* parameter : _parameters) {
        copiedParameters.push_back(llvm::dyn_cast<ParameterDecl>(parameter->deepCopy()));
    }

    for (Cont* contract : _contracts) {
        copiedContracts.push_back(contract->deepCopy());
    }

    for (TemplateParameterDecl* templateParameter : _templateParameters) {
        copiedTemplateParameters.push_back(llvm::dyn_cast<TemplateParameterDecl>(templateParameter->deepCopy()));
    }

    auto result = new TemplateFunctionDecl(_sourceFileID, copiedAttributes, _declVisibility, _isConstExpr,
                                           _identifier, _declModifiers, copiedParameters,
                                           returnType->deepCopy(), copiedContracts,
                                           llvm::dyn_cast<CompoundStmt>(_body->deepCopy()),
                                           _startPosition, _endPosition, copiedTemplateParameters);
    result->container = container;
    result->containedInTemplate = containedInTemplate;
    result->originalDecl = (originalDecl == nullptr ? this : originalDecl);

    // We have to loop through all nested AST Nodes to replace old template parameter references with the new
    // template parameters
    TemplateCopyUtil templateCopyUtil;
    templateCopyUtil.instantiateTemplateFunctionCopy(&_templateParameters, result);

    return result;
}

bool gulc::TemplateFunctionDecl::getInstantiation(std::vector<Expr*> const& templateArguments,
                                                  gulc::TemplateFunctionInstDecl** result) {
    // NOTE: We don't account for default parameters here. `templateArguments` MUST have the default values.
    if (templateArguments.size() != _templateParameters.size()) {
        std::cerr << "INTERNAL ERROR[`TemplateFunctionDecl::getInstantiation`]: `templateArguments.size()` MUST BE equal to `_templateParameters.size()`!" << std::endl;
        std::exit(1);
    }

    // We don't check if the provided template parameters are valid UNLESS the isn't an existing match
    // the reason for this is if the template parameters are invalid that is an error and it is okay to take
    // slightly longer on failure, it is not okay to take longer on successes (especially when the check is
    // just a waste of time if it already exists)
    for (TemplateFunctionInstDecl* templateFunctionInst : _templateInstantiations) {
        // NOTE: We don't deal with "isExact" here, there are only matches and non-matches.
        bool isMatch = true;

        for (std::size_t i = 0; i < templateFunctionInst->templateArguments().size(); ++i) {
            if (!ConstExprHelper::compareAreSame(templateFunctionInst->templateArguments()[i], templateArguments[i])) {
                isMatch = false;
                break;
            }
        }

        if (isMatch) {
            *result = templateFunctionInst;
            return false;
        }
    }

    // While checking the parameters are valid we will also create a copy of the parameters
    std::vector<Expr*> copiedTemplateArguments;
    copiedTemplateArguments.reserve(templateArguments.size());

    // Once we've reached this point it means no match was found. Before we do anything else we need to make
    // sure the provided parameters are valid for the provided parameters
    for (std::size_t i = 0; i < _templateParameters.size(); ++i) {
        if (_templateParameters[i]->templateParameterKind() == TemplateParameterDecl::TemplateParameterKind::Typename) {
            if (!llvm::isa<TypeExpr>(templateArguments[i])) {
                std::cerr << "INTERNAL ERROR: `TemplateFunctionDecl::getInstantiation` received non-type argument where type was expected!" << std::endl;
                std::exit(1);
            }
        } else {
            // Const
            if (llvm::isa<TypeExpr>(templateArguments[i])) {
                std::cerr << "INTERNAL ERROR: `TemplateFunctionDecl::getInstantiation` received type argument where const literal was expected!" << std::endl;
                std::exit(1);
            }
        }

        copiedTemplateArguments.push_back(templateArguments[i]->deepCopy());
    }

    std::vector<Attr*> copiedAttributes;
    copiedAttributes.reserve(_attributes.size());
    std::vector<ParameterDecl*> copiedParameters;
    copiedParameters.reserve(_parameters.size());
    std::vector<Cont*> copiedContracts;
    copiedContracts.reserve(_contracts.size());
    std::vector<TemplateParameterDecl*> copiedTemplateParameters;
    copiedTemplateParameters.reserve(_templateParameters.size());

    for (Attr* attribute : _attributes) {
        copiedAttributes.push_back(attribute->deepCopy());
    }

    for (ParameterDecl* parameter : _parameters) {
        copiedParameters.push_back(llvm::dyn_cast<ParameterDecl>(parameter->deepCopy()));
    }

    for (Cont* contract : _contracts) {
        copiedContracts.push_back(contract->deepCopy());
    }

    *result = new TemplateFunctionInstDecl(_sourceFileID, copiedAttributes, _declVisibility, _isConstExpr,
                                           _identifier, _declModifiers, copiedParameters,
                                           returnType->deepCopy(), copiedContracts,
                                           llvm::dyn_cast<CompoundStmt>(_body->deepCopy()),
                                           _startPosition, _endPosition,
                                           this, copiedTemplateArguments);
    (*result)->container = container;
    (*result)->containedInTemplate = containedInTemplate;
    // TODO: Should we be setting this?
    (*result)->originalDecl = (originalDecl == nullptr ? this : originalDecl);

    TemplateInstHelper templateInstHelper;
    templateInstHelper.instantiateTemplateFunctionInstDecl((*result)->parentTemplateStruct(), *result);

    _templateInstantiations.push_back(*result);

    return false;
}
