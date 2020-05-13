#ifndef GULC_CONTRACTUTIL_HPP
#define GULC_CONTRACTUTIL_HPP

#include <ast/conts/WhereCont.hpp>
#include <string>
#include <vector>
#include <ast/exprs/CheckExtendsTypeExpr.hpp>

namespace gulc {
    class ContractUtil {
    public:
        ContractUtil(std::string const& fileName, std::vector<TemplateParameterDecl*> const* templateParameters,
                     std::vector<Expr*> const* templateArguments)
                : _fileName(fileName), _templateParameters(templateParameters),
                  _templateArguments(templateArguments) {}

        bool checkWhereCont(WhereCont* whereCont);

    protected:
        std::string const& _fileName;
        std::vector<TemplateParameterDecl*> const* _templateParameters;
        std::vector<Expr*> const* _templateArguments;

        void printError(std::string const& message, TextPosition startPosition, TextPosition endPosition) const;
        void printWarning(std::string const& message, TextPosition startPosition, TextPosition endPosition) const;

        Type const* getTemplateTypeArgument(TemplateTypenameRefType const* templateTypenameRefType) const;

        bool checkCheckExtendsTypeExpr(CheckExtendsTypeExpr* checkExtendsTypeExpr) const;

    };
}

#endif //GULC_CONTRACTUTIL_HPP
