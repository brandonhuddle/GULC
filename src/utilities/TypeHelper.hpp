#ifndef GULC_TYPEHELPER_HPP
#define GULC_TYPEHELPER_HPP

#include <ast/Type.hpp>
#include <vector>
#include <ast/decls/TemplateParameterDecl.hpp>
#include <parsing/ASTFile.hpp>

namespace gulc {
    class TypeHelper {
    public:
        /// NOTE: If `resolveTemplates` is false we will always return `true`
        static bool resolveType(Type*& unresolvedType, ASTFile const* currentFile,
                                std::vector<std::vector<TemplateParameterDecl*>*> const& templateParameters,
                                std::vector<gulc::Decl*> const& containingDecls);
        static bool typeIsConstExpr(Type* resolvedType);

    };
}

#endif //GULC_TYPEHELPER_HPP
