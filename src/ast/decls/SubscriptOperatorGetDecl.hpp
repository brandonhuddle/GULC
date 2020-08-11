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
#ifndef GULC_SUBSCRIPTOPERATORGETDECL_HPP
#define GULC_SUBSCRIPTOPERATORGETDECL_HPP

#include "FunctionDecl.hpp"

namespace gulc {
    class SubscriptOperatorGetDecl : public FunctionDecl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::SubscriptOperatorGet; }

        enum class GetResult {
            Normal, // `get`
            Ref, // `get ref`
            RefMut // `get ref mut`
        };

        SubscriptOperatorGetDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                                 bool isConstExpr, Identifier identifier, DeclModifiers declModifiers,
                                 Type* returnType, std::vector<Cont*> contracts, CompoundStmt* body,
                                 TextPosition startPosition, TextPosition endPosition, GetResult getResult)
                : FunctionDecl(Decl::Kind::SubscriptOperatorGet, sourceFileID, std::move(attributes),
                               visibility, isConstExpr, std::move(identifier), declModifiers, {}, returnType,
                               std::move(contracts), body, startPosition, endPosition),
                  _getResult(getResult) {}

        GetResult getResultType() const { return _getResult; }

        Decl* deepCopy() const override {
            std::vector<Attr*> copiedAttributes;
            copiedAttributes.reserve(_attributes.size());
            std::vector<Cont*> copiedContracts;
            copiedContracts.reserve(_contracts.size());

            for (Attr* attribute : _attributes) {
                copiedAttributes.push_back(attribute->deepCopy());
            }

            for (Cont* contract : _contracts) {
                copiedContracts.push_back(contract->deepCopy());
            }

            auto result = new SubscriptOperatorGetDecl(_sourceFileID, copiedAttributes, _declVisibility, _isConstExpr,
                                                       _identifier, _declModifiers,
                                                       returnType->deepCopy(), copiedContracts,
                                                       llvm::dyn_cast<CompoundStmt>(_body->deepCopy()),
                                                       _startPosition, _endPosition, _getResult);
            result->container = container;
            result->containedInTemplate = containedInTemplate;
            result->originalDecl = (originalDecl == nullptr ? this : originalDecl);
            return result;
        }

    protected:
        GetResult _getResult;

    };
}

#endif //GULC_SUBSCRIPTOPERATORGETDECL_HPP
