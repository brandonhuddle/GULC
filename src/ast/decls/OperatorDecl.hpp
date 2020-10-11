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
#ifndef GULC_OPERATORDECL_HPP
#define GULC_OPERATORDECL_HPP

#include <string>
#include "FunctionDecl.hpp"

namespace gulc {
    enum class OperatorType {
        Unknown,
        Prefix,
        Infix,
        Postfix

    };

    std::string operatorTypeName(OperatorType operatorType);

    class OperatorDecl : public FunctionDecl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::Operator; }

        OperatorDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                     bool isConstExpr, OperatorType operatorType, Identifier operatorIdentifier,
                     DeclModifiers declModifiers, std::vector<ParameterDecl*> parameters, Type* returnType,
                     std::vector<Cont*> contracts, CompoundStmt* body,
                     TextPosition startPosition, TextPosition endPosition)
                : FunctionDecl(Decl::Kind::Operator, sourceFileID, std::move(attributes), visibility,
                               isConstExpr,
                               Identifier(operatorIdentifier.startPosition(),
                                          operatorIdentifier.endPosition(),
                                          operatorTypeName(operatorType) + "." + operatorIdentifier.name()),
                               declModifiers, std::move(parameters), returnType,
                               std::move(contracts), body, startPosition, endPosition),
                  _operatorType(operatorType), _operatorIdentifier(std::move(operatorIdentifier)) {}

        OperatorType operatorType() const { return _operatorType; }
        Identifier operatorIdentifier() const { return _operatorIdentifier; }

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

            auto result = new OperatorDecl(_sourceFileID, copiedAttributes, _declVisibility, _isConstExpr,
                                           _operatorType, _operatorIdentifier, _declModifiers,
                                           copiedParameters, copiedReturnType, copiedContracts,
                                           llvm::dyn_cast<CompoundStmt>(_body->deepCopy()),
                                           _startPosition, _endPosition);
            result->container = container;
            result->containedInTemplate = containedInTemplate;
            result->originalDecl = (originalDecl == nullptr ? this : originalDecl);
            return result;
        }

        std::string getPrototypeString() const override {
            std::string result = getDeclModifiersString(_declModifiers);

            if (!result.empty()) result += " ";

            switch (_operatorType) {
                case OperatorType::Prefix:
                    result += "operator prefix ";
                    break;
                case OperatorType::Infix:
                    result += "operator infix ";
                    break;
                case OperatorType::Postfix:
                    result += "operator postfix ";
                    break;
            }

            result += _operatorIdentifier.name() + "(";

            for (std::size_t i = 0; i < _parameters.size(); ++i) {
                if (i != 0) result += ", ";

                result += _parameters[i]->getPrototypeString();
            }

            return result + ")";
        }

    protected:
        OperatorType _operatorType;
        Identifier _operatorIdentifier;

    };
}

#endif //GULC_OPERATORDECL_HPP
