#ifndef GULC_TYPECOMPAREUTIL_HPP
#define GULC_TYPECOMPAREUTIL_HPP

#include <vector>
#include <ast/decls/TemplateParameterDecl.hpp>
#include <ast/Expr.hpp>
#include <ast/types/TemplateTypenameRefType.hpp>

namespace gulc {
    class TypeCompareUtil {
    public:
        TypeCompareUtil() = default;
        TypeCompareUtil(std::vector<TemplateParameterDecl*> const* templateParameters,
                        std::vector<Expr*> const* templateArguments)
                : _templateParameters(templateParameters), _templateArguments(templateArguments) {}

        bool compareAreSame(Type const* left, Type const* right);
        bool compareAreSameOrInherits(Type const* checkType, Type const* extendsType);

    protected:
        std::vector<TemplateParameterDecl*> const* _templateParameters = nullptr;
        std::vector<Expr*> const* _templateArguments = nullptr;

        Type const* getTemplateTypenameArg(TemplateTypenameRefType* templateTypenameRefType) const;

    };
}

#endif //GULC_TYPECOMPAREUTIL_HPP
