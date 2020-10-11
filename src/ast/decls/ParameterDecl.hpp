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
#ifndef GULC_PARAMETERDECL_HPP
#define GULC_PARAMETERDECL_HPP

#include <ast/Decl.hpp>
#include <ast/Type.hpp>
#include <ast/Expr.hpp>
#include <optional>

namespace gulc {
    class ParameterDecl : public Decl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::Parameter; }

        enum class ParameterKind {
            Val, // The normal, unlabeled parameter
            In, // An `in` reference parameter
            Out, // An `out` reference parameter, requires the parameter to be written on all codepaths
        };

        Type* type;
        Expr* defaultValue;
        ParameterKind parameterKind() const { return _parameterKind; }

        ParameterDecl(unsigned int sourceFileID, std::vector<Attr*> attributes,
                      Identifier argumentLabel, Identifier identifier,
                      Type* type, Expr* defaultValue, ParameterKind parameterKind,
                      TextPosition startPosition, TextPosition endPosition)
                : Decl(Decl::Kind::Parameter, sourceFileID, std::move(attributes),
                       Decl::Visibility::Unassigned, false, std::move(identifier),
                       DeclModifiers::None),
                  type(type), defaultValue(defaultValue), _argumentLabel(std::move(argumentLabel)),
                  _parameterKind(parameterKind), _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Identifier const& argumentLabel() const { return _argumentLabel; }

        Decl* deepCopy() const override {
            std::vector<Attr*> copiedAttributes;
            copiedAttributes.reserve(_attributes.size());
            Expr* copiedDefaultValue = nullptr;

            for (Attr* attribute : _attributes) {
                copiedAttributes.push_back(attribute->deepCopy());
            }

            if (defaultValue != nullptr) {
                copiedDefaultValue = defaultValue->deepCopy();
            }

            auto result = new ParameterDecl(_sourceFileID, copiedAttributes, _argumentLabel,
                                            _identifier, type->deepCopy(), copiedDefaultValue,
                                            _parameterKind, _startPosition, _endPosition);
            result->container = container;
            result->containedInTemplate = containedInTemplate;
            result->originalDecl = (originalDecl == nullptr ? this : originalDecl);
            return result;
        }

        std::string getPrototypeString() const override {
            std::string result = getDeclModifiersString(_declModifiers);

            if (!result.empty()) result += " ";

            result += _argumentLabel.name() + ": ";

            switch (_parameterKind) {
                case ParameterKind::Val:
                    result += type->toString();
                    break;
                case ParameterKind::In:
                    result += "in " + type->toString();
                    break;
                case ParameterKind::Out:
                    result += "out " + type->toString();
                    break;
            }

            return result;
        }

        ~ParameterDecl() override {
            delete type;
            delete defaultValue;
        }

    protected:
        Identifier _argumentLabel;
        ParameterKind _parameterKind;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_PARAMETERDECL_HPP
