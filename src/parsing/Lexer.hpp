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
#ifndef GULC_LEXER_HPP
#define GULC_LEXER_HPP

#include <string>
#include "Token.hpp"

namespace gulc {
    struct LexerCheckpoint {
        Token _nextToken = Token(TokenType::NIL, TokenMetaType::NIL, {}, 0, {}, {}, false);
        unsigned int currentLine = 1;
        unsigned int currentColumn = 0;
        unsigned int currentIndex = 0;

        LexerCheckpoint(Token nextToken, unsigned int currentLine, unsigned int currentColumn,
                        unsigned int currentIndex)
                : _nextToken(std::move(nextToken)),
                  currentLine(currentLine), currentColumn(currentColumn), currentIndex(currentIndex) { }

    };

    class Lexer {
    public:
        Lexer() = default;
        Lexer(std::string filePath, std::string sourceCode)
                : _filePath(std::move(filePath)), _sourceCode(std::move(sourceCode)) { }

        TokenType peekType();
        TokenMetaType peekMeta();
        std::string const& peekCurrentSymbol();
        TextPosition peekStartPosition();
        TextPosition peekEndPosition();
        bool peekHasLeadingWhitespace();
        Token const& peekToken();
        Token nextToken();
        bool consumeType(TokenType type);

        LexerCheckpoint createCheckpoint();
        void returnToCheckpoint(const LexerCheckpoint& checkpoint);

        bool getRightShiftState() const;
        void setRightShiftState(bool enabled);

    private:
        std::string _filePath;
        std::string _sourceCode;
        Token _nextToken = Token(TokenType::NIL, TokenMetaType::NIL, {}, 0, {}, {}, false);
        unsigned int _currentLine = 1;
        unsigned int _currentColumn = 1;
        unsigned int _currentIndex = 0;
        // We support disabling this so we can do 'GenericType1<GenericType2<GenericType3<int>>>' easily
        // This will make it so the last three '>' characters will come in as separate tokens rather than coming in as one '>' token and one '>>' token
        bool _rightShiftEnabled = true;

        Token lexOneToken();
        Token parseToken(std::string& tokenText, TextPosition startPosition, bool hasLeadingWhitespace);

        void printError(const std::string& errorText, int errorCode = 1);
        void errorUnexpectedEOF();

    };
}

#endif //GULC_LEXER_HPP
