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
#ifndef GULC_MANGLERBASE_HPP
#define GULC_MANGLERBASE_HPP

#include <ast/decls/EnumDecl.hpp>
#include <ast/decls/StructDecl.hpp>
#include <ast/decls/NamespaceDecl.hpp>
#include <ast/decls/TemplateFunctionDecl.hpp>
#include <ast/decls/OperatorDecl.hpp>
#include <ast/decls/CallOperatorDecl.hpp>
#include <ast/decls/SubscriptOperatorDecl.hpp>
#include <ast/decls/TraitDecl.hpp>
#include <ast/decls/PropertyDecl.hpp>

namespace gulc {
    // I'm making the name mangler an abstract class to allow us to use Itanium on nearly every platform but on Windows
    // On windows I would at least like the option to try and match the Windows C++ ABI. From what I can tell the
    // Windows C++ ABI doesn't have public documentation except various documentation made from people reverse
    // engineering it throughout the years. I might have to drive to Redwood and try to become friends with a Microsoft
    // employee at some point...
    // TODO: Maybe we should give up on this idea. I think Ghoul should be uniform across all platforms. We'll have
    //       translation layers to handle proper communication with system libraries but Ghoul should have a standard
    //       ABI that is consistent across all operating systems (certain things can differ per architecture but not OS)
    //       I think trying to make Ghoul compile to the Windows C++ ABI would be a lot of work for little to no
    //       benefit. Consistency is better than unneeded complexity.
    class ManglerBase {
    public:
        // We have to do a prepass on declared types to mangle their names because we will need to access them as
        // parameters in the function signature
        virtual void mangleDecl(EnumDecl* enumDecl) = 0;
        virtual void mangleDecl(StructDecl* structDecl) = 0;
        virtual void mangleDecl(TraitDecl* traitDecl) = 0;
        virtual void mangleDecl(NamespaceDecl* namespaceDecl) = 0;

        virtual void mangle(FunctionDecl* functionDecl) = 0;
        virtual void mangle(VariableDecl* variableDecl) = 0;
        virtual void mangle(NamespaceDecl* namespaceDecl) = 0;
        virtual void mangle(StructDecl* structDecl) = 0;
        virtual void mangle(TraitDecl* traitDecl) = 0;
        virtual void mangle(CallOperatorDecl* callOperatorDecl) = 0;
        virtual void mangle(PropertyDecl* propertyDecl) = 0;

        virtual ~ManglerBase() = default;

    };
}

#endif //GULC_MANGLERBASE_HPP
