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
#ifndef GULC_DECLMODIFIERS_HPP
#define GULC_DECLMODIFIERS_HPP

namespace gulc {
    enum class DeclModifiers {
        None = 0,
        Static = 1u << 0u,
        Mut = 1u << 1u,
        Volatile = 1u << 2u,
        Abstract = 1u << 3u,
        Virtual = 1u << 4u,
        Override = 1u << 5u,
        Extern = 1u << 6u,
        // Used for `func example();` with no body
        Prototype = 1u << 7u,
    };

    inline DeclModifiers operator|(DeclModifiers left, DeclModifiers right) { return static_cast<DeclModifiers>(static_cast<int>(left) | static_cast<int>(right)); }
    inline DeclModifiers operator&(DeclModifiers left, DeclModifiers right) { return static_cast<DeclModifiers>(static_cast<int>(left) & static_cast<int>(right)); }
    inline DeclModifiers operator^(DeclModifiers left, DeclModifiers right) { return static_cast<DeclModifiers>(static_cast<int>(left) ^ static_cast<int>(right)); }

    inline DeclModifiers& operator|=(DeclModifiers& left, DeclModifiers right) { left = left | right; return left; }
    inline DeclModifiers& operator&=(DeclModifiers& left, DeclModifiers right) { left = left & right; return left; }
    inline DeclModifiers& operator^=(DeclModifiers& left, DeclModifiers right) { left = left ^ right; return left; }
}

#endif //GULC_DECLMODIFIERS_HPP
