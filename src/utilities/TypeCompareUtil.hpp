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
#ifndef GULC_TYPECOMPAREUTIL_HPP
#define GULC_TYPECOMPAREUTIL_HPP

#include <vector>
#include <ast/decls/TemplateParameterDecl.hpp>
#include <ast/Expr.hpp>
#include <ast/types/TemplateTypenameRefType.hpp>

namespace gulc {
    enum class TemplateComparePlan {
        /// The referenced template parameter must be exactly the same.
        CompareExact,
        /// `T` == `G` basically. If both are templates then we will return saying they are the same. This is only
        /// useful in vary niche scenarios
        AllTemplatesAreSame,
    };

    class TypeCompareUtil {
    public:
        TypeCompareUtil() = default;
        TypeCompareUtil(std::vector<TemplateParameterDecl*> const* templateParameters,
                        std::vector<Expr*> const* templateArguments)
                : _templateParameters(templateParameters), _templateArguments(templateArguments) {}

        bool compareAreSame(Type const* left, Type const* right,
                            TemplateComparePlan templateComparePlan = TemplateComparePlan::CompareExact);
        bool compareAreSameOrInherits(Type const* checkType, Type const* extendsType);

    protected:
        std::vector<TemplateParameterDecl*> const* _templateParameters = nullptr;
        std::vector<Expr*> const* _templateArguments = nullptr;

        Type const* getTemplateTypenameArg(TemplateTypenameRefType* templateTypenameRefType) const;

    };
}

#endif //GULC_TYPECOMPAREUTIL_HPP
