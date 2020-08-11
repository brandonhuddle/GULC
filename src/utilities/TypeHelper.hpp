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
#ifndef GULC_TYPEHELPER_HPP
#define GULC_TYPEHELPER_HPP

#include <ast/Type.hpp>
#include <vector>
#include <ast/decls/TemplateParameterDecl.hpp>
#include <parsing/ASTFile.hpp>
#include <ast/types/FunctionPointerType.hpp>
#include <ast/decls/FunctionDecl.hpp>

namespace gulc {
    class TypeHelper {
    public:
        static bool resolveType(Type*& unresolvedType, ASTFile const* currentFile,
                                std::vector<NamespaceDecl*>& namespacePrototypes,
                                std::vector<std::vector<TemplateParameterDecl*>*> const& templateParameters,
                                std::vector<gulc::Decl*> const& containingDecls, bool* outIsAmbiguous);
        static bool typeIsConstExpr(Type* resolvedType);

        static FunctionPointerType* getFunctionPointerTypeFromDecl(FunctionDecl* functionDecl);

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
