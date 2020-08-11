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
#ifndef GULC_CONTRACTUTIL_HPP
#define GULC_CONTRACTUTIL_HPP

#include <ast/conts/WhereCont.hpp>
#include <string>
#include <vector>
#include <ast/exprs/CheckExtendsTypeExpr.hpp>

namespace gulc {
    class ContractUtil {
    public:
        ContractUtil(std::string const& fileName, std::vector<TemplateParameterDecl*> const* templateParameters,
                     std::vector<Expr*> const* templateArguments)
                : _fileName(fileName), _templateParameters(templateParameters),
                  _templateArguments(templateArguments) {}

        bool checkWhereCont(WhereCont* whereCont);

    protected:
        std::string const& _fileName;
        std::vector<TemplateParameterDecl*> const* _templateParameters;
        std::vector<Expr*> const* _templateArguments;

        void printError(std::string const& message, TextPosition startPosition, TextPosition endPosition) const;
        void printWarning(std::string const& message, TextPosition startPosition, TextPosition endPosition) const;

        Type const* getTemplateTypeArgument(TemplateTypenameRefType const* templateTypenameRefType) const;

        bool checkCheckExtendsTypeExpr(CheckExtendsTypeExpr* checkExtendsTypeExpr) const;

    };
}

#endif //GULC_CONTRACTUTIL_HPP
