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
#ifndef GULC_CODETRANSFORMER_HPP
#define GULC_CODETRANSFORMER_HPP

namespace gulc {
    /**
     * CodeTransformer handles the following:
     *  * Verify all Stmts and Exprs are valid
     *  * Add temporary values to the AST
     *  * Insert destructor calls and other deferred statements
     *
     * The purpose of this is to validate the code that has come in from the source file (mostly unchanged since
     * parsing) then it transforms the code into something much easier for `CodeGen` to work with.
     */
    class CodeTransformer {

    };
}

#endif //GULC_CODETRANSFORMER_HPP
