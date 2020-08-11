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
#ifndef GULC_TEMPLATESTRUCTDECL_HPP
#define GULC_TEMPLATESTRUCTDECL_HPP

#include <vector>
#include <iostream>
#include <utilities/ConstExprHelper.hpp>
#include <ast/exprs/TypeExpr.hpp>
#include <ast/exprs/ValueLiteralExpr.hpp>
#include "StructDecl.hpp"
#include "TemplateParameterDecl.hpp"
#include "TemplateStructInstDecl.hpp"

namespace gulc {
    class TemplateStructDecl : public StructDecl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::TemplateStruct; }

        std::vector<TemplateParameterDecl*>& templateParameters() { return _templateParameters; }
        std::vector<TemplateParameterDecl*> const& templateParameters() const { return _templateParameters; }
        std::vector<TemplateStructInstDecl*>& templateInstantiations() { return _templateInstantiations; }
        std::vector<TemplateStructInstDecl*> const& templateInstantiations() const { return _templateInstantiations; }

        TemplateStructDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                           bool isConstExpr, Identifier identifier, DeclModifiers declModifiers,
                           TextPosition startPosition, TextPosition endPosition,
                           Kind structKind, std::vector<Type*> inheritedTypes, std::vector<Cont*> contracts,
                           std::vector<Decl*> ownedMembers, std::vector<ConstructorDecl*> constructors,
                           DestructorDecl* destructor, std::vector<TemplateParameterDecl*> templateParameters)
                : TemplateStructDecl(sourceFileID, std::move(attributes), visibility,
                                     isConstExpr, std::move(identifier), declModifiers, startPosition, endPosition,
                                     structKind, std::move(inheritedTypes), std::move(contracts),
                                     std::move(ownedMembers), std::move(constructors), destructor,
                                     std::move(templateParameters), {}) {}

        /**
         * Get an instantiation of the template for the provided template parameters
         *
         * NOTE: The instantiation is NOT guaranteed to be completely constructed.
         * NOTE: `templateArguments` MUST have default values included
         *
         * @param templateArguments - parameters for the template
         * @param result - the found struct instantiation
         *
         * @return true for new TemplateStructInstDecl, false for existing
         */
        bool getInstantiation(std::vector<Expr*> const& templateArguments, TemplateStructInstDecl** result);

        Decl* deepCopy() const override;

        ~TemplateStructDecl() override {
            for (TemplateParameterDecl* templateParameter : _templateParameters) {
                delete templateParameter;
            }

            for (TemplateStructInstDecl* templateInstantiation : _templateInstantiations) {
                delete templateInstantiation;
            }
        }

        // This is used to allow us to split contract instantiation off into its own function within `DeclInstantiator`
        bool contractsAreInstantiated = false;

    protected:
        std::vector<TemplateParameterDecl*> _templateParameters;
        std::vector<TemplateStructInstDecl*> _templateInstantiations;

        TemplateStructDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                           bool isConstExpr, Identifier identifier, DeclModifiers declModifiers,
                           TextPosition startPosition, TextPosition endPosition,
                           Kind structKind, std::vector<Type*> inheritedTypes, std::vector<Cont*> contracts,
                           std::vector<Decl*> ownedMembers, std::vector<ConstructorDecl*> constructors,
                           DestructorDecl* destructor, std::vector<TemplateParameterDecl*> templateParameters,
                           std::vector<TemplateStructInstDecl*> templateInstantiations)
                : StructDecl(Decl::Kind::TemplateStruct, sourceFileID, std::move(attributes), visibility,
                             isConstExpr, std::move(identifier), declModifiers, startPosition, endPosition, structKind,
                             std::move(inheritedTypes), std::move(contracts), std::move(ownedMembers),
                             std::move(constructors), destructor),
                  _templateParameters(std::move(templateParameters)),
                  _templateInstantiations(std::move(templateInstantiations)) {}

    };
}

#endif //GULC_TEMPLATESTRUCTDECL_HPP
