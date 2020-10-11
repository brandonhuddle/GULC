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
#ifndef GULC_CONSTSOLVER_HPP
#define GULC_CONSTSOLVER_HPP

#include <ast/exprs/SolvedConstExpr.hpp>
#include <ast/exprs/HasExpr.hpp>
#include <ast/decls/PropertyDecl.hpp>
#include <ast/decls/SubscriptOperatorDecl.hpp>
#include <ast/decls/OperatorDecl.hpp>
#include <ast/decls/CallOperatorDecl.hpp>
#include "TypeCompareUtil.hpp"

namespace gulc {
    class ConstSolver {
    public:
        static SolvedConstExpr* solveHasExpr(HasExpr* hasExpr);

        static bool containsMatchingVariableDecl(std::vector<Decl*> const& checkDecls,
                                                 TypeCompareUtil& typeCompareUtil, VariableDecl* findVariable);
        static bool containsMatchingPropertyDecl(std::vector<Decl*> const& checkDecls,
                                                 TypeCompareUtil& typeCompareUtil, PropertyDecl* findProperty);
        static bool containsMatchingSubscriptOperatorDecl(std::vector<Decl*> const& checkDecls,
                                                          TypeCompareUtil& typeCompareUtil,
                                                          SubscriptOperatorDecl* findSubscriptOperator);
        static bool containsMatchingFunctionDecl(std::vector<Decl*> const& checkDecls,
                                                 TypeCompareUtil& typeCompareUtil, FunctionDecl* findFunction);
        static bool containsMatchingOperatorDecl(std::vector<Decl*> const& checkDecls,
                                                 TypeCompareUtil& typeCompareUtil, OperatorDecl* findOperator);
        static bool containsMatchingCallOperatorDecl(std::vector<Decl*> const& checkDecls,
                                                     TypeCompareUtil& typeCompareUtil,
                                                     CallOperatorDecl* findCallOperator);
        static bool containsMatchingConstructorDecl(std::vector<ConstructorDecl*> const& checkConstructors,
                                                    TypeCompareUtil& typeCompareUtil, ConstructorDecl* findConstructor);

    };
}

#endif //GULC_CONSTSOLVER_HPP
