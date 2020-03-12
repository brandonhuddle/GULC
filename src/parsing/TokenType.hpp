#ifndef GULC_TOKENTYPE_HPP
#define GULC_TOKENTYPE_HPP

namespace gulc {
    enum class TokenMetaType {
        NIL,
        VALUE,
        KEYWORD,
        MODIFIER,
        OPERATOR,
        SPECIAL
    };

    enum class TokenType {
        NIL,
        // Value Tokens
        NUMBER, // Ex: 0xAF, 12, 0o7, 0b1101 - Decimals should be handled by the parser
        CHARACTER, // Ex: b, c, \t, \n, \0
        STRING, // Ex: "hello\t\n", "Здравствуйте", "您好", etc. (UTF-8 encoded?)
        SYMBOL,

        // Keyword Tokens
        TRAIT,
        STRUCT,
        CLASS,
        UNION,
        ENUM,
        OPERATOR,
        PREFIX,
        INFIX,
        POSTFIX,
        EXPLICIT,
        IMPLICIT,
        NAMESPACE,
        SIZEOF,
        ALIGNOF,
        OFFSETOF,
        NAMEOF,
        TRAITSOF,
        IF,
        ELSE,
        DO,
        WHILE,
        FOR,
        SWITCH,
        CASE,
        DEFAULT,
        CONTINUE,
        BREAK,
        GOTO,
        RETURN,
        ASM,
        IMPORT,
        AS, // Used for 'import foo as bar;' AND `expr as type`
        IS, // Used for `expr is type`
        HAS, // Used to check if a type `has` certain features e.g. `int has func parse(string) -> int`
        TRY,
        CATCH,
        FINALLY,
        THROW,
        THROWS, // Used to say when a function throws something `func example() -> int throws`
        REQUIRES,
        ENSURES,
        WHERE,
        FUNC,
        PROPERTY,
        LET,
        VAR,
        INIT, // Constructor
        DEINIT, // Destructor

        // Modifier Tokens
        PUBLIC, // 'public'
        PRIVATE, // 'private'
        PROTECTED, // 'protected'
        INTERNAL, // 'internal'
        STATIC, // 'static'
        CONST, // 'const'
        MUT, // `mut`
        IMMUT, // `immut`
        EXTERN, // 'extern'
        VOLATILE, // 'volatile'
        ABSTRACT, // 'abstract'
        SEALED, // 'sealed'
        VIRTUAL, // 'virtual'
        OVERRIDE, // 'override'
        IN, // 'in'
        OUT, // 'out'
        REF, // 'ref'
        INOUT, // Note sure if we need this

        // Operator Tokens
        EQUALS, // '='
        EQUALEQUALS, // '=='
        TEMPLATEEND, // '>' when right shift is disabled
        GREATER, // '>'
        GREATEREQUALS, // '>='
        RIGHT, // '>>'
        RIGHTEQUALS, // '>>='
        LESS, // '<'
        LESSEQUALS, // '<='
        LEFT, // '<<'
        LEFTEQUALS, // '<<='
        NOT, // '!'
        NOTEQUALS, // '!='
        TILDE, // '~'
        PLUS, // '+'
        PLUSEQUALS, // '+='
        PLUSPLUS, // '++'
        MINUS, // '-'
        MINUSEQUALS, // '-='
        MINUSMINUS, // '--'
        STAR, // '*'
        STAREQUALS, // '*='
        SLASH, // '/'
        SLASHEQUALS, // '/='
        PERCENT, // '%'
        PERCENTEQUALS, // '%='
        AMPERSAND, // '&'
        AMPERSANDEQUALS, // '&='
        AMPERSANDAMPERSAND, // '&&'
        PIPE, // '|'
        PIPEEQUALS, // '|='
        PIPEPIPE, // '||'
        CARET, // '^'
        CARETEQUALS, // '^='
        CARETCARET, // '^^' use for exponents
        CARETCARETEQUALS, // '^^='
        PERIOD, // '.'
        ARROW, // '->'
        COLON, // ':'
        COLONCOLON, // '::'
        QUESTION, // '?'
        QUESTIONQUESTION, // '??'
        QUESTIONPERIOD, // '?.'
        QUESTIONARROW, // '?->'
        QUESTIONLSQUARE, // '?['

        // Special Tokens
        LCURLY, // '{'
        RCURLY, // '}'
        LSQUARE, // '['
        RSQUARE, // ']'
        SEMICOLON, // ';'
        COMMA, // ','
        LPAREN, // '('
        RPAREN, // ')'
        ATSYMBOL, // '@', used to grab the string representation of a compiler keyword. E.g. `let @class = 12;`
        ENDOFFILE // might be removed, just used to double check everything is parsed correctly
    };
}

#endif //GULC_TOKENTYPE_HPP
