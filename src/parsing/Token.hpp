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
