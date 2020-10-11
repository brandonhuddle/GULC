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
#ifndef GULC_TYPEALIASDECL_HPP
#define GULC_TYPEALIASDECL_HPP

#include <ast/Decl.hpp>
#include <ast/Type.hpp>
#include <llvm/Support/Casting.h>
#include <ast/Expr.hpp>
#include <ast/exprs/TypeExpr.hpp>
#include <iostream>
#include <utilities/TemplateInstHelper.hpp>
#include "TemplateParameterDecl.hpp"

namespace gulc {
    // We will probably only ever support `prefix` but `infix` may be useful at some point. It could be used for
    // implementing syntax similar to `i32 | f32` among other things.
    enum class TypeAliasType {
        Normal,
        Prefix
    };

    class TypeAliasDecl : public Decl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::TypeAlias; }

        // This is the `= x;` part of the declaration.
        Type* typeValue;
        TypeAliasType typeAliasType() const { return _typeAliasType; }
        std::vector<TemplateParameterDecl*>& templateParameters() { return _templateParameters; }
        std::vector<TemplateParameterDecl*> const& templateParameters() const { return _templateParameters; }
        bool hasTemplateParameters() const { return !templateParameters().empty(); }

        TypeAliasDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                      TypeAliasType typeAliasType, Identifier identifier,
                      std::vector<TemplateParameterDecl*> templateParameters, Type* typeValue,
                      TextPosition startPosition, TextPosition endPosition)
                : Decl(Decl::Kind::TypeAlias, sourceFileID, std::move(attributes), visibility, true,
                       std::move(identifier), DeclModifiers::None),
                  typeValue(typeValue), containerTemplateType(),
                  _startPosition(startPosition), _endPosition(endPosition),
                  _typeAliasType(typeAliasType), _templateParameters(std::move(templateParameters)) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Type* getInstantiation(std::vector<Expr*> const& templateArguments) {
            // While checking the parameters are valid we will also create a copy of the parameters
            std::vector<Expr*> copiedTemplateArguments;
            copiedTemplateArguments.reserve(templateArguments.size());

            for (std::size_t i = 0; i < _templateParameters.size(); ++i) {
                if (_templateParameters[i]->templateParameterKind() == TemplateParameterDecl::TemplateParameterKind::Typename) {
                    if (!llvm::isa<TypeExpr>(templateArguments[i])) {
                        std::cerr << "INTERNAL ERROR: `TemplateTraitDecl::getInstantiation` received non-type argument where type was expected!" << std::endl;
                        std::exit(1);
                    }
                } else {
                    // Const
                    if (llvm::isa<TypeExpr>(templateArguments[i])) {
                        std::cerr << "INTERNAL ERROR: `TemplateTraitDecl::getInstantiation` received type argument where const literal was expected!" << std::endl;
                        std::exit(1);
                    }
                }

                copiedTemplateArguments.push_back(templateArguments[i]->deepCopy());
            }

            Type* result = typeValue->deepCopy();

            TemplateInstHelper templateInstHelper;
            templateInstHelper.instantiateType(result, &_templateParameters, &copiedTemplateArguments);

            // TODO: `result` doesn't contain a `TemplatedType` somewhere then `copiedTemplateArguments` will be leaky

            return result;
        }

        Decl* deepCopy() const override {
            std::vector<Attr*> copiedAttributes;
            copiedAttributes.reserve(_attributes.size());
            std::vector<TemplateParameterDecl*> copiedTemplateParameters;
            copiedTemplateParameters.reserve(_templateParameters.size());

            for (Attr* attribute : _attributes) {
                copiedAttributes.push_back(attribute->deepCopy());
            }

            for (TemplateParameterDecl* templateParameter : _templateParameters) {
                copiedTemplateParameters.push_back(llvm::dyn_cast<TemplateParameterDecl>(templateParameter->deepCopy()));
            }

            auto result = new TypeAliasDecl(_sourceFileID, copiedAttributes, _declVisibility, _typeAliasType,
                                            _identifier, copiedTemplateParameters, typeValue->deepCopy(),
                                            _startPosition, _endPosition);
            result->container = container;
            result->containedInTemplate = containedInTemplate;
            result->containerTemplateType = (containerTemplateType == nullptr ? nullptr : containerTemplateType->deepCopy());
            result->originalDecl = (originalDecl == nullptr ? this : originalDecl);
            return result;
        }

        std::string getPrototypeString() const override {
            std::string result = getDeclModifiersString(_declModifiers);

            if (!result.empty()) result += " ";

            result += "typealias";

            return result;
        }

        ~TypeAliasDecl() override {
            delete typeValue;

            for (TemplateParameterDecl* templateParameter : _templateParameters) {
                delete templateParameter;
            }
        }

        // If the type is contained within a template this is a `Type` that will resolve to the container
        // The reason we need this is to make it easier properly resolve types to `Decl`s when they are contained
        // within templates. Without this we would need to manually search the `container` backwards looking for any
        // templates for every single type that resolves to a Decl
        Type* containerTemplateType;

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;
        TypeAliasType _typeAliasType;
        std::vector<TemplateParameterDecl*> _templateParameters;

    };
}

#endif //GULC_TYPEALIASDECL_HPP
