#include <fstream>
#include <sstream>
#include <iostream>
#include <ast/exprs/ParenExpr.hpp>
#include <ast/types/ReferenceType.hpp>
#include <llvm/Support/Casting.h>
#include <ast/types/UnresolvedType.hpp>
#include <ast/types/PointerType.hpp>
#include <ast/types/DimensionType.hpp>
#include <ast/exprs/AssignmentOperatorExpr.hpp>
#include <ast/exprs/TernaryExpr.hpp>
#include <ast/exprs/PrefixOperatorExpr.hpp>
#include <ast/exprs/PostfixOperatorExpr.hpp>
#include <ast/exprs/FunctionCallExpr.hpp>
#include <ast/exprs/IndexerCallExpr.hpp>
#include <ast/exprs/MemberAccessCallExpr.hpp>
#include <ast/exprs/VariableDeclExpr.hpp>
#include <ast/attrs/UnresolvedAttr.hpp>
#include <ast/exprs/AsExpr.hpp>
#include <ast/exprs/IsExpr.hpp>
#include <ast/exprs/HasExpr.hpp>
#include <ast/exprs/LabeledArgumentExpr.hpp>
#include <ast/exprs/ArrayLiteralExpr.hpp>
#include <ast/exprs/TypeExpr.hpp>
#include <ast/decls/TemplateStructDecl.hpp>
#include <ast/decls/TemplateTraitDecl.hpp>
#include "Parser.hpp"

using namespace gulc;

ASTFile Parser::parseFile(unsigned int fileID, std::string const& filePath) {
    std::ifstream fileStream(filePath);

    if (fileStream.good()) {
        std::stringstream buffer;
        buffer << fileStream.rdbuf();

        _lexer = Lexer(filePath, buffer.str());
        _fileID = fileID;
        _filePath = filePath;
    } else {
        std::cout << "gulc error: file '" << filePath << "' was not found!" << std::endl;
        std::exit(1);
    }

    std::vector<Decl*> result;

    while (_lexer.peekType() != TokenType::ENDOFFILE) {
        result.push_back(parseDecl());
    }

    return ASTFile(fileID, result);
}

/**
 * Print an error to the console with the position in the source code file that we are erroring out for
 * After print the error message out we exit the application with exit code `1`
 *
 * @param errorMessage - message to print to the console
 * @param startPosition - start position of the section of code that caused the error
 * @param endPosition - end position of the section of code that caused the error
 */
void Parser::printError(const std::string& errorMessage, TextPosition startPosition, TextPosition endPosition) {
    std::cout << "gulc parser error[" << _filePath << ", "
                                   "{" << startPosition.line << ", " << startPosition.column << "} "
                                   "to {" << endPosition.line << ", " << endPosition.column << "}]: "
              << errorMessage
              << std::endl;

    std::exit(1);
}

/**
 * Print a warning to the console with the position in the source code file that we are warning about
 *
 * @param warningMessage - message to print to the console
 * @param startPosition - start position of the section of code to warn about
 * @param endPosition - end position of the section of code to warn about
 */
void Parser::printWarning(const std::string &warningMessage, TextPosition startPosition, TextPosition endPosition) {
    std::cout << "gulc parser warning[" << _filePath << ", "
                                     "{" << startPosition.line << ", " << startPosition.column << "} "
                                     "to {" << endPosition.line << ", " << endPosition.column << "}]: "
              << warningMessage
              << std::endl;
}

std::vector<Attr*> Parser::parseAttrs() {
    std::vector<Attr*> result;

    while (_lexer.consumeType(TokenType::LSQUARE)) {
        // Parse the first and potentially only attribute
        result.push_back(parseAttr());

        // If there is a comma then that means they're specifying multiple attributes in the same `[...]` which is
        // allowed
        while (_lexer.consumeType(TokenType::COMMA)) {
            result.push_back(parseAttr());
        }

        if (!_lexer.consumeType(TokenType::RSQUARE)) {
            printError("expected ending `]` for attribute! (found `" + _lexer.peekToken().currentSymbol + "`)",
                       _lexer.peekToken().startPosition, _lexer.peekToken().endPosition);
        }
    }

    return result;
}

Attr* Parser::parseAttr() {
    TextPosition startPosition = _lexer.peekToken().startPosition;
    TextPosition endPosition = _lexer.peekToken().endPosition;

    if (_lexer.peekType() != TokenType::SYMBOL) {
        printError("expected attribute name, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekToken().startPosition, _lexer.peekToken().endPosition);
    }

    std::vector<Identifier> namespacePath;
    Identifier attributeName = parseIdentifier();
    std::vector<Expr*> arguments;

    while (_lexer.peekType() == TokenType::PERIOD) {
        _lexer.consumeType(TokenType::PERIOD);
        namespacePath.push_back(attributeName);

        attributeName = parseIdentifier();
        endPosition = _lexer.peekToken().endPosition;

        if (!_lexer.consumeType(TokenType::SYMBOL)) {
            printError("expected namespace or attribute name after `.`, found `" + attributeName.name() + "`!",
                       _lexer.peekToken().startPosition, _lexer.peekToken().endPosition);
            return nullptr;
        }
    }

    // If the next token is an `(` we immediately consume it and then parse the arguments
    // NOTE: Attributes can be called without parenthesis. `[attribute]`, `[move]`, `[move()]`, etc. are all allowed
    if (_lexer.consumeType(TokenType::LPAREN)) {
        while (_lexer.peekType() != TokenType::RPAREN && _lexer.peekType() != TokenType::ENDOFFILE) {
            arguments.push_back(parseExpr());

            // If the next token isn't a comma we break from the loop
            if (!_lexer.consumeType(TokenType::COMMA)) {
                break;
            }
        }

        endPosition = _lexer.peekToken().endPosition;

        if (!_lexer.consumeType(TokenType::RPAREN)) {
            printError("expected ending `)` after attribute arguments! (found '" + _lexer.peekToken().currentSymbol + "')",
                       _lexer.peekToken().startPosition, _lexer.peekToken().endPosition);
        }
    }

    return new UnresolvedAttr(startPosition, endPosition, namespacePath, attributeName, arguments);
}

std::vector<Identifier> Parser::parseDotSeparatedIdentifiers() {
    std::vector<Identifier> result {
        parseIdentifier()
    };

    while (_lexer.consumeType(TokenType::PERIOD)) {
        result.push_back(parseIdentifier());
    }

    return result;
}

Identifier Parser::parseIdentifier() {
    if (_lexer.consumeType(TokenType::ATSYMBOL)) {
        Token currentToken = _lexer.peekToken();
        std::string identifier = currentToken.currentSymbol;

        if (currentToken.metaType != TokenMetaType::KEYWORD && currentToken.metaType != TokenMetaType::MODIFIER &&
                currentToken.tokenType != TokenType::SYMBOL) {
            printError("expected identifier, found `" + _lexer.peekCurrentSymbol() + "`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }

        _lexer.consumeType(currentToken.tokenType);

        return Identifier(currentToken.startPosition, currentToken.endPosition, currentToken.currentSymbol);
    } else {
        Token currentToken = _lexer.peekToken();

        if (!_lexer.consumeType(TokenType::SYMBOL)) {
            printError("expected identifier, found `" + _lexer.peekCurrentSymbol() + "`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }

        return Identifier(currentToken.startPosition, currentToken.endPosition, currentToken.currentSymbol);
    }
}

Type* Parser::parseType() {
    TextPosition startPosition = _lexer.peekStartPosition();
    TextPosition endPosition = _lexer.peekEndPosition();

    switch (_lexer.peekType()) {
        case TokenType::CONST: {
            printError("`const` cannot be used in this context! "
                       "(`const` is equivalent to `constexpr` in C++, did you mean `immut`?)",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
            return nullptr;
        }
        case TokenType::MUT: {
            _lexer.consumeType(TokenType::MUT);
            bool parseParen = _lexer.consumeType(TokenType::LPAREN);

            Type* nestedType = parseType();

            if (parseParen && !_lexer.consumeType(TokenType::RPAREN)) {
                printError("expected ending `)`, found `" + _lexer.peekCurrentSymbol() + "`!",
                           _lexer.peekStartPosition(), _lexer.peekEndPosition());
            }

            if (nestedType->qualifier() != Type::Qualifier::Unassigned) {
                if (nestedType->qualifier() == Type::Qualifier::Mut) {
                    printError("duplicate `mut` keyword is not allowed!",
                               startPosition, endPosition);
                } else if (nestedType->qualifier() == Type::Qualifier::Immut) {
                    printError("`mut immut` is not allowed!",
                               startPosition, endPosition);
                }
            }

            nestedType->setQualifier(Type::Qualifier::Mut);

            return nestedType;
        }
        case TokenType::IMMUT: {
            _lexer.consumeType(TokenType::IMMUT);
            bool parseParen = _lexer.consumeType(TokenType::LPAREN);

            Type* nestedType = parseType();

            if (parseParen && !_lexer.consumeType(TokenType::RPAREN)) {
                printError("expected ending `)`, found `" + _lexer.peekCurrentSymbol() + "`!",
                           _lexer.peekStartPosition(), _lexer.peekEndPosition());
            }

            if (nestedType->qualifier() != Type::Qualifier::Unassigned) {
                if (nestedType->qualifier() == Type::Qualifier::Immut) {
                    printError("duplicate `immut` keyword is not allowed!",
                               startPosition, endPosition);
                } else if (nestedType->qualifier() == Type::Qualifier::Mut) {
                    printError("`immut mut` is not allowed!",
                               startPosition, endPosition);
                }
            }

            nestedType->setQualifier(Type::Qualifier::Immut);

            return nestedType;
        }
        case TokenType::REF: {
            _lexer.consumeType(TokenType::REF);
            bool parseParen = _lexer.consumeType(TokenType::LPAREN);

            Type* nestedType = parseType();

            if (parseParen && !_lexer.consumeType(TokenType::RPAREN)) {
                printError("expected ending `)`, found `" + _lexer.peekCurrentSymbol() + "`!",
                           _lexer.peekStartPosition(), _lexer.peekEndPosition());
            }

            return new ReferenceType(Type::Qualifier::Unassigned, nestedType);
        }
        case TokenType::STAR:
            _lexer.consumeType(TokenType::STAR);

            return new PointerType(Type::Qualifier::Unassigned, parseType());
        case TokenType::LSQUARE: {
            _lexer.consumeType(TokenType::LSQUARE);

            // Dimensions always start at one. `int[]` is 1-dimensional, `int[,]` is 2d, etc.
            std::size_t dimensions = 1;

            while (_lexer.peekType() != TokenType::RSQUARE && _lexer.peekType() != TokenType::ENDOFFILE) {
                dimensions += 1;

                if (!_lexer.consumeType(TokenType::COMMA)) {
                    break;
                }
            }

            if (!_lexer.consumeType(TokenType::RSQUARE)) {
                printError(
                        "expected `,` or `]` for dimension type, found `" + _lexer.peekCurrentSymbol() + "`!",
                        _lexer.peekStartPosition(), _lexer.peekEndPosition());
            }

            return new DimensionType(Type::Qualifier::Unassigned, parseType(), dimensions);
        }
        case TokenType::SYMBOL: {
            std::vector<Identifier> namespacePath;
            Identifier typeIdentifier(parseIdentifier());

            while (_lexer.consumeType(TokenType::PERIOD)) {
                namespacePath.push_back(std::move(typeIdentifier));

                typeIdentifier = parseIdentifier();
            }

            std::vector<Expr*> templateArguments;

            if (_lexer.peekType() == TokenType::LESS) {
                _lexer.consumeType(TokenType::LESS);

                // We tell the lexer to NOT combine two `>` operators into a single `>>` token.
                // this also tells the lexer to return `TEMPLATEEND` instead of `GREATER` for `>`
                // We back up the old state so we can return to it later. Allowing nested states for whether this is on or off
                bool oldRightShiftEnabledValue = _lexer.getRightShiftState();
                _lexer.setRightShiftState(false);

                // Parse until we find the closing `>` or until we hit the end of the file
                while (_lexer.peekType() != TokenType::TEMPLATEEND && _lexer.peekType() != TokenType::ENDOFFILE) {
                    templateArguments.push_back(parseExpr());

                    // If consuming a comma failed then break, this is a quick an easy operation.
                    if (!_lexer.consumeType(TokenType::COMMA)) break;
                }

                if (!_lexer.consumeType(TokenType::TEMPLATEEND)) {
                    printError("expected closing '>' for template type reference! (found: '" + _lexer.peekToken().currentSymbol + "')",
                               _lexer.peekToken().startPosition, _lexer.peekToken().endPosition);
                }

                // Return to the old state for whether the lexer should combine two `>` into a `>>` Token
                _lexer.setRightShiftState(oldRightShiftEnabledValue);
            }

            return new UnresolvedType(Type::Qualifier::Unassigned, namespacePath, typeIdentifier, templateArguments);
        }
        default:
            printError("expected `const`, `mut`, `ref`, or a type name, found `" + _lexer.peekCurrentSymbol() + "`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
            return nullptr;
    }
}

Decl::Visibility Parser::parseDeclVisibility() {
    if (_lexer.consumeType(TokenType::PRIVATE)) {
        return Decl::Visibility::Private;
    } else if (_lexer.consumeType(TokenType::PUBLIC)) {
        return Decl::Visibility::Public;
    } else if (_lexer.consumeType(TokenType::INTERNAL)) {
        return Decl::Visibility::Internal;
    } else if (_lexer.consumeType(TokenType::PROTECTED)) {
        if (_lexer.consumeType(TokenType::INTERNAL)) {
            return Decl::Visibility::ProtectedInternal;
        }

        return Decl::Visibility::Protected;
    }

    return Decl::Visibility::Unassigned;
}

DeclModifiers Parser::parseDeclModifiers(bool* isConstExpr) {
    DeclModifiers declModifiers = DeclModifiers::None;

    while (_lexer.peekMeta() == TokenMetaType::MODIFIER) {
        switch (_lexer.peekType()) {
            case TokenType::STATIC:
                if ((declModifiers & DeclModifiers::Static) == DeclModifiers::Static) {
                    printError("duplicate `static` keyword!",
                               _lexer.peekStartPosition(), _lexer.peekEndPosition());
                }

                _lexer.consumeType(TokenType::STATIC);
                declModifiers |= DeclModifiers::Static;
                break;
            case TokenType::EXTERN:
                if ((declModifiers & DeclModifiers::Extern) == DeclModifiers::Extern) {
                    printError("duplicate `extern` keyword!",
                               _lexer.peekStartPosition(), _lexer.peekEndPosition());
                }

                _lexer.consumeType(TokenType::EXTERN);
                declModifiers |= DeclModifiers::Extern;
                break;
            case TokenType::CONST:
                if (*isConstExpr) {
                    printError("duplicate `const` keyword!",
                               _lexer.peekStartPosition(), _lexer.peekEndPosition());
                }

                _lexer.consumeType(TokenType::CONST);
                (*isConstExpr) = true;
                break;
            case TokenType::MUT:
                if ((declModifiers & DeclModifiers::Mut) == DeclModifiers::Mut) {
                    printError("duplicate `mut` keyword!",
                               _lexer.peekStartPosition(), _lexer.peekEndPosition());
                }

                _lexer.consumeType(TokenType::MUT);
                declModifiers |= DeclModifiers::Mut;
                break;
            case TokenType::VOLATILE:
                if ((declModifiers & DeclModifiers::Volatile) == DeclModifiers::Volatile) {
                    printError("duplicate `volatile` keyword!",
                               _lexer.peekStartPosition(), _lexer.peekEndPosition());
                }

                _lexer.consumeType(TokenType::VOLATILE);
                declModifiers |= DeclModifiers::Volatile;
                break;
            case TokenType::ABSTRACT:
                if ((declModifiers & DeclModifiers::Abstract) == DeclModifiers::Abstract) {
                    printError("duplicate `abstract` keyword!",
                               _lexer.peekStartPosition(), _lexer.peekEndPosition());
                }

                _lexer.consumeType(TokenType::ABSTRACT);
                declModifiers |= DeclModifiers::Abstract;
                break;
            case TokenType::VIRTUAL:
                if ((declModifiers & DeclModifiers::Virtual) == DeclModifiers::Virtual) {
                    printError("duplicate `virtual` keyword!",
                               _lexer.peekStartPosition(), _lexer.peekEndPosition());
                }

                _lexer.consumeType(TokenType::VIRTUAL);
                declModifiers |= DeclModifiers::Virtual;
                break;
            case TokenType::OVERRIDE:
                if ((declModifiers & DeclModifiers::Override) == DeclModifiers::Override) {
                    printError("duplicate `override` keyword!",
                               _lexer.peekStartPosition(), _lexer.peekEndPosition());
                }

                _lexer.consumeType(TokenType::OVERRIDE);
                declModifiers |= DeclModifiers::Override;
                break;
            default:
                printError("unknown modifier `" + _lexer.peekCurrentSymbol() + "`!",
                           _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }
    }

    return declModifiers;
}

Decl* Parser::parseDecl() {
    std::vector<Attr*> attributes(parseAttrs());
    TextPosition startPosition = _lexer.peekStartPosition();
    Decl::Visibility visibility = parseDeclVisibility();
    bool isConst = false;
    DeclModifiers declModifiers = parseDeclModifiers(&isConst);

    switch (_lexer.peekType()) {
        case TokenType::IMPORT:
            if (visibility != Decl::Visibility::Unassigned) printError("imports cannot have visibility modifiers!", startPosition, _lexer.peekEndPosition());
            if ((declModifiers & DeclModifiers::Static) == DeclModifiers::Static) printError("imports cannot be `static`!", startPosition, _lexer.peekEndPosition());
            if ((declModifiers & DeclModifiers::Extern) == DeclModifiers::Extern) printError("imports cannot be `extern`!", startPosition, _lexer.peekEndPosition());
            if (isConst) printError("imports cannot be `const`!", startPosition, _lexer.peekEndPosition());
            if ((declModifiers & DeclModifiers::Mut) == DeclModifiers::Mut) printError("imports cannot be `mut`!", startPosition, _lexer.peekEndPosition());
            if ((declModifiers & DeclModifiers::Volatile) == DeclModifiers::Volatile) printError("imports cannot be `volatile`!", startPosition, _lexer.peekEndPosition());
            if ((declModifiers & DeclModifiers::Abstract) == DeclModifiers::Abstract) printError("imports cannot be `abstract`!", startPosition, _lexer.peekEndPosition());
            if ((declModifiers & DeclModifiers::Virtual) == DeclModifiers::Virtual) printError("imports cannot be `virtual`!", startPosition, _lexer.peekEndPosition());
            if ((declModifiers & DeclModifiers::Override) == DeclModifiers::Override) printError("imports cannot be `override`!", startPosition, _lexer.peekEndPosition());

            return parseImportDecl(attributes, startPosition);
        case TokenType::NAMESPACE:
            if (visibility != Decl::Visibility::Unassigned) printError("namespaces cannot have visibility modifiers!", startPosition, _lexer.peekEndPosition());
            if ((declModifiers & DeclModifiers::Static) == DeclModifiers::Static) printError("namespaces cannot be `static`!", startPosition, _lexer.peekEndPosition());
            if ((declModifiers & DeclModifiers::Extern) == DeclModifiers::Extern) printError("namespaces cannot be `extern`!", startPosition, _lexer.peekEndPosition());
            if (isConst) printError("namespaces cannot be `const`!", startPosition, _lexer.peekEndPosition());
            if ((declModifiers & DeclModifiers::Mut) == DeclModifiers::Mut) printError("namespaces cannot be `mut`!", startPosition, _lexer.peekEndPosition());
            if ((declModifiers & DeclModifiers::Volatile) == DeclModifiers::Volatile) printError("namespaces cannot be `volatile`!", startPosition, _lexer.peekEndPosition());
            if ((declModifiers & DeclModifiers::Abstract) == DeclModifiers::Abstract) printError("namespaces cannot be `abstract`!", startPosition, _lexer.peekEndPosition());
            if ((declModifiers & DeclModifiers::Virtual) == DeclModifiers::Virtual) printError("namespaces cannot be `virtual`!", startPosition, _lexer.peekEndPosition());
            if ((declModifiers & DeclModifiers::Override) == DeclModifiers::Override) printError("namespaces cannot be `override`!", startPosition, _lexer.peekEndPosition());

            return parseNamespaceDecl(attributes);
        case TokenType::TYPEALIAS:
            if ((declModifiers & DeclModifiers::Static) == DeclModifiers::Static) printError("typealiases cannot be `static`!", startPosition, _lexer.peekEndPosition());
            if ((declModifiers & DeclModifiers::Extern) == DeclModifiers::Extern) printError("typealiases cannot be `extern`!", startPosition, _lexer.peekEndPosition());
            if (isConst) printError("typealiases cannot be `const`!", startPosition, _lexer.peekEndPosition());
            if ((declModifiers & DeclModifiers::Mut) == DeclModifiers::Mut) printError("typealiases cannot be `mut`!", startPosition, _lexer.peekEndPosition());
            if ((declModifiers & DeclModifiers::Volatile) == DeclModifiers::Volatile) printError("typealiases cannot be `volatile`!", startPosition, _lexer.peekEndPosition());
            if ((declModifiers & DeclModifiers::Abstract) == DeclModifiers::Abstract) printError("typealiases cannot be `abstract`!", startPosition, _lexer.peekEndPosition());
            if ((declModifiers & DeclModifiers::Virtual) == DeclModifiers::Virtual) printError("typealiases cannot be `virtual`!", startPosition, _lexer.peekEndPosition());
            if ((declModifiers & DeclModifiers::Override) == DeclModifiers::Override) printError("typealiases cannot be `override`!", startPosition, _lexer.peekEndPosition());

            return parseTypeAliasDecl(attributes, visibility, startPosition);

        case TokenType::TYPESUFFIX:
            return parseTypeSuffixDecl(attributes, visibility, isConst, declModifiers, startPosition);

        case TokenType::FUNC:
            return parseFunctionDecl(attributes, visibility, isConst, declModifiers, startPosition);
        case TokenType::INIT:
            return parseConstructorDecl(attributes, visibility, isConst, declModifiers, startPosition);
        case TokenType::DEINIT:
            return parseDestructorDecl(attributes, visibility, isConst, declModifiers, startPosition);
        case TokenType::CALL:
            // NOTE: Functors/functionoids cannot have templates arguments. This would potentially lead to confusing
            //       syntax
            return parseCallOperatorDecl(attributes, visibility, isConst, declModifiers, startPosition);
        case TokenType::SUBSCRIPT:
            return parseSubscriptOperator(attributes, visibility, isConst, startPosition, declModifiers);
        case TokenType::PROPERTY:
            // NOTE: Properties shouldn't be able to be templates, throwing random `variable.prop<int> = 21` looks weird
            return parsePropertyDecl(attributes, visibility, isConst, declModifiers, startPosition);
        case TokenType::OPERATOR:
            return parseOperatorDecl(attributes, visibility, isConst, declModifiers, startPosition);
        case TokenType::STRUCT:
            return parseStructDecl(attributes, visibility, isConst, startPosition, declModifiers,
                                   StructDecl::Kind::Struct);
        case TokenType::CLASS:
            return parseStructDecl(attributes, visibility, isConst, startPosition, declModifiers,
                                   StructDecl::Kind::Class);
        case TokenType::UNION:
            return parseStructDecl(attributes, visibility, isConst, startPosition, declModifiers,
                                   StructDecl::Kind::Union);
        case TokenType::TRAIT:
            return parseTraitDecl(attributes, visibility, isConst, startPosition, declModifiers);
        case TokenType::ENUM:
            return parseEnumDecl(attributes, visibility, isConst, declModifiers, startPosition);
        case TokenType::EXTENSION:
            return parseExtensionDecl(attributes, visibility, isConst, declModifiers, startPosition);

        case TokenType::LET:
            printError("`let` cannot be used outside of function bodies or related! (use `static var` or `const var` instead)",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
            break;
        case TokenType::VAR: {
            _lexer.consumeType(TokenType::VAR);

            VariableDecl* result = parseVariableDecl(attributes, visibility, isConst, startPosition,
                                                     declModifiers);

            if (!_lexer.consumeType(TokenType::SEMICOLON)) {
                printError("expected closing `;` after variable declaration!",
                           _lexer.peekStartPosition(), _lexer.peekEndPosition());
            }

            return result;
        }
        default:
            if (_lexer.peekType() == TokenType::SYMBOL) {
                printError("unexpected token `" + _lexer.peekCurrentSymbol() + "`, "
                           "did you mean `var " + _lexer.peekCurrentSymbol() + "`?",
                           _lexer.peekStartPosition(), _lexer.peekEndPosition());
            } else {
                printError("unexpected token '" + _lexer.peekToken().currentSymbol + "'!",
                           _lexer.peekToken().startPosition, _lexer.peekToken().endPosition);
            }
            break;
    }

    return nullptr;
}

CallOperatorDecl* Parser::parseCallOperatorDecl(std::vector<Attr*> attributes, Decl::Visibility visibility,
                                                bool isConstExpr, DeclModifiers declModifiers,
                                                TextPosition startPosition) {
    Identifier callKeyword(_lexer.peekStartPosition(), _lexer.peekEndPosition(), "call");

    if (!_lexer.consumeType(TokenType::CALL)) {
        printError("expected `call`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    if (_lexer.peekType() == TokenType::LESS) {
        printError("unexpected `<` found after `call`, expected `(`! (note: `call` cannot have template arguments!)",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    if (_lexer.peekType() != TokenType::LPAREN) {
        printError("expected call parameters, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    TextPosition endPosition;

    std::vector<ParameterDecl*> parameters(parseParameters(&endPosition));
    Type* returnType = nullptr;

    if (_lexer.consumeType(TokenType::ARROW)) {
        returnType = parseType();

        endPosition = returnType->endPosition();
    }

    std::vector<Cont*> contracts(parseConts());
    CompoundStmt* body = nullptr;

    if (_lexer.consumeType(TokenType::SEMICOLON)) {
        // If there is a semicolon then the function is marked as a `prototype`, mainly used for `trait` parsing
        body = new CompoundStmt({}, {}, {});
        declModifiers |= DeclModifiers::Prototype;
    } else {
        if (_lexer.peekType() != TokenType::LCURLY) {
            printError("expected beginning `{` for call body, found `" + _lexer.peekCurrentSymbol() + "`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }

        body = parseCompoundStmt();
    }

    return new CallOperatorDecl(_fileID, std::move(attributes), visibility, isConstExpr, callKeyword,
                               declModifiers, parameters, returnType, contracts, body, startPosition, endPosition);
}

ConstructorDecl* Parser::parseConstructorDecl(std::vector<Attr*> attributes, Decl::Visibility visibility,
                                              bool isConstExpr,
                                              DeclModifiers declModifiers, TextPosition startPosition) {
    Identifier initKeyword(_lexer.peekStartPosition(), _lexer.peekEndPosition(), "init");
    ConstructorType constructorType = ConstructorType::Normal;

    if (!_lexer.consumeType(TokenType::INIT)) {
        printError("expected `init`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    // We support `copy` and `move` constructors by naming the constructor. These are special cases, there cannot be
    // custom named constructors besides `copy` and `move` for now.
    if (_lexer.peekType() == TokenType::SYMBOL) {
        if (_lexer.peekCurrentSymbol() == "move") {
            constructorType = ConstructorType::Move;
        } else if (_lexer.peekCurrentSymbol() == "copy") {
            constructorType = ConstructorType::Copy;
        } else {
            printError("unknown `init` type `" + _lexer.peekCurrentSymbol() + "`, only `move` and `copy` are accepted!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }

        _lexer.consumeType(TokenType::SYMBOL);
    }

    if (_lexer.peekType() == TokenType::LESS) {
        printError("unexpected `<` found after `init`, expected `(`! (note: `init` cannot have template arguments!)",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    if (_lexer.peekType() != TokenType::LPAREN) {
        printError("expected init parameters, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    TextPosition endPosition;

    std::vector<ParameterDecl*> parameters(parseParameters(&endPosition));
    std::vector<Cont*> contracts(parseConts());
    CompoundStmt* body = nullptr;

    if (_lexer.consumeType(TokenType::SEMICOLON)) {
        // If there is a semicolon then the function is marked as a `prototype`, mainly used for `trait` parsing
        body = new CompoundStmt({}, {}, {});
        declModifiers |= DeclModifiers::Prototype;
    } else {
        if (_lexer.peekType() != TokenType::LCURLY) {
            printError("expected beginning `{` for init body, found `" + _lexer.peekCurrentSymbol() + "`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }

        body = parseCompoundStmt();
    }

    return new ConstructorDecl(_fileID, std::move(attributes), visibility, isConstExpr, initKeyword,
                               declModifiers, parameters, contracts, body, startPosition, endPosition,
                               constructorType);
}

DestructorDecl* Parser::parseDestructorDecl(std::vector<Attr*> attributes, Decl::Visibility visibility,
                                            bool isConstExpr,
                                            DeclModifiers declModifiers, TextPosition startPosition) {
    Identifier deinitKeyword(_lexer.peekStartPosition(), _lexer.peekEndPosition(), "deinit");

    if (!_lexer.consumeType(TokenType::DEINIT)) {
        printError("expected `deinit`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    if (_lexer.peekType() == TokenType::LESS) {
        printError("unexpected `<` found after `deinit`, expected `(`! (note: `deinit` cannot have template arguments!)",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    TextPosition endPosition = deinitKeyword.endPosition();

    // Empty `()` are optional but there CANNOT be parameters.
    if (_lexer.consumeType(TokenType::LPAREN)) {
        endPosition = _lexer.peekEndPosition();

        if (!_lexer.consumeType(TokenType::RPAREN)) {
            printError("`deinit` cannot be provided parameters!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }
    }

    std::vector<Cont*> contracts(parseConts());
    CompoundStmt* body = nullptr;

    if (_lexer.consumeType(TokenType::SEMICOLON)) {
        // If there is a semicolon then the function is marked as a `prototype`, mainly used for `trait` parsing
        body = new CompoundStmt({}, {}, {});
        declModifiers |= DeclModifiers::Prototype;
    } else {
        if (_lexer.peekType() != TokenType::LCURLY) {
            printError("expected beginning `{` for deinit body, found `" + _lexer.peekCurrentSymbol() + "`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }

        body = parseCompoundStmt();
    }

    return new DestructorDecl(_fileID, std::move(attributes), visibility, isConstExpr, deinitKeyword,
                               declModifiers, contracts, body, startPosition, endPosition);
}

EnumDecl* Parser::parseEnumDecl(std::vector<Attr*> attributes, Decl::Visibility visibility, bool isConstExpr,
                                DeclModifiers declModifiers, TextPosition startPosition) {
    if (!_lexer.consumeType(TokenType::ENUM)) {
        printError("expected `enum`, found `" + _lexer.peekCurrentSymbol() = "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    if (_lexer.peekType() != TokenType::SYMBOL && _lexer.peekType() != TokenType::ATSYMBOL) {
        printError("expected enum identifier, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    Identifier enumIdentifier = parseIdentifier();
    // If there isn't a type specified we will default to the identifier end position
    TextPosition endPosition = enumIdentifier.endPosition();

    if (_lexer.peekType() == TokenType::LESS) {
        printError("enums cannot be templates!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    Type* constType = nullptr;

    if (_lexer.consumeType(TokenType::COLON)) {
        constType = parseType();
        endPosition = constType->endPosition();
    }

    if (!_lexer.consumeType(TokenType::LCURLY)) {
        printError("expected opening `{` for enum, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    std::vector<EnumConstDecl*> enumConsts;

    while (_lexer.peekType() != TokenType::RCURLY && _lexer.peekType() != TokenType::ENDOFFILE) {
        std::vector<Attr*> enumConstAttrs = parseAttrs();

        if (_lexer.peekType() != TokenType::SYMBOL && _lexer.peekType() != TokenType::ATSYMBOL) {
            printError("expected enum const identifier, found `" + _lexer.peekCurrentSymbol() + "`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }

        TextPosition enumConstStartPosition = _lexer.peekStartPosition();
        Identifier enumConstIdentifier = parseIdentifier();
        TextPosition enumConstEndPosition = enumConstIdentifier.endPosition();
        Expr* enumConstValue = nullptr;

        if (_lexer.consumeType(TokenType::EQUALS)) {
            enumConstValue = parseExpr();
            enumConstEndPosition = enumConstValue->endPosition();
        }

        enumConsts.push_back(new EnumConstDecl(_fileID, enumConstAttrs, enumConstIdentifier,
                                               enumConstStartPosition, enumConstEndPosition,
                                               enumConstValue));

        if (!_lexer.consumeType(TokenType::COMMA)) {
            break;
        }
    }

    if (!_lexer.consumeType(TokenType::RCURLY)) {
        printError("expected closing `}` for enum, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    return new EnumDecl(_fileID, std::move(attributes), enumIdentifier, startPosition, endPosition,
                        constType, enumConsts);
}

ExtensionDecl* Parser::parseExtensionDecl(std::vector<Attr*> attributes, Decl::Visibility visibility, bool isConstExpr,
                                          DeclModifiers declModifiers, TextPosition startPosition) {
    if (!_lexer.consumeType(TokenType::EXTENSION)) {
        printError("expected `extension`, found `" + _lexer.peekCurrentSymbol() = "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    std::vector<TemplateParameterDecl*> templateParameters;

    if (_lexer.peekType() == TokenType::LESS) {
        templateParameters = parseTemplateParameters();
    }

    Type* typeToExtend = parseType();
    TextPosition endPosition = typeToExtend->endPosition();
    std::vector<Type*> inheritedTypes;

    if (_lexer.consumeType(TokenType::COLON)) {
        while (true) {
            inheritedTypes.push_back(parseType());

            if (!_lexer.consumeType(TokenType::COMMA)) {
                break;
            }
        }
    }

    // NOTE: Only template structs can have contracts, non-templates don't have anything that could be contractual
    std::vector<Cont*> contracts(parseConts());

    if (!_lexer.consumeType(TokenType::LCURLY)) {
        printError("expected beginning `{` for `extension`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    std::vector<ConstructorDecl*> constructors;
//    DestructorDecl* destructor = nullptr;
    std::vector<Decl*> members;

    while (_lexer.peekType() != TokenType::RCURLY && _lexer.peekType() != TokenType::ENDOFFILE) {
        Decl* parsedMember = parseDecl();

        if (parsedMember->getDeclKind() == Decl::Kind::Constructor) {
            constructors.push_back(llvm::dyn_cast<ConstructorDecl>(parsedMember));
        } else if (parsedMember->getDeclKind() == Decl::Kind::Destructor) {
            printError("extensions cannot define destructors!",
                       parsedMember->startPosition(), parsedMember->endPosition());
//            if (destructor != nullptr) {
//                printError("there cannot be more than one `deinit` at per extension!",
//                           _lexer.peekStartPosition(), _lexer.peekEndPosition());
//            }
//
//            destructor = llvm::dyn_cast<DestructorDecl>(parsedMember);
        } else {
            members.push_back(parsedMember);
        }
    }

    if (!_lexer.consumeType(TokenType::RCURLY)) {
        printError("expected ending `}` for `extension`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    return new ExtensionDecl(_fileID, std::move(attributes), visibility, isConstExpr, declModifiers,
                             templateParameters, typeToExtend, startPosition, endPosition, inheritedTypes,
                             contracts, members, constructors);
}

FunctionDecl* Parser::parseFunctionDecl(std::vector<Attr*> attributes, Decl::Visibility visibility, bool isConstExpr,
                                        DeclModifiers declModifiers, TextPosition startPosition) {
    if (!_lexer.consumeType(TokenType::FUNC)) {
        printError("expected `func`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    TextPosition endPosition;

    Identifier funcName = parseIdentifier();
    std::vector<TemplateParameterDecl*> templateParameters;

    if (_lexer.peekType() == TokenType::LESS) {
        templateParameters = parseTemplateParameters();
    }

    std::vector<ParameterDecl*> parameters = parseParameters(&endPosition);
    Type* returnType = nullptr;

    if (_lexer.consumeType(TokenType::ARROW)) {
        returnType = parseType();

        endPosition = returnType->endPosition();
    }

    std::vector<Cont*> contracts(parseConts());
    CompoundStmt* body = nullptr;

    if (_lexer.consumeType(TokenType::SEMICOLON)) {
        // If there is a semicolon then the function is marked as a `prototype`, mainly used for `trait` parsing
        body = new CompoundStmt({}, {}, {});
        declModifiers |= DeclModifiers::Prototype;
    } else {
        if (_lexer.peekType() != TokenType::LCURLY) {
            printError("expected beginning `{` for `func` declaration, found `" + _lexer.peekCurrentSymbol() + "`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }

        body = parseCompoundStmt();
    }

    if (templateParameters.empty()) {
        return new FunctionDecl(_fileID, std::move(attributes), visibility, isConstExpr, funcName,
                                declModifiers, parameters, returnType, contracts, body, startPosition, endPosition);
    } else {
        return new TemplateFunctionDecl(_fileID, std::move(attributes), visibility, isConstExpr, funcName,
                                        declModifiers, parameters, returnType, contracts, body,
                                        startPosition, endPosition, templateParameters);
    }
}

ImportDecl* Parser::parseImportDecl(std::vector<Attr*> attributes, TextPosition startPosition) {
    TextPosition importStartPosition = _lexer.peekStartPosition();
    TextPosition importEndPosition = _lexer.peekEndPosition();

    if (!_lexer.consumeType(TokenType::IMPORT)) {
        printError("expected `import`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   startPosition, _lexer.peekEndPosition());
    }

    std::vector<Identifier> importPath = parseDotSeparatedIdentifiers();

    if (_lexer.peekType() == TokenType::AS) {
        TextPosition asStartPosition = _lexer.peekStartPosition();
        TextPosition asEndPosition = _lexer.peekEndPosition();

        _lexer.consumeType(TokenType::AS);

        if (_lexer.peekType() != TokenType::SYMBOL) {
            printError("expected import alias identifier after `as`, found `" + _lexer.peekCurrentSymbol() + "`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }

        Identifier importAlias = parseIdentifier();

        if (!_lexer.consumeType(TokenType::SEMICOLON)) {
            printError("expected `;` to end import, found `" + _lexer.peekCurrentSymbol() + "`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }

        return new ImportDecl(_fileID, std::move(attributes), importStartPosition, importEndPosition,
                              importPath, asStartPosition, asEndPosition, importAlias);
    } else {
        if (!_lexer.consumeType(TokenType::SEMICOLON)) {
            printError("expected `;` to end import, found `" + _lexer.peekCurrentSymbol() + "`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }

        return new ImportDecl(_fileID, std::move(attributes), importStartPosition, importEndPosition,
                              importPath);
    }
}

NamespaceDecl* Parser::parseNamespaceDecl(std::vector<Attr*> attributes) {
    TextPosition startPosition = _lexer.peekStartPosition();
    TextPosition endPosition = _lexer.peekEndPosition();

    _lexer.consumeType(TokenType::NAMESPACE);

    if (_lexer.peekType() != TokenType::SYMBOL) {
        printError("expected namespace name after `namespace`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    Identifier rootIdentifier = parseIdentifier();
    NamespaceDecl* rootNamespace = new NamespaceDecl(_fileID, {}, rootIdentifier, startPosition, endPosition);
    NamespaceDecl* currentNamespace = rootNamespace;

    while (_lexer.consumeType(TokenType::PERIOD)) {
        if (_lexer.peekType() != TokenType::SYMBOL) {
            printError("expected namespace name, found `" + _lexer.peekCurrentSymbol() + "`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }

        Identifier namespaceIdentifier = parseIdentifier();
        NamespaceDecl* newNamespace = new NamespaceDecl(_fileID, {}, namespaceIdentifier,
                                                        startPosition, endPosition);

        currentNamespace->addNestedDecl(newNamespace);
        currentNamespace = newNamespace;
    }

    currentNamespace->setAttributes(std::move(attributes));

    if (!_lexer.consumeType(TokenType::LCURLY)) {
        printError("expected beginning `{` for namespace, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    while (_lexer.peekType() != TokenType::RCURLY && _lexer.peekType() != TokenType::ENDOFFILE) {
        currentNamespace->addNestedDecl(parseDecl());
    }

    if (!_lexer.consumeType(TokenType::RCURLY)) {
        printError("expected ending `}` for namespace, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    return rootNamespace;
}

OperatorDecl* Parser::parseOperatorDecl(std::vector<Attr*> attributes, Decl::Visibility visibility, bool isConstExpr,
                                        DeclModifiers declModifiers, TextPosition startPosition) {
    if (!_lexer.consumeType(TokenType::OPERATOR)) {
        printError("expected `operator`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    OperatorType operatorType = OperatorType::Unknown;

    if (_lexer.consumeType(TokenType::PREFIX)) {
        operatorType = OperatorType::Prefix;
    } else if (_lexer.consumeType(TokenType::INFIX)) {
        operatorType = OperatorType::Infix;
    } else if (_lexer.consumeType(TokenType::POSTFIX)) {
        operatorType = OperatorType::Postfix;
    } else {
        printError("unexpected token after `operator`, found `" + _lexer.peekCurrentSymbol() + "`! "
                   "(expected `prefix`, `infix`, or `postfix`",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    TextPosition endPosition;
    Identifier operatorIdentifier(_lexer.peekStartPosition(), _lexer.peekEndPosition(), _lexer.peekCurrentSymbol());

    if (_lexer.peekMeta() != TokenMetaType::OPERATOR && _lexer.peekType() != TokenType::SYMBOL) {
        printError("expected operator but found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    } else {
        _lexer.consumeType(_lexer.peekType());
    }

    std::vector<ParameterDecl*> parameters = parseParameters(&endPosition);
    Type* returnType = nullptr;

    if (_lexer.consumeType(TokenType::ARROW)) {
        returnType = parseType();

        endPosition = returnType->endPosition();
    }

    std::vector<Cont*> contracts(parseConts());
    CompoundStmt* body = nullptr;

    if (_lexer.consumeType(TokenType::SEMICOLON)) {
        // If there is a semicolon then the function is marked as a `prototype`, mainly used for `trait` parsing
        body = new CompoundStmt({}, {}, {});
        declModifiers |= DeclModifiers::Prototype;
    } else {
        if (_lexer.peekType() != TokenType::LCURLY) {
            printError("expected beginning `{` for `operator` declaration, found `" + _lexer.peekCurrentSymbol() + "`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }

        body = parseCompoundStmt();
    }

    return new OperatorDecl(_fileID, std::move(attributes), visibility, isConstExpr, operatorType, operatorIdentifier,
                            declModifiers, parameters, returnType, contracts, body, startPosition, endPosition);
}

std::vector<TemplateParameterDecl*> Parser::parseTemplateParameters() {
    // TODO: Support default values
    if (!_lexer.consumeType(TokenType::LESS)) {
        printError("expected beginning `<` for template parameters, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    std::vector<TemplateParameterDecl*> templateParameters;

    bool oldRightShiftEnabledValue = _lexer.getRightShiftState();
    _lexer.setRightShiftState(false);

    while (_lexer.peekType() != TokenType::TEMPLATEEND && _lexer.peekType() != TokenType::ENDOFFILE) {
        if (_lexer.peekType() == TokenType::LSQUARE) {
            printError("template parameters cannot have attributes!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }

        if (_lexer.consumeType(TokenType::CONST)) {
            Identifier constIdentifier = parseIdentifier();

            if (!_lexer.consumeType(TokenType::COLON)) {
                printError("template const parameters MUST have a type!",
                           _lexer.peekStartPosition(), _lexer.peekEndPosition());
            }

            Type* type = parseType();

            templateParameters.push_back(new TemplateParameterDecl(_fileID, {}, constIdentifier, type,
                                                                   constIdentifier.startPosition(),
                                                                   type->endPosition()));
        } else {
            Identifier typenameIdentifier = parseIdentifier();

            if (_lexer.peekType() == TokenType::COLON) {
                printError("template typename parameters cannot have types! (did you mean `const " +
                           typenameIdentifier.name() + ": ...` instead?)",
                           _lexer.peekStartPosition(), _lexer.peekEndPosition());
            }

            templateParameters.push_back(new TemplateParameterDecl(_fileID, {}, typenameIdentifier,
                                                                   typenameIdentifier.startPosition(),
                                                                   typenameIdentifier.endPosition()));
        }

        if (!_lexer.consumeType(TokenType::COMMA)) {
            break;
        }
    }

    if (!_lexer.consumeType(TokenType::TEMPLATEEND)) {
        printError("expected `,` or `>` for template parameters, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    _lexer.setRightShiftState(oldRightShiftEnabledValue);

    return templateParameters;
}

std::vector<ParameterDecl*> Parser::parseParameters(TextPosition* paramsEndPosition) {
    if (!_lexer.consumeType(TokenType::LPAREN)) {
        printError("expected beginning `(` for parameters, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    std::vector<ParameterDecl*> parameters;

    while (_lexer.peekType() != TokenType::RPAREN && _lexer.peekType() != TokenType::ENDOFFILE) {
        std::vector<Attr*> attributes = parseAttrs();

        TextPosition startPosition = _lexer.peekStartPosition();

        if (_lexer.peekType() == TokenType::IN ||
            _lexer.peekType() == TokenType::OUT ||
            _lexer.peekType() == TokenType::INOUT ||
            _lexer.peekType() == TokenType::MUT ||
            _lexer.peekType() == TokenType::IMMUT ||
            _lexer.peekType() == TokenType::CONST) {
            printError("`in`, `out`, `inout`, `const`, `mut`, and `immut` must be placed before the parameter type! "
                       "(if this was meant to be the argument label prefix it with `@` instead)",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }

        // If there is an `@` it will parse it, if not nothing will happen and we keep going.
        _lexer.consumeType(TokenType::ATSYMBOL);

        Identifier argumentLabel;

        if (_lexer.peekMeta() == TokenMetaType::KEYWORD || _lexer.peekMeta() == TokenMetaType::MODIFIER ||
            _lexer.peekType() == TokenType::SYMBOL) {
            argumentLabel = Identifier(_lexer.peekStartPosition(),
                                       _lexer.peekEndPosition(),
                                       _lexer.peekCurrentSymbol());

            _lexer.consumeType(_lexer.peekType());
        // Since argument labels are optional we check to see if there is a `:` as well as a symbol.
        } else if (_lexer.peekType() != TokenType::SYMBOL && _lexer.peekType() != TokenType::COLON) {
            printError("expected argument label or parameter name, found `" + _lexer.peekCurrentSymbol() + "`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
            return {};
        }

        Identifier paramName;

        // If there is a `:` then the argument label is treated to be the same as the parameter name
        if (_lexer.consumeType(TokenType::COLON)) {
            paramName = argumentLabel;
        } else {
            paramName = parseIdentifier();

            if (!_lexer.consumeType(TokenType::COLON)) {
                printError("expected `:` after parameter name `" + paramName.name() + "`, found `" + _lexer.peekCurrentSymbol() + "`!",
                           _lexer.peekStartPosition(), _lexer.peekEndPosition());
            }
        }

        ParameterDecl::ParameterKind parameterKind = ParameterDecl::ParameterKind::Val;

        if (_lexer.consumeType(TokenType::IN)) {
            parameterKind = ParameterDecl::ParameterKind::In;
        } else if (_lexer.consumeType(TokenType::OUT)) {
            parameterKind = ParameterDecl::ParameterKind::Out;
        } else if (_lexer.peekType() == TokenType::IMMUT) {
            printError("redundant `immut`, parameters are `immut` by default!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        } else if (_lexer.peekType() == TokenType::CONST) {
            printError("`const` cannot be used in this context!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }

        Type* paramType = parseType();
        Expr* defaultValue = nullptr;

        TextPosition endPosition = _lexer.peekStartPosition();

        if (_lexer.consumeType(TokenType::EQUALS)) {
            defaultValue = parseExpr();

            endPosition = defaultValue->endPosition();
        }

        parameters.emplace_back(new ParameterDecl(_fileID, attributes, argumentLabel, paramName, paramType, defaultValue,
                                                  parameterKind, startPosition, endPosition));

        if (!_lexer.consumeType(TokenType::COMMA)) {
            break;
        }
    }

    *paramsEndPosition = _lexer.peekEndPosition();

    if (!_lexer.consumeType(TokenType::RPAREN)) {
        printError("expected ending `)` for parameters, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    return parameters;
}

PropertyDecl* Parser::parsePropertyDecl(std::vector<Attr*> attributes, Decl::Visibility visibility, bool isConstExpr,
                                        DeclModifiers declModifiers, TextPosition startPosition) {
    if (!_lexer.consumeType(TokenType::PROPERTY)) {
        printError("expected `property`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    Identifier propertyIdentifier = parseIdentifier();

    if (!_lexer.consumeType(TokenType::COLON)) {
        printError("expected `:` after property name, properties MUST have a type specified!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    Type* propertyType = parseType();
    std::vector<PropertyGetDecl*> getters;
    PropertySetDecl* setter = nullptr;

    if (!_lexer.consumeType(TokenType::LCURLY)) {
        printError("expected opening `{` for property, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    while (_lexer.peekType() != TokenType::RCURLY && _lexer.peekType() != TokenType::ENDOFFILE) {
        TextPosition getSetStartPosition = _lexer.peekStartPosition();

        std::vector<Attr*> getSetAttributes = parseAttrs();
        Decl::Visibility getSetVisibility = parseDeclVisibility();
        bool isConst = false;
        DeclModifiers getSetModifiers = parseDeclModifiers(&isConst);
        std::string const& getOrSet = _lexer.peekCurrentSymbol();

        if (getOrSet == "get") {
            Identifier getIdentifier(_lexer.peekStartPosition(), _lexer.peekEndPosition(), "get");
            TextPosition getEndPosition = _lexer.peekEndPosition();

            _lexer.consumeType(TokenType::SYMBOL);

            PropertyGetDecl::GetResult getResultType = PropertyGetDecl::GetResult::Normal;
            Type* getReturnType = propertyType->deepCopy();

            // Support for `get ref` and `get ref mut`
            if (_lexer.consumeType(TokenType::REF)) {
                if (_lexer.consumeType(TokenType::MUT)) {
                    getReturnType = new ReferenceType(Type::Qualifier::Mut, getReturnType);

                    getResultType = PropertyGetDecl::GetResult::RefMut;
                } else {
                    getReturnType = new ReferenceType(Type::Qualifier::Immut, getReturnType);

                    getResultType = PropertyGetDecl::GetResult::Ref;
                }
            }

            std::vector<Cont*> contracts = parseConts();
            CompoundStmt* getterBody = nullptr;

            if (_lexer.consumeType(TokenType::SEMICOLON)) {
                // If there is a semicolon then the function is marked as a `prototype`, mainly used for `trait` parsing
                getterBody = new CompoundStmt({}, {}, {});
                declModifiers |= DeclModifiers::Prototype;
            } else {
                if (_lexer.peekType() != TokenType::LCURLY) {
                    printError("expected beginning `{` for property `get` body!",
                               _lexer.peekStartPosition(), _lexer.peekEndPosition());
                }

                getterBody = parseCompoundStmt();
            }

            getters.push_back(new PropertyGetDecl(_fileID, getSetAttributes, getSetVisibility, isConst,
                                                  getIdentifier, getSetModifiers, getReturnType, contracts, getterBody,
                                                  getSetStartPosition, getEndPosition, getResultType));
        } else if (getOrSet == "set") {
            if (setter != nullptr) {
                printError("duplicate `set` found! (there can only be one `set` body per property)",
                           _lexer.peekStartPosition(), _lexer.peekEndPosition());
            }

            Identifier setIdentifier(_lexer.peekStartPosition(), _lexer.peekEndPosition(), "get");
            TextPosition setEndPosition = _lexer.peekEndPosition();

            _lexer.consumeType(TokenType::SYMBOL);

            std::vector<Cont*> contracts = parseConts();
            CompoundStmt* setterBody = nullptr;

            if (_lexer.consumeType(TokenType::SEMICOLON)) {
                // If there is a semicolon then the function is marked as a `prototype`, mainly used for `trait` parsing
                setterBody = new CompoundStmt({}, {}, {});
                declModifiers |= DeclModifiers::Prototype;
            } else {
                if (_lexer.peekType() != TokenType::LCURLY) {
                    printError("expected beginning `{` for property `set` body!",
                               _lexer.peekStartPosition(), _lexer.peekEndPosition());
                }

                setterBody = parseCompoundStmt();
            }

            setter = new PropertySetDecl(_fileID, getSetAttributes, getSetVisibility, isConstExpr, setIdentifier,
                                         getSetModifiers, propertyType->deepCopy(), contracts, setterBody,
                                         getSetStartPosition, setEndPosition);
        } else {
            printError("unknown keyword `" + getOrSet + "`, expected `get` or `set`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }
    }

    if (!_lexer.consumeType(TokenType::RCURLY)) {
        printError("expected closing `}` for property, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    return new PropertyDecl(_fileID, std::move(attributes), visibility, isConstExpr, propertyIdentifier, propertyType,
                            startPosition, propertyType->endPosition(), declModifiers, getters, setter);
}

StructDecl* Parser::parseStructDecl(std::vector<Attr*> attributes, Decl::Visibility visibility, bool isConstExpr,
                                    TextPosition startPosition, DeclModifiers declModifiers,
                                    StructDecl::Kind structKind) {
    std::string errorName;

    if (structKind == StructDecl::Kind::Class) {
        if (!_lexer.consumeType(TokenType::CLASS)) {
            printError("expected `class`, found `" + _lexer.peekCurrentSymbol() + "`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }

        errorName = "class";
    } else if (structKind == StructDecl::Kind::Struct) {
        if (!_lexer.consumeType(TokenType::STRUCT)) {
            printError("expected `struct`, found `" + _lexer.peekCurrentSymbol() + "`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }

        errorName = "struct";
    } else if (structKind == StructDecl::Kind::Union) {
        if (!_lexer.consumeType(TokenType::UNION)) {
            printError("expected `union`, found `" + _lexer.peekCurrentSymbol() + "`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }

        errorName = "union";
    }

    if (_lexer.peekType() != TokenType::SYMBOL && _lexer.peekType() != TokenType::ATSYMBOL) {
        printError("expected identifier after `" + errorName + "`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    Identifier name = parseIdentifier();
    TextPosition endPosition = name.endPosition();

    std::vector<TemplateParameterDecl*> templateParameters;

    if (_lexer.peekType() == TokenType::LESS) {
        templateParameters = parseTemplateParameters();
    }

    std::vector<Type*> inheritedTypes;

    if (_lexer.consumeType(TokenType::COLON)) {
        while (true) {
            inheritedTypes.push_back(parseType());

            if (!_lexer.consumeType(TokenType::COMMA)) {
                break;
            }
        }
    }

    // NOTE: Only template structs can have contracts, non-templates don't have anything that could be contractual
    std::vector<Cont*> contracts(parseConts());

    if (!_lexer.consumeType(TokenType::LCURLY)) {
        printError("expected beginning `{` for " + errorName + " `" + name.name() + "`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    std::vector<ConstructorDecl*> constructors;
    DestructorDecl* destructor = nullptr;
    std::vector<Decl*> members;

    while (_lexer.peekType() != TokenType::RCURLY && _lexer.peekType() != TokenType::ENDOFFILE) {
        Decl* parsedMember = parseDecl();

        if (parsedMember->getDeclKind() == Decl::Kind::Constructor) {
            constructors.push_back(llvm::dyn_cast<ConstructorDecl>(parsedMember));
        } else if (parsedMember->getDeclKind() == Decl::Kind::Destructor) {
            if (destructor != nullptr) {
                printError("there cannot be more than one `deinit` at per " + errorName + "!",
                           _lexer.peekStartPosition(), _lexer.peekEndPosition());
            }

            destructor = llvm::dyn_cast<DestructorDecl>(parsedMember);
        } else {
            members.push_back(parsedMember);
        }
    }

    if (!_lexer.consumeType(TokenType::RCURLY)) {
        printError("expected ending `}` for " + errorName + " `" + name.name() + "`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    if (templateParameters.empty()) {
        return new StructDecl(_fileID, std::move(attributes), visibility, isConstExpr, name, declModifiers,
                              startPosition, endPosition, structKind, inheritedTypes, contracts, members, constructors,
                              destructor);
    } else {
        return new TemplateStructDecl(_fileID, std::move(attributes), visibility, isConstExpr, name, declModifiers,
                                      startPosition, endPosition, structKind, inheritedTypes, contracts, members,
                                      constructors, destructor, templateParameters);
    }
}

SubscriptOperatorDecl* Parser::parseSubscriptOperator(std::vector<Attr*> attributes, Decl::Visibility visibility,
                                                      bool isConstExpr, TextPosition startPosition,
                                                      DeclModifiers declModifiers) {
    Identifier subscriptKeyword(_lexer.peekStartPosition(), _lexer.peekEndPosition(),
                                "subscript");

    if (!_lexer.consumeType(TokenType::SUBSCRIPT)) {
        printError("expected `subscript`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    TextPosition endPosition;
    std::vector<ParameterDecl*> parameters = parseParameters(&endPosition);

    if (!_lexer.consumeType(TokenType::ARROW)) {
        printError("expected `->` for subscript type, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    Type* subscriptType = parseType();
    std::vector<SubscriptOperatorGetDecl*> getters;
    SubscriptOperatorSetDecl* setter = nullptr;

    if (!_lexer.consumeType(TokenType::LCURLY)) {
        printError("expected opening `{` for subscript, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    while (_lexer.peekType() != TokenType::RCURLY && _lexer.peekType() != TokenType::ENDOFFILE) {
        TextPosition getSetStartPosition = _lexer.peekStartPosition();

        std::vector<Attr*> getSetAttributes = parseAttrs();
        Decl::Visibility getSetVisibility = parseDeclVisibility();
        bool isConst = false;
        DeclModifiers getSetModifiers = parseDeclModifiers(&isConst);
        std::string const& getOrSet = _lexer.peekCurrentSymbol();

        if (getOrSet == "get") {
            Identifier getIdentifier(_lexer.peekStartPosition(), _lexer.peekEndPosition(), "get");
            TextPosition getEndPosition = _lexer.peekEndPosition();

            _lexer.consumeType(TokenType::SYMBOL);

            SubscriptOperatorGetDecl::GetResult getResultType = SubscriptOperatorGetDecl::GetResult::Normal;
            Type* getReturnType = subscriptType->deepCopy();

            // Support for `get ref` and `get ref mut`
            if (_lexer.consumeType(TokenType::REF)) {
                if (_lexer.consumeType(TokenType::MUT)) {
                    getReturnType = new ReferenceType(Type::Qualifier::Mut, getReturnType);

                    getResultType = SubscriptOperatorGetDecl::GetResult::RefMut;
                } else {
                    getReturnType = new ReferenceType(Type::Qualifier::Immut, getReturnType);

                    getResultType = SubscriptOperatorGetDecl::GetResult::Ref;
                }
            }

            std::vector<Cont*> contracts = parseConts();
            CompoundStmt* getterBody = nullptr;

            if (_lexer.consumeType(TokenType::SEMICOLON)) {
                // If there is a semicolon then the function is marked as a `prototype`, mainly used for `trait` parsing
                getterBody = new CompoundStmt({}, {}, {});
                declModifiers |= DeclModifiers::Prototype;
            } else {
                if (_lexer.peekType() != TokenType::LCURLY) {
                    printError("expected beginning `{` for subscript `get` body!",
                               _lexer.peekStartPosition(), _lexer.peekEndPosition());
                }

                getterBody = parseCompoundStmt();
            }

            getters.push_back(new SubscriptOperatorGetDecl(_fileID, getSetAttributes, getSetVisibility, isConst,
                                                  getIdentifier, getSetModifiers, getReturnType, contracts, getterBody,
                                                  getSetStartPosition, getEndPosition, getResultType));
        } else if (getOrSet == "set") {
            if (setter != nullptr) {
                printError("duplicate `set` found! (there can only be one `set` body per subscript)",
                           _lexer.peekStartPosition(), _lexer.peekEndPosition());
            }

            Identifier setIdentifier(_lexer.peekStartPosition(), _lexer.peekEndPosition(), "get");
            TextPosition setEndPosition = _lexer.peekEndPosition();

            _lexer.consumeType(TokenType::SYMBOL);

            std::vector<Cont*> contracts = parseConts();
            CompoundStmt* setterBody = nullptr;

            if (_lexer.consumeType(TokenType::SEMICOLON)) {
                // If there is a semicolon then the function is marked as a `prototype`, mainly used for `trait` parsing
                setterBody = new CompoundStmt({}, {}, {});
                declModifiers |= DeclModifiers::Prototype;
            } else {
                if (_lexer.peekType() != TokenType::LCURLY) {
                    printError("expected beginning `{` for subscript `set` body!",
                               _lexer.peekStartPosition(), _lexer.peekEndPosition());
                }

                setterBody = parseCompoundStmt();
            }

            setter = new SubscriptOperatorSetDecl(_fileID, getSetAttributes, getSetVisibility, isConstExpr,
                                                  setIdentifier, getSetModifiers, subscriptType->deepCopy(), contracts,
                                                  setterBody, getSetStartPosition, setEndPosition);
        } else {
            printError("unknown keyword `" + getOrSet + "`, expected `get` or `set`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }
    }

    if (!_lexer.consumeType(TokenType::RCURLY)) {
        printError("expected closing `}` for subscript, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    return new SubscriptOperatorDecl(_fileID, std::move(attributes), visibility, isConstExpr, subscriptKeyword,
                                     parameters, subscriptType, startPosition, endPosition, declModifiers, getters,
                                     setter);
}

TraitDecl* Parser::parseTraitDecl(std::vector<Attr*> attributes, Decl::Visibility visibility, bool isConstExpr,
                                  TextPosition startPosition, DeclModifiers declModifiers) {
    if (!_lexer.consumeType(TokenType::TRAIT)) {
        printError("expected `trait`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    if (_lexer.peekType() != TokenType::SYMBOL && _lexer.peekType() != TokenType::ATSYMBOL) {
        printError("expected identifier after `trait`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    Identifier name = parseIdentifier();
    TextPosition endPosition = name.endPosition();

    std::vector<TemplateParameterDecl*> templateParameters;

    if (_lexer.peekType() == TokenType::LESS) {
        templateParameters = parseTemplateParameters();
    }

    std::vector<Type*> inheritedTypes;

    if (_lexer.consumeType(TokenType::COLON)) {
        while (true) {
            inheritedTypes.push_back(parseType());

            if (!_lexer.consumeType(TokenType::COMMA)) {
                break;
            }
        }
    }

    // NOTE: Only template traits can have contracts, non-templates don't have anything that could be contractual
    std::vector<Cont*> contracts(parseConts());

    if (!_lexer.consumeType(TokenType::LCURLY)) {
        printError("expected beginning `{` for trait `" + name.name() + "`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    std::vector<Decl*> members;

    while (_lexer.peekType() != TokenType::RCURLY && _lexer.peekType() != TokenType::ENDOFFILE) {
        Decl* parsedMember = parseDecl();

        members.push_back(parsedMember);
    }

    if (!_lexer.consumeType(TokenType::RCURLY)) {
        printError("expected ending `}` for trait `" + name.name() + "`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    if (templateParameters.empty()) {
        return new TraitDecl(_fileID, std::move(attributes), visibility, isConstExpr, name, declModifiers,
                             startPosition, endPosition, inheritedTypes, contracts, members);
    } else {
        return new TemplateTraitDecl(_fileID, std::move(attributes), visibility, isConstExpr, name, declModifiers,
                                     startPosition, endPosition, inheritedTypes, contracts, members,
                                     templateParameters);
    }
}

TypeAliasDecl* Parser::parseTypeAliasDecl(std::vector<Attr*> attributes, Decl::Visibility visibility,
                                          TextPosition startPosition) {
    if (!_lexer.consumeType(TokenType::TYPEALIAS)) {
        printError("expected `typealias`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    TypeAliasType typeAliasType;
    Identifier aliasIdentifier;

    if (_lexer.consumeType(TokenType::PREFIX)) {
        typeAliasType = TypeAliasType::Prefix;

        TextPosition aliasStartPosition = _lexer.peekStartPosition();

        // TODO: Support custom prefix operators such as `?`, `^`, `&`, `%`, etc.
        if (_lexer.consumeType(TokenType::LSQUARE)) {
            // Dimension type
            TextPosition aliasEndPosition = _lexer.peekEndPosition();

            if (!_lexer.consumeType(TokenType::RSQUARE)) {
                printError("expected `]` for `prefix []`, found `" + _lexer.peekCurrentSymbol() + "`!",
                           _lexer.peekStartPosition(), _lexer.peekEndPosition());
            }

            aliasIdentifier = Identifier(aliasStartPosition, aliasEndPosition, "[]");
        } else {
            printError("unexpected token after `typealias prefix`, expected `[]` but found `" + _lexer.peekCurrentSymbol() + "`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }
    } else {
        typeAliasType = TypeAliasType::Normal;

        if (_lexer.peekType() != TokenType::SYMBOL && _lexer.peekType() != TokenType::ATSYMBOL) {
            printError("expected identifier after `trait`, found `" + _lexer.peekCurrentSymbol() + "`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }

        aliasIdentifier = parseIdentifier();
    }
    std::vector<TemplateParameterDecl*> templateParameters;

    if (_lexer.peekType() == TokenType::LESS) {
        templateParameters = parseTemplateParameters();
    }

    if (!_lexer.consumeType(TokenType::EQUALS)) {
        printError("expected `=` for `typealias`, found `" + _lexer.peekCurrentSymbol() = "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    Type* typeValue = parseType();

    TextPosition endPosition = _lexer.peekEndPosition();

    if (!_lexer.consumeType(TokenType::SEMICOLON)) {
        printError("expected `;` to end `typealias`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    return new TypeAliasDecl(_fileID, std::move(attributes), visibility, typeAliasType, aliasIdentifier,
                             templateParameters, typeValue, startPosition, endPosition);
}

TypeSuffixDecl* Parser::parseTypeSuffixDecl(std::vector<Attr*> attributes, Decl::Visibility visibility,
                                            bool isConstExpr, DeclModifiers declModifiers,
                                            TextPosition startPosition) {
    if (!_lexer.consumeType(TokenType::TYPESUFFIX)) {
        printError("expected `typesuffix`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    if (_lexer.peekType() != TokenType::SYMBOL) {
        printError("expected `typesuffix` identifier, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    Identifier suffixIdentifier = parseIdentifier();

    if (_lexer.peekType() != TokenType::LPAREN) {
        printError("expected `(` for `typesuffix` parameters, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    TextPosition endPosition;
    std::vector<ParameterDecl*> parameters = parseParameters(&endPosition);

    if (!_lexer.consumeType(TokenType::ARROW)) {
        printError("expected `->` for `typesuffix` type, found `" + _lexer.peekCurrentSymbol() + "`! "
                   "(NOTE: `typesuffix` MUST have a return type)",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    Type* resultType = parseType();
    std::vector<Cont*> contracts = parseConts();
    CompoundStmt* body = nullptr;

    if (_lexer.consumeType(TokenType::SEMICOLON)) {
        // If there is a semicolon then the function is marked as a `prototype`, mainly used for `trait` parsing
        body = new CompoundStmt({}, {}, {});
        declModifiers |= DeclModifiers::Prototype;
    } else {
        if (_lexer.peekType() != TokenType::LCURLY) {
            printError("expected beginning `{` for `typesuffix` declaration, found `" + _lexer.peekCurrentSymbol() + "`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }

        body = parseCompoundStmt();
    }

    return new TypeSuffixDecl(_fileID, std::move(attributes), visibility, isConstExpr, suffixIdentifier, declModifiers,
                              parameters, resultType, contracts, body, startPosition, endPosition);
}

VariableDecl* Parser::parseVariableDecl(std::vector<Attr*> attributes, Decl::Visibility visibility, bool isConstExpr,
                                        TextPosition startPosition, DeclModifiers declModifiers) {
    if (_lexer.peekType() != TokenType::SYMBOL && _lexer.peekType() != TokenType::ATSYMBOL) {
        printError("expected variable identifier, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    Identifier variableIdentifier = parseIdentifier();
    // If there isn't a type specified we will default to the identifier end position
    TextPosition endPosition = variableIdentifier.endPosition();
    Type* variableType = nullptr;

    if (_lexer.consumeType(TokenType::COLON)) {
        variableType = parseType();
        endPosition = variableType->endPosition();
    }

    Expr* initialValue = nullptr;

    if (_lexer.consumeType(TokenType::EQUALS)) {
        initialValue = parseExpr();
    }

    return new VariableDecl(_fileID, std::move(attributes), visibility, isConstExpr, variableIdentifier, declModifiers,
                            variableType, initialValue, startPosition, endPosition);
}

// Contracts ----------------------------------------------------------------------------------------------------------
std::vector<Cont*> Parser::parseConts() {
    std::vector<Cont*> result;

    while (true) {
        switch (_lexer.peekType()) {
            case TokenType::REQUIRES:
                result.push_back(parseRequiresCont());
                break;
            case TokenType::ENSURES:
                result.push_back(parseEnsuresCont());
                break;
            case TokenType::THROWS:
                result.push_back(parseThrowsCont());
                break;
            case TokenType::WHERE:
                result.push_back(parseWhereCont());
                break;
            default:
                goto exit_loop;
        }
    }

    exit_loop:
    return result;
}

RequiresCont* Parser::parseRequiresCont() {
    TextPosition startPosition = _lexer.peekStartPosition();

    if (!_lexer.consumeType(TokenType::REQUIRES)) {
        printError("expected `requires`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    Expr* condition = parseExpr();

    return new RequiresCont(condition, startPosition, condition->endPosition());
}

EnsuresCont* Parser::parseEnsuresCont() {
    TextPosition startPosition = _lexer.peekStartPosition();

    if (!_lexer.consumeType(TokenType::ENSURES)) {
        printError("expected `ensures`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    Expr* condition = parseExpr();

    return new EnsuresCont(condition, startPosition, condition->endPosition());
}

ThrowsCont* Parser::parseThrowsCont() {
    TextPosition startPosition = _lexer.peekStartPosition();
    TextPosition endPosition = _lexer.peekEndPosition();

    if (!_lexer.consumeType(TokenType::THROWS)) {
        printError("expected `throws`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    if (_lexer.peekType() == TokenType::SYMBOL) {
        Identifier exceptionType = parseIdentifier();

        return new ThrowsCont(startPosition, endPosition, exceptionType);
    } else {
        return new ThrowsCont(startPosition, endPosition);
    }
}

WhereCont* Parser::parseWhereCont() {
    TextPosition startPosition = _lexer.peekStartPosition();

    if (!_lexer.consumeType(TokenType::WHERE)) {
        printError("expected `where`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    Expr* condition = parseExpr();

    return new WhereCont(condition, startPosition, condition->endPosition());
}

// Statements ---------------------------------------------------------------------------------------------------------
Stmt* Parser::parseStmt() {
    switch (_lexer.peekType()) {
        case TokenType::BREAK:
            return parseBreakStmt();
        case TokenType::CASE:
            return parseCaseStmt();
        case TokenType::LCURLY:
            return parseCompoundStmt();
        case TokenType::CONTINUE:
            return parseContinueStmt();
        case TokenType::DO:
            return parseDoStmt();
        case TokenType::FOR:
            return parseForStmt();
        case TokenType::GOTO:
            return parseGotoStmt();
        case TokenType::IF:
            return parseIfStmt();
        case TokenType::RETURN:
            return parseReturnStmt();
        case TokenType::SWITCH:
            return parseSwitchStmt();
        case TokenType::TRY:
            return parseTryStmt();
        case TokenType::WHILE:
            return parseWhileStmt();
        default: {
            Expr* resultExpr = parseVariableExpr();

            // If it is at the statement level and you have a single identifier with a `:` after it then this is a
            // labeled statement (used by `goto`, `break`, and `continue`)
            if (llvm::isa<IdentifierExpr>(resultExpr) && _lexer.consumeType(TokenType::COLON)) {
                auto identifierExpr = llvm::dyn_cast<IdentifierExpr>(resultExpr);
                Identifier identifier = identifierExpr->identifier();

                Stmt* labeledStmt = parseStmt();

                delete identifierExpr;

                return new LabeledStmt(identifier, labeledStmt);
            } else {
                if (!_lexer.consumeType(TokenType::SEMICOLON)) {
                    printError("expected `;` after expression, found `" + _lexer.peekCurrentSymbol() + "`!",
                               _lexer.peekStartPosition(), _lexer.peekEndPosition());
                }

                return resultExpr;
            }
        }
    }
}

BreakStmt* Parser::parseBreakStmt() {
    TextPosition startPosition = _lexer.peekStartPosition();
    TextPosition endPosition = _lexer.peekEndPosition();

    _lexer.consumeType(TokenType::BREAK);

    if (_lexer.peekType() == TokenType::SYMBOL) {
        Identifier breakLabel = parseIdentifier();

        if (!_lexer.consumeType(TokenType::SEMICOLON)) {
            printError("expected `;` after `break " + breakLabel.name() + "`, found `" + _lexer.peekCurrentSymbol() + "`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }

        return new BreakStmt(startPosition, endPosition, breakLabel);
    } else {
        if (!_lexer.consumeType(TokenType::SEMICOLON)) {
            printError("expected `;` after `break`, found `" + _lexer.peekCurrentSymbol() + "`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }

        return new BreakStmt(startPosition, endPosition);
    }
}

CaseStmt* Parser::parseCaseStmt() {
    TextPosition startPosition = _lexer.peekStartPosition();
    TextPosition endPosition = _lexer.peekEndPosition();

    _lexer.consumeType(TokenType::CASE);

    Expr* condition = parseExpr();

    if (!_lexer.consumeType(TokenType::COLON)) {
        printError("expected `:` after `case`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    Stmt* trueStmt = parseStmt();

    return new CaseStmt(startPosition, endPosition, false, condition, trueStmt);
}

CatchStmt* Parser::parseCatchStmt() {
    TextPosition startPosition = _lexer.peekStartPosition();
    TextPosition endPosition = _lexer.peekEndPosition();

    if (_lexer.peekType() == TokenType::SYMBOL) {
        // `catch e: exception`
        Identifier varName = parseIdentifier();

        if (!_lexer.consumeType(TokenType::COLON)) {
            printError("expected `:` after exception variable name, found `" + _lexer.peekCurrentSymbol() + "`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }

        Type* exceptionType = parseType();
        CompoundStmt* catchBody = parseCompoundStmt();

        return new CatchStmt(startPosition, endPosition, catchBody, exceptionType, varName);
    } else if (_lexer.peekType() == TokenType::COLON) {
        // `catch: exception`
        _lexer.consumeType(TokenType::COLON);

        Type* exceptionType = parseType();
        CompoundStmt* catchBody = parseCompoundStmt();

        return new CatchStmt(startPosition, endPosition, catchBody, exceptionType);
    } else {
        // `catch`
        CompoundStmt* catchBody = parseCompoundStmt();

        return new CatchStmt(startPosition, endPosition, catchBody);
    }
}

CompoundStmt* Parser::parseCompoundStmt() {
    std::vector<Stmt*> statements;

    TextPosition startPosition = _lexer.peekStartPosition();

    if (!_lexer.consumeType(TokenType::LCURLY)) {
        printError("expected `{`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    while (_lexer.peekType() != TokenType::RCURLY && _lexer.peekType() != TokenType::ENDOFFILE) {
        statements.push_back(parseStmt());
    }

    TextPosition endPosition = _lexer.peekEndPosition();

    if (!_lexer.consumeType(TokenType::RCURLY)) {
        printError("expected ending `}`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    return new CompoundStmt(statements, startPosition, endPosition);
}

ContinueStmt* Parser::parseContinueStmt() {
    TextPosition startPosition = _lexer.peekStartPosition();
    TextPosition endPosition = _lexer.peekEndPosition();

    _lexer.consumeType(TokenType::CONTINUE);

    if (_lexer.peekType() == TokenType::SYMBOL) {
        Identifier continueLabel = parseIdentifier();

        if (!_lexer.consumeType(TokenType::SEMICOLON)) {
            printError("expected `;` after `continue " + continueLabel.name() + "`, found `" + _lexer.peekCurrentSymbol() + "`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }

        return new ContinueStmt(startPosition, endPosition, continueLabel);
    } else {
        if (!_lexer.consumeType(TokenType::SEMICOLON)) {
            printError("expected `;` after `continue`, found `" + _lexer.peekCurrentSymbol() + "`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }

        return new ContinueStmt(startPosition, endPosition);
    }
}

DoStmt* Parser::parseDoStmt() {
    TextPosition doStartPosition = _lexer.peekStartPosition();
    TextPosition doEndPosition = _lexer.peekEndPosition();

    _lexer.consumeType(TokenType::DO);

    CompoundStmt* loopStmt = parseCompoundStmt();

    TextPosition whileStartPosition = _lexer.peekStartPosition();
    TextPosition whileEndPosition = _lexer.peekEndPosition();

    if (!_lexer.consumeType(TokenType::WHILE)) {
        printError("expected `while` to end `do` loop, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    Expr* condition = parseExpr();

    if (!_lexer.consumeType(TokenType::SEMICOLON)) {
        printError("expected `;` after `do...while` condition, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    return new DoStmt(loopStmt, condition, doStartPosition, doEndPosition, whileStartPosition, whileEndPosition);
}

ForStmt* Parser::parseForStmt() {
    TextPosition startPosition = _lexer.peekStartPosition();
    TextPosition endPosition = _lexer.peekEndPosition();

    _lexer.consumeType(TokenType::FOR);

    Expr* init = nullptr;

    // All aspects of the loop are optional
    if (!_lexer.consumeType(TokenType::SEMICOLON)) {
        init = parseVariableExpr();

        if (!_lexer.consumeType(TokenType::SEMICOLON)) {
            printError("expected `;` after `for` loop's init expression!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }
    }

    Expr* condition = nullptr;

    if (!_lexer.consumeType(TokenType::SEMICOLON)) {
        condition = parseExpr();

        if (!_lexer.consumeType(TokenType::SEMICOLON)) {
            printError("expected `;` after `for` loop's condition!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }
    }

    Expr* iteration = nullptr;

    if (_lexer.peekType() != TokenType::LCURLY) {
        iteration = parseExpr();
    }

    CompoundStmt* loopBody = parseCompoundStmt();

    return new ForStmt(init, condition, iteration, loopBody, startPosition, endPosition);
}

GotoStmt* Parser::parseGotoStmt() {
    TextPosition startPosition = _lexer.peekStartPosition();
    TextPosition endPosition = _lexer.peekEndPosition();

    _lexer.consumeType(TokenType::GOTO);

    Identifier gotoLabel = parseIdentifier();

    if (!_lexer.consumeType(TokenType::SEMICOLON)) {
        printError("expected `;` after `goto " + gotoLabel.name() + "`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    return new GotoStmt(startPosition, endPosition, gotoLabel);
}

IfStmt* Parser::parseIfStmt() {
    TextPosition startPosition = _lexer.peekStartPosition();
    TextPosition endPosition = _lexer.peekEndPosition();

    _lexer.consumeType(TokenType::IF);

    Expr* condition = parseExpr();
    CompoundStmt* trueStmt = parseCompoundStmt();
    Stmt* falseStmt = nullptr;

    if (_lexer.consumeType(TokenType::ELSE)) {
        if (_lexer.peekType() == TokenType::IF) {
            falseStmt = parseIfStmt();
        } else if (_lexer.peekType() == TokenType::LCURLY) {
            falseStmt = parseCompoundStmt();
        } else {
            printError("expected `if` or `{` after `else`, found `" + _lexer.peekCurrentSymbol() + "`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }
    }

    return new IfStmt(startPosition, endPosition, condition, trueStmt, falseStmt);
}

ReturnStmt* Parser::parseReturnStmt() {
    TextPosition startPosition = _lexer.peekStartPosition();
    TextPosition endPosition = _lexer.peekEndPosition();

    _lexer.consumeType(TokenType::RETURN);

    if (!_lexer.consumeType(TokenType::SEMICOLON)) {
        Expr* returnValue = parseExpr();

        if (!_lexer.consumeType(TokenType::SEMICOLON)) {
            printError("expected `;` after `return ...`, found `" + _lexer.peekCurrentSymbol() + "`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }

        return new ReturnStmt(startPosition, endPosition, returnValue);
    } else {
        return new ReturnStmt(startPosition, endPosition);
    }
}

SwitchStmt* Parser::parseSwitchStmt() {
    TextPosition startPosition = _lexer.peekStartPosition();
    TextPosition endPosition = _lexer.peekEndPosition();

    _lexer.consumeType(TokenType::SWITCH);

    Expr* condition = parseExpr();

    if (!_lexer.consumeType(TokenType::LCURLY)) {
        printError("expected `{` after `switch` condition, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    std::vector<Stmt*> statements;

    while (_lexer.peekType() != TokenType::RCURLY && _lexer.peekType() != TokenType::ENDOFFILE) {
        statements.push_back(parseStmt());
    }

    if (!_lexer.consumeType(TokenType::RCURLY)) {
        printError("expected `}` to end `switch`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    return new SwitchStmt(startPosition, endPosition, condition, statements);
}

TryStmt* Parser::parseTryStmt() {
    TextPosition startPosition = _lexer.peekStartPosition();
    TextPosition endPosition = _lexer.peekEndPosition();

    _lexer.consumeType(TokenType::TRY);

    CompoundStmt* tryBody = parseCompoundStmt();

    std::vector<CatchStmt*> catchStatements;
    CompoundStmt* finallyStatement = nullptr;

    while (_lexer.peekType() == TokenType::CATCH || _lexer.peekType() == TokenType::FINALLY) {
        if (_lexer.peekType() == TokenType::CATCH) {
            catchStatements.push_back(parseCatchStmt());
        } else if (_lexer.peekType() == TokenType::FINALLY) {
            if (finallyStatement != nullptr) {
                printError("a `try` statement cannot have multiple `finally` statements!",
                           _lexer.peekStartPosition(), _lexer.peekEndPosition());
            }

            _lexer.consumeType(TokenType::FINALLY);

            finallyStatement = parseCompoundStmt();
        }
    }

    return new TryStmt(startPosition, endPosition, tryBody, catchStatements, finallyStatement);
}

WhileStmt* Parser::parseWhileStmt() {
    TextPosition startPosition = _lexer.peekStartPosition();
    TextPosition endPosition = _lexer.peekEndPosition();

    _lexer.consumeType(TokenType::WHILE);

    Expr* condition = parseExpr();
    CompoundStmt* loopBody = parseCompoundStmt();

    return new WhileStmt(condition, loopBody, startPosition, endPosition);
}

// Expressions --------------------------------------------------------------------------------------------------------
Expr* Parser::parseVariableExpr() {
    TextPosition startPosition = _lexer.peekStartPosition();

    if (_lexer.consumeType(TokenType::LET)) {
        bool isMutable = _lexer.consumeType(TokenType::MUT);

        if (_lexer.peekType() != TokenType::SYMBOL) {
            if (isMutable) {
                printError("expected variable named after `let mut`, found `" + _lexer.peekCurrentSymbol() + "`!",
                           _lexer.peekStartPosition(), _lexer.peekEndPosition());
            } else {
                printError("expected variable named after `let`, found `" + _lexer.peekCurrentSymbol() + "`!",
                           _lexer.peekStartPosition(), _lexer.peekEndPosition());
            }
        }

        Identifier identifier = parseIdentifier();
        TextPosition endPosition = identifier.endPosition();

        Type* type = nullptr;

        if (_lexer.consumeType(TokenType::COLON)) {
            type = parseType();

            endPosition = type->endPosition();
        }

        Expr* initialValue = nullptr;

        if (_lexer.consumeType(TokenType::EQUALS)) {
            initialValue = parseExpr();

            endPosition = initialValue->endPosition();
        }

        // TODO: Support multiple variables separated by comma
        return new VariableDeclExpr(identifier, type, initialValue, isMutable, startPosition, endPosition);
    } else if (_lexer.peekType() == TokenType::VAR) {
        printError("`var` cannot be used in this context, use `let mut` instead!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
        return nullptr;
    } else {
        return parseExpr();
    }
}

Expr* Parser::parseExpr() {
    return parseAssignment();
}

Expr* Parser::parseAssignment() {
    TextPosition startPosition = _lexer.peekStartPosition();
    Expr* result = parseTernary();

    switch (_lexer.peekType()) {
        case TokenType::EQUALS: {
            _lexer.consumeType(TokenType::EQUALS);
            Expr* rightValue = parseAssignment();
            TextPosition endPosition = rightValue->endPosition();

            return new AssignmentOperatorExpr(result, rightValue, startPosition, endPosition);
        }
        case TokenType::PLUSEQUALS: {
            _lexer.consumeType(TokenType::PLUSEQUALS);
            Expr* rightValue = parseAssignment();
            TextPosition endPosition = rightValue->endPosition();

            return new AssignmentOperatorExpr(result, rightValue, InfixOperators::Add,
                                              startPosition, endPosition);
        }
        case TokenType::MINUSEQUALS: {
            _lexer.consumeType(TokenType::MINUSEQUALS);
            Expr* rightValue = parseAssignment();
            TextPosition endPosition = rightValue->endPosition();

            return new AssignmentOperatorExpr(result, rightValue, InfixOperators::Subtract,
                                              startPosition, endPosition);
        }
        case TokenType::STAREQUALS: {
            _lexer.consumeType(TokenType::STAREQUALS);
            Expr* rightValue = parseAssignment();
            TextPosition endPosition = rightValue->endPosition();

            return new AssignmentOperatorExpr(result, rightValue, InfixOperators::Multiply,
                                              startPosition, endPosition);
        }
        case TokenType::SLASHEQUALS: {
            _lexer.consumeType(TokenType::SLASHEQUALS);
            Expr* rightValue = parseAssignment();
            TextPosition endPosition = rightValue->endPosition();

            return new AssignmentOperatorExpr(result, rightValue, InfixOperators::Divide,
                                              startPosition, endPosition);
        }
        case TokenType::PERCENTEQUALS: {
            _lexer.consumeType(TokenType::PERCENTEQUALS);
            Expr* rightValue = parseAssignment();
            TextPosition endPosition = rightValue->endPosition();

            return new AssignmentOperatorExpr(result, rightValue, InfixOperators::Remainder,
                                              startPosition, endPosition);
        }
        case TokenType::LEFTEQUALS: {
            _lexer.consumeType(TokenType::LEFTEQUALS);
            Expr* rightValue = parseAssignment();
            TextPosition endPosition = rightValue->endPosition();

            return new AssignmentOperatorExpr(result, rightValue, InfixOperators::BitshiftLeft,
                                              startPosition, endPosition);
        }
        case TokenType::RIGHTEQUALS: {
            _lexer.consumeType(TokenType::RIGHTEQUALS);
            Expr* rightValue = parseAssignment();
            TextPosition endPosition = rightValue->endPosition();

            return new AssignmentOperatorExpr(result, rightValue, InfixOperators::BitshiftRight,
                                              startPosition, endPosition);
        }
        case TokenType::AMPERSANDEQUALS: {
            _lexer.consumeType(TokenType::AMPERSANDEQUALS);
            Expr* rightValue = parseAssignment();
            TextPosition endPosition = rightValue->endPosition();

            return new AssignmentOperatorExpr(result, rightValue, InfixOperators::BitwiseAnd,
                                              startPosition, endPosition);
        }
        case TokenType::CARETEQUALS: {
            _lexer.consumeType(TokenType::CARETEQUALS);
            Expr* rightValue = parseAssignment();
            TextPosition endPosition = rightValue->endPosition();

            return new AssignmentOperatorExpr(result, rightValue, InfixOperators::BitwiseXor,
                                              startPosition, endPosition);
        }
        case TokenType::PIPEEQUALS: {
            _lexer.consumeType(TokenType::PIPEEQUALS);
            Expr* rightValue = parseAssignment();
            TextPosition endPosition = rightValue->endPosition();

            return new AssignmentOperatorExpr(result, rightValue, InfixOperators::BitwiseOr,
                                              startPosition, endPosition);
        }
        case TokenType::CARETCARETEQUALS: {
            _lexer.consumeType(TokenType::CARETCARETEQUALS);
            Expr* rightValue = parseAssignment();
            TextPosition endPosition = rightValue->endPosition();

            return new AssignmentOperatorExpr(result, rightValue, InfixOperators::Power,
                                              startPosition, endPosition);
        }
        default:
            return result;
    }
}

Expr* Parser::parseTernary() {
    Expr* result = parseLogicalOr();

    if (_lexer.consumeType(TokenType::QUESTION)) {
        Expr* trueExpr = parseAssignment();

        if (!_lexer.consumeType(TokenType::COLON)) {
            printError("expected ':' in ternary statement! (found '" + _lexer.peekToken().currentSymbol + "')",
                       _lexer.peekToken().startPosition, _lexer.peekToken().endPosition);
        }

        Expr* falseExpr = parseAssignment();

        result = new TernaryExpr(result, trueExpr, falseExpr);
    }

    return result;
}

Expr* Parser::parseLogicalOr() {
    Expr* result = parseLogicalAnd();

    while (_lexer.peekType() != TokenType::ENDOFFILE) {
        switch (_lexer.peekType()) {
            case TokenType::PIPEPIPE: {
                _lexer.consumeType(TokenType::PIPEPIPE);
                Expr* rightValue = parseLogicalAnd();

                result = new InfixOperatorExpr(InfixOperators::LogicalOr, result, rightValue);
                continue;
            }
            default:
                return result;
        }
    }

    printError("reach end of file unexpectedly!",
               _lexer.peekToken().startPosition, _lexer.peekToken().endPosition);
    return nullptr;
}

Expr* Parser::parseLogicalAnd() {
    Expr* result = parseBitwiseOr();

    while (_lexer.peekType() != TokenType::ENDOFFILE) {
        switch (_lexer.peekType()) {
            case TokenType::AMPERSANDAMPERSAND: {
                _lexer.consumeType(TokenType::AMPERSANDAMPERSAND);
                Expr* rightValue = parseBitwiseOr();

                result = new InfixOperatorExpr(InfixOperators::LogicalAnd, result, rightValue);
                continue;
            }
            default:
                return result;
        }
    }

    printError("reach end of file unexpectedly!",
               _lexer.peekToken().startPosition, _lexer.peekToken().endPosition);
    return nullptr;
}

Expr* Parser::parseBitwiseOr() {
    Expr* result = parseBitwiseXor();

    while (_lexer.peekType() != TokenType::ENDOFFILE) {
        switch (_lexer.peekType()) {
            case TokenType::PIPE: {
                _lexer.consumeType(TokenType::PIPE);
                Expr* rightValue = parseBitwiseXor();

                result = new InfixOperatorExpr(InfixOperators::BitwiseOr, result, rightValue);
                continue;
            }
            default:
                return result;
        }
    }

    printError("reach end of file unexpectedly!",
               _lexer.peekToken().startPosition, _lexer.peekToken().endPosition);
    return nullptr;
}

Expr* Parser::parseBitwiseXor() {
    Expr* result = parseBitwiseAnd();

    while (_lexer.peekType() != TokenType::ENDOFFILE) {
        switch (_lexer.peekType()) {
            case TokenType::CARET: {
                _lexer.consumeType(TokenType::CARET);
                Expr* rightValue = parseBitwiseAnd();

                result = new InfixOperatorExpr(InfixOperators::BitwiseXor, result, rightValue);
                continue;
            }
            default:
                return result;
        }
    }

    printError("reach end of file unexpectedly!",
               _lexer.peekToken().startPosition, _lexer.peekToken().endPosition);
    return nullptr;
}

Expr* Parser::parseBitwiseAnd() {
    Expr* result = parseEqualToNotEqualTo();

    while (_lexer.peekType() != TokenType::ENDOFFILE) {
        switch (_lexer.peekType()) {
            case TokenType::AMPERSAND: {
                _lexer.consumeType(TokenType::AMPERSAND);
                Expr* rightValue = parseEqualToNotEqualTo();

                result = new InfixOperatorExpr(InfixOperators::BitwiseAnd, result, rightValue);
                continue;
            }
            default:
                return result;
        }
    }

    printError("reach end of file unexpectedly!",
               _lexer.peekToken().startPosition, _lexer.peekToken().endPosition);
    return nullptr;
}

Expr* Parser::parseEqualToNotEqualTo() {
    Expr* result = parseGreaterThanLessThan();

    while (_lexer.peekType() != TokenType::ENDOFFILE) {
        switch (_lexer.peekType()) {
            case TokenType::EQUALEQUALS: {
                _lexer.consumeType(TokenType::EQUALEQUALS);
                Expr* rightValue = parseGreaterThanLessThan();

                result = new InfixOperatorExpr(InfixOperators::EqualTo, result, rightValue);
                continue;
            }
            case TokenType::NOTEQUALS: {
                _lexer.consumeType(TokenType::NOTEQUALS);
                Expr* rightValue = parseGreaterThanLessThan();

                result = new InfixOperatorExpr(InfixOperators::NotEqualTo, result, rightValue);
                continue;
            }
            default:
                return result;
        }
    }

    printError("reach end of file unexpectedly!",
               _lexer.peekToken().startPosition, _lexer.peekToken().endPosition);
    return nullptr;
}

Expr* Parser::parseGreaterThanLessThan() {
    Expr* result = parseBitwiseShifts();

    while (_lexer.peekType() != TokenType::ENDOFFILE) {
        switch (_lexer.peekType()) {
            case TokenType::GREATER: {
                _lexer.consumeType(TokenType::GREATER);
                Expr* rightValue = parseBitwiseShifts();

                result = new InfixOperatorExpr(InfixOperators::GreaterThan, result, rightValue);
                continue;
            }
            case TokenType::GREATEREQUALS: {
                _lexer.consumeType(TokenType::GREATEREQUALS);
                Expr* rightValue = parseBitwiseShifts();

                result = new InfixOperatorExpr(InfixOperators::GreaterThanEqualTo, result, rightValue);
                continue;
            }
            case TokenType::LESS: {
                _lexer.consumeType(TokenType::LESS);
                Expr* rightValue = parseBitwiseShifts();

                result = new InfixOperatorExpr(InfixOperators::LessThan, result, rightValue);
                continue;
            }
            case TokenType::LESSEQUALS: {
                _lexer.consumeType(TokenType::LESSEQUALS);
                Expr* rightValue = parseBitwiseShifts();

                result = new InfixOperatorExpr(InfixOperators::LessThanEqualTo, result, rightValue);
                continue;
            }
            default:
                return result;
        }
    }

    printError("reach end of file unexpectedly!",
               _lexer.peekToken().startPosition, _lexer.peekToken().endPosition);
    return nullptr;
}

Expr* Parser::parseBitwiseShifts() {
    Expr* result = parseAdditionSubtraction();

    while (_lexer.peekType() != TokenType::ENDOFFILE) {
        switch (_lexer.peekType()) {
            case TokenType::LEFT: {
                _lexer.consumeType(TokenType::LEFT);
                Expr* rightValue = parseAdditionSubtraction();

                result = new InfixOperatorExpr(InfixOperators::BitshiftLeft, result, rightValue);
                continue;
            }
            case TokenType::RIGHT: {
                _lexer.consumeType(TokenType::RIGHT);
                Expr* rightValue = parseAdditionSubtraction();

                result = new InfixOperatorExpr(InfixOperators::BitshiftRight, result, rightValue);
                continue;
            }
            default:
                return result;
        }
    }

    printError("reach end of file unexpectedly!",
               _lexer.peekToken().startPosition, _lexer.peekToken().endPosition);
    return nullptr;
}

Expr* Parser::parseAdditionSubtraction() {
    Expr* result = parseMultiplicationDivisionOrRemainder();

    while (_lexer.peekType() != TokenType::ENDOFFILE) {
        switch (_lexer.peekType()) {
            case TokenType::PLUS: {
                _lexer.consumeType(TokenType::PLUS);
                Expr* rightValue = parseMultiplicationDivisionOrRemainder();

                result = new InfixOperatorExpr(InfixOperators::Add, result, rightValue);
                continue;
            }
            case TokenType::MINUS: {
                _lexer.consumeType(TokenType::MINUS);
                Expr* rightValue = parseMultiplicationDivisionOrRemainder();

                result = new InfixOperatorExpr(InfixOperators::Subtract, result, rightValue);
                continue;
            }
            default:
                return result;
        }
    }

    printError("reach end of file unexpectedly!",
               _lexer.peekToken().startPosition, _lexer.peekToken().endPosition);
    return nullptr;
}

Expr* Parser::parseMultiplicationDivisionOrRemainder() {
    Expr* result = parseIsAsHas();

    while (_lexer.peekType() != TokenType::ENDOFFILE) {
        switch (_lexer.peekType()) {
            case TokenType::STAR: {
                _lexer.consumeType(TokenType::STAR);
                Expr* rightValue = parseIsAsHas();

                result = new InfixOperatorExpr(InfixOperators::Multiply, result, rightValue);
                continue;
            }
            case TokenType::SLASH: {
                _lexer.consumeType(TokenType::SLASH);
                Expr* rightValue = parseIsAsHas();

                result = new InfixOperatorExpr(InfixOperators::Divide, result, rightValue);
                continue;
            }
            case TokenType::PERCENT: {
                _lexer.consumeType(TokenType::PERCENT);
                Expr* rightValue = parseIsAsHas();

                result = new InfixOperatorExpr(InfixOperators::Remainder, result, rightValue);
                continue;
            }
            default:
                return result;
        }
    }

    printError("reach end of file unexpectedly!",
               _lexer.peekToken().startPosition, _lexer.peekToken().endPosition);
    return nullptr;
}

Expr* Parser::parseIsAsHas() {
    Expr* result = parsePrefixes();

    while (_lexer.peekType() != TokenType::ENDOFFILE) {
        switch (_lexer.peekType()) {
            case TokenType::AS: {
                TextPosition asStartPosition = _lexer.peekStartPosition();
                TextPosition asEndPosition = _lexer.peekEndPosition();
                _lexer.consumeType(TokenType::AS);

                Type* asType = parseType();

                result = new AsExpr(result, asType, asStartPosition, asEndPosition);
                continue;
            }
            case TokenType::IS: {
                TextPosition isStartPosition = _lexer.peekStartPosition();
                TextPosition isEndPosition = _lexer.peekEndPosition();
                _lexer.consumeType(TokenType::IS);

                Type* isType = parseType();

                result = new IsExpr(result, isType, isStartPosition, isEndPosition);
                continue;
            }
            case TokenType::HAS: {
                TextPosition hasStartPosition = _lexer.peekStartPosition();
                TextPosition hasEndPosition = _lexer.peekEndPosition();
                _lexer.consumeType(TokenType::HAS);

                Type* traitType = parseType();

                result = new HasExpr(result, traitType, hasStartPosition, hasEndPosition);
                continue;
            }
            default:
                return result;
        }
    }
    return nullptr;
}

Expr* Parser::parsePrefixes() {
    TextPosition startPosition = _lexer.peekStartPosition();
    TextPosition endPosition = _lexer.peekEndPosition();

    switch (_lexer.peekType()) {
        case TokenType::PLUSPLUS: {
            _lexer.consumeType(TokenType::PLUSPLUS);
            Expr* expr = parsePrefixes();

            return new PrefixOperatorExpr(PrefixOperators::Increment, expr, startPosition, endPosition);
        }
        case TokenType::MINUSMINUS: {
            _lexer.consumeType(TokenType::MINUSMINUS);
            Expr* expr = parsePrefixes();

            return new PrefixOperatorExpr(PrefixOperators::Decrement, expr, startPosition, endPosition);
        }
        case TokenType::PLUS: {
            _lexer.consumeType(TokenType::PLUS);
            Expr* expr = parsePrefixes();

            return new PrefixOperatorExpr(PrefixOperators::Positive, expr, startPosition, endPosition);
        }
        case TokenType::MINUS: {
            _lexer.consumeType(TokenType::MINUS);
            Expr* expr = parsePrefixes();

            return new PrefixOperatorExpr(PrefixOperators::Negative, expr, startPosition, endPosition);
        }
        case TokenType::NOT: {
            _lexer.consumeType(TokenType::NOT);
            Expr* expr = parsePrefixes();

            return new PrefixOperatorExpr(PrefixOperators::LogicalNot, expr, startPosition, endPosition);
        }
        case TokenType::TILDE: {
            _lexer.consumeType(TokenType::TILDE);
            Expr* expr = parsePrefixes();

            return new PrefixOperatorExpr(PrefixOperators::BitwiseNot, expr, startPosition, endPosition);
        }
        case TokenType::STAR: {
            _lexer.consumeType(TokenType::STAR);
            Expr* expr = parsePrefixes();

            return new PrefixOperatorExpr(PrefixOperators::Dereference, expr, startPosition, endPosition);
        }
        case TokenType::AMPERSAND: {
            _lexer.consumeType(TokenType::AMPERSAND);
            Expr* expr = parsePrefixes();

            return new PrefixOperatorExpr(PrefixOperators::Reference, expr, startPosition, endPosition);
        }
        case TokenType::SIZEOF: {
            _lexer.consumeType(TokenType::SIZEOF);
            Expr* expr = parsePrefixes();

            return new PrefixOperatorExpr(PrefixOperators::SizeOf, expr, startPosition, endPosition);
        }
        case TokenType::ALIGNOF: {
            _lexer.consumeType(TokenType::ALIGNOF);
            Expr* expr = parsePrefixes();

            return new PrefixOperatorExpr(PrefixOperators::AlignOf, expr, startPosition, endPosition);
        }
        case TokenType::OFFSETOF: {
            _lexer.consumeType(TokenType::OFFSETOF);
            Expr* expr = parsePrefixes();

            return new PrefixOperatorExpr(PrefixOperators::OffsetOf, expr, startPosition, endPosition);
        }
        case TokenType::NAMEOF: {
            _lexer.consumeType(TokenType::NAMEOF);
            Expr* expr = parsePrefixes();

            return new PrefixOperatorExpr(PrefixOperators::NameOf, expr, startPosition, endPosition);
        }
        case TokenType::TRAITSOF: {
            _lexer.consumeType(TokenType::TRAITSOF);
            Expr* expr = parsePrefixes();

            return new PrefixOperatorExpr(PrefixOperators::TraitsOf, expr, startPosition, endPosition);
        }
        default:
            return parseCallPostfixOrMemberAccess();
    }
}

Expr* Parser::parseCallPostfixOrMemberAccess() {
    Expr* result = parseIdentifierOrLiteralExpr();

    while (_lexer.peekType() != TokenType::ENDOFFILE) {
        switch (_lexer.peekType()) {
            case TokenType::PLUSPLUS: {
                TextPosition startPosition = _lexer.peekToken().startPosition;
                TextPosition endPosition = _lexer.peekToken().endPosition;
                _lexer.consumeType(TokenType::PLUSPLUS);

                result = new PostfixOperatorExpr(PostfixOperators::Increment, result, startPosition, endPosition);
                continue;
            }
            case TokenType::MINUSMINUS: {
                TextPosition startPosition = _lexer.peekToken().startPosition;
                TextPosition endPosition = _lexer.peekToken().endPosition;
                _lexer.consumeType(TokenType::MINUSMINUS);

                result = new PostfixOperatorExpr(PostfixOperators::Decrement, result, startPosition, endPosition);
                continue;
            }
            case TokenType::LPAREN: {
                _lexer.consumeType(TokenType::LPAREN);

                std::vector<Expr*> arguments{};

                while (_lexer.peekType() != TokenType::RPAREN && _lexer.peekType() != TokenType::ENDOFFILE) {
                    // When parsing arguments we usually require argument labels (like Swift)
                    // Examples:
                    //     example(left: 2, right: new! Window());
                    //     attempt(try: function(), onFail: failure());
                    // We also allow keywords as labels (as you can see with the `try: ...`)
                    // In this situation you don't have to prefix with `@`
                    if (_lexer.peekMeta() == TokenMetaType::KEYWORD || _lexer.peekMeta() == TokenMetaType::MODIFIER) {
                        Identifier argumentLabel(_lexer.peekStartPosition(),
                                                 _lexer.peekEndPosition(),
                                                 _lexer.peekCurrentSymbol());

                        _lexer.consumeType(_lexer.peekType());

                        if (!_lexer.consumeType(TokenType::COLON)) {
                            printError("expected `:` after argument label `" + argumentLabel.name() + "`, "
                                       "found `" + _lexer.peekCurrentSymbol() + "`!",
                                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
                        }

                        arguments.push_back(new LabeledArgumentExpr(argumentLabel, parseExpr()));
                    } else {
                        Expr* parsedExpr = parseExpr();

                        if (llvm::isa<IdentifierExpr>(parsedExpr)) {
                            if (_lexer.consumeType(TokenType::COLON)) {
                                auto identifierExpr = llvm::dyn_cast<IdentifierExpr>(parsedExpr);
                                Identifier argumentLabel = identifierExpr->identifier();

                                // TODO: Make sure `identifierExpr` doesn't have template arguments
                                delete identifierExpr;

                                arguments.push_back(new LabeledArgumentExpr(argumentLabel, parseExpr()));
                            } else {
                                arguments.push_back(parsedExpr);
                            }
                        } else {
                            arguments.push_back(parsedExpr);
                        }
                    }

                    if (!_lexer.consumeType(TokenType::COMMA)) break;
                }

                TextPosition endPosition = _lexer.peekToken().endPosition;

                if (!_lexer.consumeType(TokenType::RPAREN)) {
                    printError("expected ending ')' for function call! (found '" + _lexer.peekToken().currentSymbol + "')",
                               _lexer.peekToken().startPosition, _lexer.peekToken().endPosition);
                    return nullptr;
                }

                result = new FunctionCallExpr(result, arguments, result->startPosition(), endPosition);
                continue;
            }
            case TokenType::LSQUARE: {
                _lexer.consumeType(TokenType::LSQUARE);


                std::vector<Expr*> arguments{};

                while (_lexer.peekType() != TokenType::RSQUARE && _lexer.peekType() != TokenType::ENDOFFILE) {
                    arguments.push_back(parseExpr());

                    if (!_lexer.consumeType(TokenType::COMMA)) break;
                }

                TextPosition endPosition = _lexer.peekToken().endPosition;

                if (!_lexer.consumeType(TokenType::RSQUARE)) {
                    printError("expected ending ']' for indexer call! (found '" + _lexer.peekToken().currentSymbol + "')",
                               _lexer.peekToken().startPosition, _lexer.peekToken().endPosition);
                    return nullptr;
                }

                result = new IndexerCallExpr(result, arguments, result->startPosition(), endPosition);
                continue;
            }
            case TokenType::PERIOD: {
                _lexer.consumeType(TokenType::PERIOD);
                IdentifierExpr* member = parseIdentifierExpr();

                result = new MemberAccessCallExpr(false, result, member);
                continue;
            }
            case TokenType::ARROW: {
                _lexer.consumeType(TokenType::ARROW);
                IdentifierExpr* member = parseIdentifierExpr();

                result = new MemberAccessCallExpr(true, result, member);
                continue;
            }
            default:
                return result;
        }
    }

    printError("reach end of file unexpectedly!", _lexer.peekToken().startPosition, _lexer.peekToken().endPosition);
    return nullptr;
}

Expr* Parser::parseIdentifierOrLiteralExpr() {
    Token const& peekedToken = _lexer.peekToken();

    switch (peekedToken.tokenType) {
        // `@` is used to allow keywords as names
        case TokenType::ATSYMBOL:
        case TokenType::SYMBOL:
            return parseIdentifierExpr();
        case TokenType::NUMBER:
            return parseNumberLiteralExpr();
        case TokenType::STRING:
            return parseStringLiteralExpr();
        case TokenType::CHARACTER:
            return parseCharacterLiteralExpr();
        case TokenType::LPAREN: {
            // TODO: Support tuples by checking for a comma after.
            TextPosition startPosition = _lexer.peekStartPosition();

            _lexer.consumeType(TokenType::LPAREN);

            Expr* nestedExpr = parseExpr();

            TextPosition endPosition = _lexer.peekEndPosition();

            if (!_lexer.consumeType(TokenType::RPAREN)) {
                printError("expected ending `)`, found `" + _lexer.peekCurrentSymbol() + "`!",
                           _lexer.peekStartPosition(), _lexer.peekEndPosition());
            }

            return new ParenExpr(nestedExpr, startPosition, endPosition);
        }
        case TokenType::LCURLY: {
            return parseArrayLiteralOrDimensionType();
        }
        default:
            printError("expected constant literal or identifier! (found `" + _lexer.peekToken().currentSymbol + "`)",
                       peekedToken.startPosition, peekedToken.endPosition);
            return nullptr;
    }
}

IdentifierExpr* Parser::parseIdentifierExpr() {
    Identifier identifier(parseIdentifier());

    std::vector<Expr*> templateArguments;

    if (_lexer.peekType() == TokenType::LESS) {
        bool oldRightShiftEnabledValue = _lexer.getRightShiftState();
        _lexer.setRightShiftState(false);
        LexerCheckpoint lexerCheckpoint(_lexer.createCheckpoint());

        _lexer.consumeType(TokenType::LESS);

        while (_lexer.peekType() != TokenType::TEMPLATEEND) {
            templateArguments.push_back(parsePrefixes());

            if (!_lexer.consumeType(TokenType::COMMA)) break;
        }

        bool canceled = false;

        if (_lexer.consumeType(TokenType::TEMPLATEEND)) {
            switch (_lexer.peekType()) {
                case TokenType::SEMICOLON:
                case TokenType::RPAREN:
                case TokenType::PERIOD:
                case TokenType::COLONCOLON:
                case TokenType::COMMA:
                // `(` is the only iffy one. E.g. `A<B>(12)` could be a function call OR  `(A < B) > (12)`.
                // For now we favor template usages in that scenario and if you want that to be the greater than less
                // than then use the parenthesis.
                case TokenType::LPAREN:
                    break;
                default:
                    // If the token is anything but the ones above then this wasn't a template usage.
                    _lexer.returnToCheckpoint(lexerCheckpoint);
                    canceled = true;
                    break;
            }
        } else {
            // If we didn't find the `>` then this wasn't a template usage.
            _lexer.returnToCheckpoint(lexerCheckpoint);
            canceled = true;
        }

        _lexer.setRightShiftState(oldRightShiftEnabledValue);

        // If there there wasn't a valid template type syntax then we delete the arguments.
        if (canceled && !templateArguments.empty()) {
            for (Expr* templateArgument : templateArguments) {
                delete templateArgument;
            }

            templateArguments.clear();
        }
    }

    return new IdentifierExpr(identifier, templateArguments);
}

ValueLiteralExpr* Parser::parseNumberLiteralExpr() {
    // TODO: We need to support numerical bases.
    //       0b - binary
    //       0x - hexadecimal
    //       0o - octal (NOT just leading zero, this can lead to confusion for novice programmers)
    ValueLiteralExpr::LiteralType literalType = ValueLiteralExpr::LiteralType::Integer;
    std::string numberValue = _lexer.peekCurrentSymbol();
    TextPosition startPosition = _lexer.peekStartPosition();
    TextPosition endPosition = _lexer.peekEndPosition();

    if (!_lexer.consumeType(TokenType::NUMBER)) {
        printError("expected number literal, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    // NOTE: We DO NOT allow `1 .0f`
    if (!_lexer.peekHasLeadingWhitespace()) {
        // We create a checkpoint and then try to parse floating point periods.
        // We have to create a checkpoint because we allow code like `123.toString()`
        LexerCheckpoint lexerCheckpoint(_lexer.createCheckpoint());

        if (_lexer.consumeType(TokenType::PERIOD)) {
            // NOTE: We DO NOT allow `1. 0f`
            if (_lexer.peekHasLeadingWhitespace() || _lexer.peekType() != TokenType::NUMBER) {
                _lexer.returnToCheckpoint(lexerCheckpoint);
            } else {
                literalType = ValueLiteralExpr::LiteralType::Float;

                numberValue += "." + _lexer.peekCurrentSymbol();

                endPosition = _lexer.peekEndPosition();

                _lexer.consumeType(TokenType::NUMBER);
            }
        }
    }

    std::string resultNumber;
    std::string resultSuffix;
    bool fillSuffix = false;
    int base = 10;

    for (std::size_t i = 0; i < numberValue.size(); ++i) {
        char c = numberValue[i];

        if (fillSuffix) {
            resultSuffix += c;
        } else {
            if (base == 10 && std::isdigit(c)) {
                resultNumber += c;
            } else if (base == 16 && std::isdigit(c) ||
                       (c == 'a' || c == 'A' || c == 'b' || c == 'B' || c == 'c' || c == 'C' || c == 'd' || c == 'D' ||
                        c == 'e' || c == 'E' || c == 'f' || c == 'F')) {
                resultNumber += c;
            } else if (base == 2 && (c == '0' || c == '1')) {
                resultNumber += c;
            } else if (base == 8 &&
                       (c == '0' || c == '1' || c == '2' || c == '3' || c == '4' || c == '5' || c == '6' || c == '7')) {
                resultNumber += c;
            } else if (i == 1 && numberValue[0] == '0') {
                // Handle `0x...`, `0b...`, `0o...`
                switch (c) {
                    case 'x':
                        // Hex
                        base = 16;
                        resultNumber += c;
                        break;
                    case 'b':
                        // Binary
                        base = 2;
                        resultNumber += c;
                        break;
                    case 'o':
                        // Octal
                        base = 8;
                        resultNumber += c;
                        break;
                    default:
                        // Suffix
                        fillSuffix = true;
                        resultSuffix += c;
                        break;
                }
            } else {
                fillSuffix = true;
                resultSuffix += c;
            }
        }
    }

    return new ValueLiteralExpr(literalType, resultNumber, resultSuffix, startPosition, endPosition);
}

ValueLiteralExpr* Parser::parseStringLiteralExpr() {
    std::string stringValue = _lexer.peekCurrentSymbol();
    TextPosition startPosition = _lexer.peekStartPosition();
    TextPosition endPosition = _lexer.peekEndPosition();

    if (!_lexer.consumeType(TokenType::STRING)) {
        printError("expected string literal, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    std::string typeSuffix;

    // NOTE: We DO NOT allow `"example string" str`, this is invalid syntax
    if (!_lexer.peekHasLeadingWhitespace() && _lexer.peekType() == TokenType::SYMBOL) {
        typeSuffix = _lexer.peekCurrentSymbol();
        endPosition = _lexer.peekEndPosition();

        _lexer.consumeType(TokenType::SYMBOL);
    }

    return new ValueLiteralExpr(ValueLiteralExpr::LiteralType::String, stringValue, typeSuffix,
                                startPosition, endPosition);
}

ValueLiteralExpr* Parser::parseCharacterLiteralExpr() {
    // TODO: We need to parse character literals the same as strings where 'multiple characters' are allowed in a char
    //       and we validate outside of the parser.
    printError("character literals are not yet supported!",
               _lexer.peekStartPosition(), _lexer.peekEndPosition());
    return nullptr;
}

Expr* Parser::parseArrayLiteralOrDimensionType() {
    TextPosition startPosition = _lexer.peekStartPosition();

    if (!_lexer.consumeType(TokenType::LSQUARE)) {
        printError("expected `[`, found `" + _lexer.peekCurrentSymbol() + "`!",
                   _lexer.peekStartPosition(), _lexer.peekEndPosition());
    }

    if (_lexer.peekType() == TokenType::COMMA) {
        // If the syntax is `[,` with no data for the first column then it is assumed that we are parsing a dimension
        // type
        std::size_t dimensions = 1;

        while (_lexer.consumeType(TokenType::COMMA)) {
            dimensions += 1;
        }

        if (!_lexer.consumeType(TokenType::RSQUARE)) {
            printError("expected ending `]` for dimension type, found `" + _lexer.peekCurrentSymbol() + "`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }

        Type* nestedType = parseType();
        Type* resultType = new DimensionType(Type::Qualifier::Unassigned, nestedType, 1);

        return new TypeExpr(resultType);
    } else if (_lexer.peekType() == TokenType::RSQUARE) {
        TextPosition endPosition = _lexer.peekEndPosition();

        _lexer.consumeType(TokenType::RSQUARE);

        // If the syntax is `[]` then we have to check what the next token after the `]` is.
        if (_lexer.peekType() == TokenType::SYMBOL) {
            // If the token after `]` is `SYMBOL` then we parse the type
            Type* nestedType = parseType();
            Type* resultType = new DimensionType(Type::Qualifier::Unassigned, nestedType, 1);

            return new TypeExpr(resultType);
        } else {
            // If the token isn't `SYMBOL` then we will sadly have to figure out if it is a type in a compiler pass
            // since `[] * variable` is allowed, we will have to see if `variable` is a type
            return new ArrayLiteralExpr({}, startPosition, endPosition);
        }
    } else {
        std::vector<Expr*> indexes;

        while (_lexer.peekType() != TokenType::RSQUARE && _lexer.peekType() != TokenType::ENDOFFILE) {
            indexes.push_back(parseExpr());

            if (!_lexer.consumeType(TokenType::COMMA)) {
                break;
            }
        }

        TextPosition endPosition = _lexer.peekEndPosition();

        if (!_lexer.consumeType(TokenType::RSQUARE)) {
            printError("expected ending `]` for array literal, found `" + _lexer.peekCurrentSymbol() + "`!",
                       _lexer.peekStartPosition(), _lexer.peekEndPosition());
        }

        return new ArrayLiteralExpr(indexes, startPosition, endPosition);
    }
}
