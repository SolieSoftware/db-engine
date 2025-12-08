#pragma once
#include <string>

namespace dbengine {
    enum class TokenType {
        // Keywords
        SELECT, FROM , WHERE, INSERT, INTO, VALUES,

        // Identifiers and Literals
        IDENTIFIER, NUMBERS, STRING, 

        // Operators
        EQUALS, NOT_EQUALS, LESS_THAN, GREATER_THAN, LESS_EQUAL, GREATER_EQUAL,

        // Punctuation
        COMMA, LPAREN, RPAREN, SEMICOLON, STAR,

        // Special
        END_OF_FILE, INVALID
    };

    class Token {
        public:
        Token(TokenType token_type, char[] token_content) : token_(token_type), token_content_(token_content) {};

        TokenType GetTokenType() {return token_type_; }

        char[] GetTokenContent() {return token_content_; }

        private:
        TokenType token_type_;
        char[] token_content_;

    };
}