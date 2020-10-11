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
#ifndef GULC_TEMPLATETRAITDECL_HPP
#define GULC_TEMPLATETRAITDECL_HPP

#include <ast/decls/TraitDecl.hpp>
#include <llvm/Support/Casting.h>
#include <iostream>
#include <utilities/ConstExprHelper.hpp>
#include <ast/exprs/TypeExpr.hpp>
#include "TemplateParameterDecl.hpp"
#include "TemplateTraitInstDecl.hpp"

namespace gulc {
    class TemplateTraitDecl : public TraitDecl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::TemplateTrait; }

        std::vector<TemplateParameterDecl*>& templateParameters() { return _templateParameters; }
        std::vector<TemplateParameterDecl*> const& templateParameters() const { return _templateParameters; }
        std::vector<TemplateTraitInstDecl*>& templateInstantiations() { return _templateInstantiations; }
        std::vector<TemplateTraitInstDecl*> const& templateInstantiations() const { return _templateInstantiations; }

        TemplateTraitDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                          bool isConstExpr, Identifier identifier, DeclModifiers declModifiers,
                          TextPosition startPosition, TextPosition endPosition,
                          std::vector<Type*> inheritedTypes, std::vector<Cont*> contracts,
                          std::vector<Decl*> ownedMembers, std::vector<TemplateParameterDecl*> templateParameters)
                : TemplateTraitDecl(sourceFileID, std::move(attributes), visibility,
                                    isConstExpr, std::move(identifier), declModifiers, startPosition, endPosition,
                                    std::move(inheritedTypes), std::move(contracts), std::move(ownedMembers),
                                    std::move(templateParameters), {}) {}

        bool getInstantiation(std::vector<Expr*> const& templateArguments, TemplateTraitInstDecl** result);

        Decl* deepCopy() const override;

        std::string getPrototypeString() const override {
            std::string result = getDeclModifiersString(_declModifiers);

            if (!result.empty()) result += " ";

            result += "trait " + _identifier.name() + "<";

            for (std::size_t i = 0; i < _templateParameters.size(); ++i) {
                if (i != 0) result += ", ";

                result += _templateParameters[i]->getPrototypeString();
            }

            result += ">";

            return result;
        }

        ~TemplateTraitDecl() override {
            for (TemplateParameterDecl* templateParameter : _templateParameters) {
                delete templateParameter;
            }

            for (TemplateTraitInstDecl* templateInstantiation : _templateInstantiations) {
                delete templateInstantiation;
            }
        }

        // This is used to allow us to split contract instantiation off into its own function within `DeclInstantiator`
        bool contractsAreInstantiated = false;
        // This is a specialized template instantiation used to validate the template logic.
        TemplateTraitInstDecl* validationInst = nullptr;

    protected:
        std::vector<TemplateParameterDecl*> _templateParameters;
        std::vector<TemplateTraitInstDecl*> _templateInstantiations;

        TemplateTraitDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                           bool isConstExpr, Identifier identifier, DeclModifiers declModifiers,
                           TextPosition startPosition, TextPosition endPosition,
                           std::vector<Type*> inheritedTypes, std::vector<Cont*> contracts,
                           std::vector<Decl*> ownedMembers, std::vector<TemplateParameterDecl*> templateParameters,
                           std::vector<TemplateTraitInstDecl*> templateInstantiations)
                : TraitDecl(Decl::Kind::TemplateTrait, sourceFileID, std::move(attributes), visibility,
                            isConstExpr, std::move(identifier), declModifiers, startPosition, endPosition,
                            std::move(inheritedTypes), std::move(contracts), std::move(ownedMembers)),
                  _templateParameters(std::move(templateParameters)),
                  _templateInstantiations(std::move(templateInstantiations)) {}

    };
}

#endif //GULC_TEMPLATETRAITDECL_HPP
