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
#ifndef GULC_TOKEN_HPP
#define GULC_TOKEN_HPP

#include <string>
#include <ast/Node.hpp>
#include "TokenType.hpp"

namespace gulc {
    struct Token {
        TokenType tokenType;
        TokenMetaType metaType;
        std::string currentSymbol;
        unsigned int currentChar;
        TextPosition startPosition;
        TextPosition endPosition;
        // Some of the GUL syntax doesn't allow spaces before tokens, we need to keep try of this.
        // E.g. `foreach!` no space before `!`
        //      `1.0f` no space before `.` and `0` or `f`
        bool hasLeadingWhitespace;

        Token(TokenType tokenType, TokenMetaType metaType, std::string currentSymbol, unsigned int currentChar,
              TextPosition startPosition, TextPosition endPosition, bool hasLeadingWhitespace)
                : tokenType(tokenType), metaType(metaType), currentSymbol(std::move(currentSymbol)),
                  currentChar(currentChar), startPosition(startPosition), endPosition(endPosition),
                  hasLeadingWhitespace(hasLeadingWhitespace) {}
    };
}

#endif //GULC_TOKEN_HPP
