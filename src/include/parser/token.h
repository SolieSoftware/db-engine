#pragma once
#include <string>

namespace dbengine {
    enum class TokenType {
        // Keywords
        SELECT, FROM , WHERE, INSERT, INTO, VALUES,

        // Identifiers and Literals
        IDENTIFIER, NUMBER, STRING, 

        // Operators
        EQUALS, NOT_EQUALS, LESS_THAN, GREATER_THAN, LESS_EQUAL, GREATER_EQUAL,

        // Punctuation
        COMMA, LPAREN, RPAREN, SEMICOLON, STAR,

        // Special
        END_OF_FILE, INVALID
    };

    class Token {
        public:
        Token(TokenType type, const std::string &lexeme) : type_(token_type), lexeme(token_content) {};

        TokenType GetTokenType() const {return type_; }
        std::string GetTokenContent() const {return lexeme_; }

        private:
        TokenType token_type_;
        std::string lexeme_;
    };
}