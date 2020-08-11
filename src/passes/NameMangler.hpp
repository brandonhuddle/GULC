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
#ifndef GULC_NAMEMANGLER_HPP
#define GULC_NAMEMANGLER_HPP

#include <namemangling/ManglerBase.hpp>
#include <parsing/ASTFile.hpp>

namespace gulc {
    class NameMangler {
    public:
        explicit NameMangler(ManglerBase* manglerBase)
                : _manglerBase(manglerBase) {}

        void processFiles(std::vector<ASTFile>& files);

    private:
        ManglerBase* _manglerBase;

        void processTypeDecl(Decl* decl);
        void processDecl(Decl* decl);

    };
}

#endif //GULC_NAMEMANGLER_HPP
