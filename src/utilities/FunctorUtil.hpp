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
#ifndef GULC_FUNCTORUTIL_HPP
#define GULC_FUNCTORUTIL_HPP

#include "SignatureComparer.hpp"

namespace gulc {
    /**
     * `FunctorUtil` is used to check types for "functor" abilities.
     * The below are valid functors:
     *
     *     var funcPointer: func(param1: i32)
     *
     *     struct CallExample {
     *         call(param1: i32) {}
     *     }
     *
     *     var callExample = CallExample.init()
     *     callExample()
     */
    class FunctorUtil {
    public:
        /**
         * Check if the `checkType` is valid to be called as a functor.
         *
         * @param checkType
         * @param arguments
         * @param outFoundDecl - Filled with the found `Decl` underlying the functor call (only for Struct and Trait)
         * @return
         */
        static SignatureComparer::ArgMatchResult checkValidFunctorCall(Type* checkType,
                std::vector<LabeledArgumentExpr*> const& arguments, Decl** outFoundDecl);

    };
}

#endif //GULC_FUNCTORUTIL_HPP
