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
#ifndef GULC_TARGET_HPP
#define GULC_TARGET_HPP

#include <cstddef>

namespace gulc {
    class Target {
    public:
        // Currently only support x86_64
        enum class Arch {
            x86_64
        };

        enum class OS {
            Linux,
            Windows
        };

        enum class Env {
            GNU,
            MSVC
        };

    private:
        Arch _arch;
        OS _os;
        Env _env;

        std::size_t _sizeofPtr;
        /// sizeof(usize) == sizeof(isize)
        std::size_t _sizeofUSize;

        /// The align of for struct also tells us how to pad the struct
        std::size_t _alignofStruct;

    public:
        Target(Arch arch, OS os, Env env);

        Arch getArch() const { return _arch; }
        OS getOS() const { return _os; }
        Env getEnv() const { return _env; }

        std::size_t sizeofPtr() const { return _sizeofPtr; }
        std::size_t sizeofUSize() const { return _sizeofUSize; }
        std::size_t sizeofISize() const { return _sizeofUSize; }

        std::size_t alignofStruct() const { return _alignofStruct; }

        static Target getHostTarget();

    };
}

#endif //GULC_TARGET_HPP
