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
#ifndef GULC_TEMPLATEFUNCTIONDECL_HPP
#define GULC_TEMPLATEFUNCTIONDECL_HPP

#include <ast/Decl.hpp>
#include "FunctionDecl.hpp"
#include "TemplateParameterDecl.hpp"
#include "TemplateFunctionInstDecl.hpp"

namespace gulc {
    class TemplateFunctionDecl : public FunctionDecl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::TemplateFunction; }

        std::vector<TemplateParameterDecl*>& templateParameters() { return _templateParameters; }
        std::vector<TemplateParameterDecl*> const& templateParameters() const { return _templateParameters; }
        std::vector<TemplateFunctionInstDecl*>& templateInstantiations() { return _templateInstantiations; }
        std::vector<TemplateFunctionInstDecl*> const& templateInstantiations() const { return _templateInstantiations; }

        TemplateFunctionDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                             bool isConstExpr, Identifier identifier, DeclModifiers declModifiers,
                             std::vector<ParameterDecl*> parameters, Type* returnType,
                             std::vector<Cont*> contracts, CompoundStmt* body,
                             TextPosition startPosition, TextPosition endPosition,
                             std::vector<TemplateParameterDecl*> templateParameters)
                : TemplateFunctionDecl(sourceFileID, std::move(attributes), visibility, isConstExpr,
                                       std::move(identifier), declModifiers, std::move(parameters), returnType,
                                       std::move(contracts), body, startPosition, endPosition,
                                       std::move(templateParameters), {}) {}

        bool getInstantiation(std::vector<Expr*> const& templateArguments, TemplateFunctionInstDecl** result);

        Decl* deepCopy() const override;

        std::string getPrototypeString() const override {
            std::string result = getDeclModifiersString(_declModifiers);

            if (!result.empty()) result += " ";

            result += "func " + _identifier.name() + "<";

            for (std::size_t i = 0; i < _templateParameters.size(); ++i) {
                if (i != 0) result += ", ";

                result += _templateParameters[i]->getPrototypeString();
            }

            result += ">(";

            for (std::size_t i = 0; i < _parameters.size(); ++i) {
                if (i != 0) result += ", ";

                result += _parameters[i]->getPrototypeString();
            }

            return result + ")";
        }

        ~TemplateFunctionDecl() override {
            for (TemplateParameterDecl* templateParameter : _templateParameters) {
                delete templateParameter;
            }
        }

        // This is used to allow us to split contract instantiation off into its own function within `DeclInstantiator`
        bool contractsAreInstantiated = false;
        // This is a specialized template instantiation used to validate the template logic.
        TemplateFunctionInstDecl* validationInst = nullptr;

    protected:
        std::vector<TemplateParameterDecl*> _templateParameters;
        std::vector<TemplateFunctionInstDecl*> _templateInstantiations;

        TemplateFunctionDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                             bool isConstExpr, Identifier identifier, DeclModifiers declModifiers,
                             std::vector<ParameterDecl*> parameters, Type* returnType,
                             std::vector<Cont*> contracts, CompoundStmt* body,
                             TextPosition startPosition, TextPosition endPosition,
                             std::vector<TemplateParameterDecl*> templateParameters,
                             std::vector<TemplateFunctionInstDecl*> templateInstantiations)
                : FunctionDecl(Decl::Kind::TemplateFunction, sourceFileID, std::move(attributes), visibility,
                               isConstExpr, std::move(identifier), declModifiers, std::move(parameters), returnType,
                               std::move(contracts), body, startPosition, endPosition),
                  _templateParameters(std::move(templateParameters)),
                  _templateInstantiations(std::move(templateInstantiations)) {}

    };
}

#endif //GULC_TEMPLATEFUNCTIONDECL_HPP
