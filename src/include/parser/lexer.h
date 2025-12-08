#pragma once
#include "parser/token.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace dbengine {
    class Lexer {
        public: 
        Lexer(const std::string &input);

        std::vector<Token> Tokenize();

        private:
            void SkipWhiteSpace();
            Token ScanIdentifierOrKeyWord();
            Token ScanNumber();
            Token ScanOperator();

            char Peek() const;
            char PeekNext() const;
            char Advance();
            bool IsAtEnd() const;

            bool IsDigit(char c) const;
            bool IsAlpha(char c) const;
            bool IsAlphaNumeric(char c) const;


            std::string input_;
            size_t position_;

            static const std::unordered_map<std::string, TokenType> keywords_;
    };
}