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
#ifndef GULC_BASICDECLVALIDATOR_HPP
#define GULC_BASICDECLVALIDATOR_HPP

#include <vector>
#include <string>
#include <ast/decls/NamespaceDecl.hpp>
#include <parsing/ASTFile.hpp>
#include <ast/decls/TemplateParameterDecl.hpp>
#include <ast/decls/FunctionDecl.hpp>
#include <ast/decls/PropertyDecl.hpp>
#include <ast/decls/StructDecl.hpp>
#include <ast/decls/SubscriptOperatorDecl.hpp>
#include <ast/decls/TemplateFunctionDecl.hpp>
#include <ast/decls/TemplateStructDecl.hpp>
#include <ast/decls/TemplateTraitDecl.hpp>
#include <ast/decls/TypeAliasDecl.hpp>
#include <ast/decls/OperatorDecl.hpp>
#include <ast/decls/CallOperatorDecl.hpp>
#include <ast/decls/EnumDecl.hpp>
#include <ast/decls/TypeSuffixDecl.hpp>
#include <ast/decls/ExtensionDecl.hpp>

namespace gulc {
    class BasicDeclValidator {
    public:
        BasicDeclValidator(std::vector<std::string> const& filePaths, std::vector<NamespaceDecl*>& namespacePrototypes)
                : _filePaths(filePaths), _namespacePrototypes(namespacePrototypes), _currentFile(),
                  _currentContainerDecl(), _currentContainerTemplateType() {}

        void processFiles(std::vector<ASTFile>& files);

    private:
        std::vector<std::string> const& _filePaths;
        std::vector<NamespaceDecl*>& _namespacePrototypes;
        ASTFile* _currentFile;
        // List of template parameter lists. Used to account for multiple levels of templates:
        //     struct Example1<T> { struct Example2<S> { func example3<U>(); } }
        // The above would result in three separate lists.
        std::vector<std::vector<TemplateParameterDecl*>*> _templateParameters;
        // The container of the currently processing Decl.
        gulc::Decl* _currentContainerDecl;
        // This is null unless we're processing decls within a template.
        Type* _currentContainerTemplateType;

        void printError(std::string const& message, TextPosition startPosition, TextPosition endPosition) const;
        void printWarning(std::string const& message, TextPosition startPosition, TextPosition endPosition) const;

        void validateImports(std::vector<ImportDecl*>& imports) const;
        bool resolveImport(std::vector<Identifier> const& importPath, std::size_t pathIndex,
                           NamespaceDecl* checkNamespace, NamespaceDecl** foundNamespace) const;

        Decl* getRedefinition(std::string const& findName, Decl* skipDecl) const;

        void validateParameters(std::vector<ParameterDecl*> const& parameters) const;
        void validateTemplateParameters(std::vector<TemplateParameterDecl*> const& templateParameters) const;

        void validateDecl(Decl* decl, bool isGlobal = true);
        void validateCallOperatorDecl(CallOperatorDecl* callOperatorDecl) const;
        void validateConstructorDecl(ConstructorDecl* constructorDecl) const;
        void validateDestructorDecl(DestructorDecl* destructorDecl) const;
        void validateEnumDecl(EnumDecl* enumDecl);
        void validateExtensionDecl(ExtensionDecl* extensionDecl);
        void validateFunctionDecl(FunctionDecl* functionDecl) const;
        void validateNamespaceDecl(NamespaceDecl* namespaceDecl);
        void validateOperatorDecl(OperatorDecl* operatorDecl) const;
        void validatePropertyDecl(PropertyDecl* propertyDecl, bool isGlobal);
        void validatePropertyGetDecl(PropertyGetDecl* propertyGetDecl);
        void validatePropertySetDecl(PropertySetDecl* propertySetDecl);
        void validateStructDecl(StructDecl* structDecl, bool checkForRedefinition, bool setTemplateContainer);
        void validateSubscriptOperatorDecl(SubscriptOperatorDecl* subscriptOperatorDecl, bool isGlobal);
        void validateSubscriptOperatorGetDecl(SubscriptOperatorGetDecl* subscriptOperatorGetDecl);
        void validateSubscriptOperatorSetDecl(SubscriptOperatorSetDecl* subscriptOperatorSetDecl);
        void validateTemplateFunctionDecl(TemplateFunctionDecl* templateFunctionDecl);
        void validateTemplateStructDecl(TemplateStructDecl* templateStructDecl);
        void validateTemplateTraitDecl(TemplateTraitDecl* templateTraitDecl);
        void validateTraitDecl(TraitDecl* traitDecl, bool checkForRedefinition, bool setTemplateContainer);
        void validateTypeAliasDecl(TypeAliasDecl* typeAliasDecl);
        void validateTypeSuffixDecl(TypeSuffixDecl* typeSuffixDecl, bool isGlobal);
        void validateVariableDecl(VariableDecl* variableDecl, bool isGlobal) const;

    };
}

#endif //GULC_BASICDECLVALIDATOR_HPP
