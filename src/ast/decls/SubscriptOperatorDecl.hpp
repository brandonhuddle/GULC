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
#ifndef GULC_SUBSCRIPTOPERATORDECL_HPP
#define GULC_SUBSCRIPTOPERATORDECL_HPP

#include <ast/Decl.hpp>
#include <ast/Type.hpp>
#include <ast/DeclModifiers.hpp>
#include <llvm/Support/Casting.h>
#include "SubscriptOperatorGetDecl.hpp"
#include "SubscriptOperatorSetDecl.hpp"

namespace gulc {
    class SubscriptOperatorDecl : public Decl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::SubscriptOperator; }

        // The type that will be used for the generic `get`/`set`
        Type* type;

        SubscriptOperatorDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                              bool isConstExpr, Identifier identifier, std::vector<ParameterDecl*> parameters, Type* type,
                              TextPosition startPosition, TextPosition endPosition, DeclModifiers declModifiers,
                              std::vector<SubscriptOperatorGetDecl*> getters, SubscriptOperatorSetDecl* setter)
                : Decl(Decl::Kind::SubscriptOperator, sourceFileID, std::move(attributes), visibility,
                       isConstExpr, std::move(identifier), declModifiers),
                  type(type), _parameters(std::move(parameters)), _startPosition(startPosition),
                  _endPosition(endPosition), _getters(std::move(getters)), _setter(setter) {
            for (SubscriptOperatorGetDecl* getter : _getters) {
                getter->container = this;
            }

            if (_setter != nullptr) {
                _setter->container = this;
            }
        }

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        std::vector<ParameterDecl*>& parameters() { return _parameters; }
        std::vector<ParameterDecl*> const& parameters() const { return _parameters; }

        std::vector<SubscriptOperatorGetDecl*>& getters() { return _getters; }
        std::vector<SubscriptOperatorGetDecl*> const& getters() const { return _getters; }
        SubscriptOperatorSetDecl* setter() { return _setter; }
        SubscriptOperatorSetDecl const* setter() const { return _setter; }
        bool hasSetter() const { return _setter != nullptr; }

        Decl* deepCopy() const override {
            std::vector<Attr*> copiedAttributes;
            copiedAttributes.reserve(_attributes.size());
            std::vector<ParameterDecl*> copiedParameters;
            copiedParameters.reserve(_parameters.size());
            std::vector<SubscriptOperatorGetDecl*> copiedGetters;
            copiedGetters.reserve(_getters.size());
            SubscriptOperatorSetDecl* copiedSetter = nullptr;

            for (Attr* attribute : _attributes) {
                copiedAttributes.push_back(attribute->deepCopy());
            }

            for (ParameterDecl* parameter : _parameters) {
                copiedParameters.push_back(llvm::dyn_cast<ParameterDecl>(parameter->deepCopy()));
            }

            for (SubscriptOperatorGetDecl* getter : _getters) {
                copiedGetters.push_back(llvm::dyn_cast<SubscriptOperatorGetDecl>(getter->deepCopy()));
            }

            if (_setter != nullptr) {
                copiedSetter = llvm::dyn_cast<SubscriptOperatorSetDecl>(_setter->deepCopy());
            }

            auto result = new SubscriptOperatorDecl(_sourceFileID, copiedAttributes, _declVisibility, _isConstExpr,
                                                    _identifier, copiedParameters,
                                                    type->deepCopy(), _startPosition, _endPosition,
                                                    _declModifiers, copiedGetters, copiedSetter);
            result->container = container;
            result->containedInTemplate = containedInTemplate;
            result->originalDecl = (originalDecl == nullptr ? this : originalDecl);
            return result;
        }

        ~SubscriptOperatorDecl() override {
            for (ParameterDecl* parameter : _parameters) {
                delete parameter;
            }

            delete type;
            delete _setter;

            for (SubscriptOperatorGetDecl* getter : _getters) {
                delete getter;
            }
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;
        std::vector<ParameterDecl*> _parameters;
        std::vector<SubscriptOperatorGetDecl*> _getters;
        SubscriptOperatorSetDecl* _setter;

    };
}

#endif //GULC_SUBSCRIPTOPERATORDECL_HPP
