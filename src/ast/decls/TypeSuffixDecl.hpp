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
#ifndef GULC_TYPESUFFIXDECL_HPP
#define GULC_TYPESUFFIXDECL_HPP

#include "FunctionDecl.hpp"

namespace gulc {
    class TypeSuffixDecl : public FunctionDecl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::TypeSuffix; }

        TypeSuffixDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                       bool isConstExpr, Identifier identifier, DeclModifiers declModifiers,
                       std::vector<ParameterDecl*> parameters, Type* returnType,
                       std::vector<Cont*> contracts, CompoundStmt* body,
                       TextPosition startPosition, TextPosition endPosition)
                : FunctionDecl(Decl::Kind::TypeSuffix, sourceFileID, std::move(attributes), visibility,
                               isConstExpr, std::move(identifier), declModifiers, std::move(parameters), returnType,
                               std::move(contracts), body, startPosition, endPosition) {}

        Decl* deepCopy() const override {
            std::vector<Attr*> copiedAttributes;
            copiedAttributes.reserve(_attributes.size());
            std::vector<ParameterDecl*> copiedParameters;
            copiedParameters.reserve(_parameters.size());
            std::vector<Cont*> copiedContracts;
            copiedContracts.reserve(_contracts.size());
            Type* copiedReturnType = nullptr;

            for (Attr* attribute : _attributes) {
                copiedAttributes.push_back(attribute->deepCopy());
            }

            for (ParameterDecl* parameter : _parameters) {
                copiedParameters.push_back(llvm::dyn_cast<ParameterDecl>(parameter->deepCopy()));
            }

            for (Cont* contract : _contracts) {
                copiedContracts.push_back(contract->deepCopy());
            }

            if (returnType != nullptr) {
                copiedReturnType = returnType->deepCopy();
            }

            auto result = new TypeSuffixDecl(_sourceFileID, copiedAttributes, _declVisibility, _isConstExpr,
                                             _identifier, _declModifiers, copiedParameters, copiedReturnType,
                                             copiedContracts, llvm::dyn_cast<CompoundStmt>(_body->deepCopy()),
                                             _startPosition, _endPosition);
            result->container = container;
            result->containedInTemplate = containedInTemplate;
            result->originalDecl = (originalDecl == nullptr ? this : originalDecl);
            return result;
        }

    };
}

#endif //GULC_TYPESUFFIXDECL_HPP
