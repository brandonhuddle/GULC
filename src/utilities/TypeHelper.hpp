#ifndef GULC_TYPEHELPER_HPP
#define GULC_TYPEHELPER_HPP

#include <ast/Type.hpp>
#include <vector>
#include <ast/decls/TemplateParameterDecl.hpp>
#include <parsing/ASTFile.hpp>

namespace gulc {
    class TypeHelper {
    public:
        static bool resolveType(Type*& unresolvedType, ASTFile const* currentFile,
                                std::vector<NamespaceDecl*>& namespacePrototypes,
                                std::vector<std::vector<TemplateParameterDecl*>*> const& templateParameters,
                                std::vector<gulc::Decl*> const& containingDecls, bool* outIsAmbiguous);
        static bool typeIsConstExpr(Type* resolvedType);

        // TODO: Note that `type` must be `UnresolvedType`
        static bool resolveTypeWithinDecl(Type*& type, Decl* container);
        // `reresolve` - resolve again...
        static bool reresolveDependentWithinDecl(Type*& type, Decl* container);

    protected:
        static bool resolveTypeToDecl(Type*& type, Decl* checkDecl, std::string const& checkName,
                                      bool templated, std::vector<Decl*>& potentialTemplates,
                                      bool searchMembers, bool resolveToCheckDecl, Decl** outFoundDecl = nullptr);
        static bool resolveNamespacePathToDecl(std::vector<Identifier> const& namespacePath, std::size_t pathIndex,
                                               std::vector<Decl*> const& declList, Decl** resultDecl);
        static bool checkImportForAmbiguity(ImportDecl* importDecl, std::string const& checkName, Decl* skipDecl);

    };
}

#endif //GULC_TYPEHELPER_HPP
