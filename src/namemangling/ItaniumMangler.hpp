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
#ifndef GULC_ITANIUMMANGLER_HPP
#define GULC_ITANIUMMANGLER_HPP

#include "ManglerBase.hpp"

namespace gulc {
    // I try my best to keep the Ghoul ABI as close to the C++ Itanium ABI spec but there are bound to be areas where
    // the languages differ too much. In situations where the languages can't be a perfect match I will try my best to
    // at least come close to matching the Itanium spec.
    class ItaniumMangler : public ManglerBase {
    public:
        void mangleDecl(EnumDecl* enumDecl) override;
        void mangleDecl(StructDecl* structDecl) override;
        void mangleDecl(TraitDecl* traitDecl) override;
        void mangleDecl(NamespaceDecl* namespaceDecl) override;

        void mangle(FunctionDecl* functionDecl) override;
        void mangle(VariableDecl* variableDecl) override;
        void mangle(NamespaceDecl* namespaceDecl) override;
        void mangle(StructDecl* structDecl) override;
        void mangle(TraitDecl* traitDecl) override;
        void mangle(CallOperatorDecl* callOperatorDecl) override;
        void mangle(PropertyDecl* propertyDecl) override;

    private:
        void mangleDeclEnum(EnumDecl* enumDecl, std::string const& prefix, std::string const& nameSuffix);
        void mangleDeclStruct(StructDecl* structDecl, std::string const& prefix, std::string const& nameSuffix);
        void mangleDeclTrait(TraitDecl* traitDecl, std::string const& prefix, std::string const& nameSuffix);
        void mangleDeclNamespace(NamespaceDecl* namespaceDecl, std::string const& prefix);

        void mangleFunction(FunctionDecl* functionDecl, std::string const& prefix, std::string const& nameSuffix);
        void mangleVariable(VariableDecl* variableDecl, std::string const& prefix, std::string const& nameSuffix);
        void mangleNamespace(NamespaceDecl* namespaceDecl, std::string const& prefix);
        void mangleStruct(StructDecl* structDecl, std::string const& prefix);
        void mangleTrait(TraitDecl* traitDecl, std::string const& prefix);
        void mangleCallOperator(CallOperatorDecl* callOperatorDecl, std::string const& prefix,
                                std::string const& nameSuffix);
        void mangleOperator(OperatorDecl* operatorDecl, std::string const& prefix, std::string const& nameSuffix);
        void mangleProperty(PropertyDecl* propertyDecl, std::string const& prefix, std::string const& nameSuffix);
        void manglePropertyGet(PropertyGetDecl* propertyGetDecl, std::string const& prefix,
                               std::string const& nameSuffix);
        void manglePropertySet(PropertySetDecl* propertySetDecl, std::string const& prefix,
                               std::string const& nameSuffix);
        void mangleSubscript(SubscriptOperatorDecl* subscriptOperatorDecl, std::string const& prefix,
                             std::string const& nameSuffix);
        void mangleSubscriptGet(SubscriptOperatorGetDecl* subscriptOperatorGetDecl, std::string const& prefix,
                                std::string const& nameSuffix);
        void mangleSubscriptSet(SubscriptOperatorSetDecl* subscriptOperatorSetDecl, std::string const& prefix,
                                std::string const& nameSuffix);

        void mangleConstructor(ConstructorDecl* constructorDecl, std::string const& prefix,
                               std::string const& nameSuffix);
        void mangleDestructor(DestructorDecl* destructorDecl, std::string const& prefix, std::string const& nameSuffix);

        std::string unqualifiedName(FunctionDecl* functionDecl);
        std::string unqualifiedName(VariableDecl* variableDecl);

        std::string sourceName(std::string const& s);
        std::string bareFunctionType(std::vector<ParameterDecl*>& params);
        std::string typeName(gulc::Type* type);

        std::string templateArgs(std::vector<TemplateParameterDecl*>& templateParams, std::vector<Expr*>& templateArgs);
        std::string templateArg(Expr const* expr);
        std::string exprPrimary(Expr const* expr);

        std::string operatorName(OperatorType operatorType, std::string const& operatorText);

    };
}

#endif //GULC_ITANIUMMANGLER_HPP
