#include "parser/lexer.h"

#include <algorithm>
#include <cctype>

namespace dbengine {

    // Define outside constructor as variable is const static, defining this outside the constructor means it is initialised once when the program is compiled
    const std::unordered_map<std::string, TokenType> Lexer::keywords_ = {
      {"select", TokenType::SELECT},
      {"from", TokenType::FROM},
      {"where", TokenType::WHERE},
      {"insert", TokenType::INSERT},
      {"into", TokenType::INTO},
      {"values", TokenType::VALUES},
      {"and", TokenType::AND},
      {"or", TokenType::OR}
    };

    Lexer::Lexer(const std::string &input) : input_(input), position_(0) {}

    // Helper Methods

    char Lexer::Peek() const {
        if (IsAtEnd()) {
            return '\0'; // This is the Null terminator and marks the end of the string
        }
        return input_[position_];
    }

    char Lexer::Advance() {
        if (IsAtEnd()) {
            return '\0';
        }
        return input_[position_++]; // This is uses a post inrement so it returns the character tha was read
    }

    bool Lexer::IsAtEnd() const {
        return position_ >= input_.size();
    }

    char Lexer::PeekNext() const {
        if (position_ + 1 >= input_.size()) {
            return '\0';
        }
        return input_[position_ + 1];
    }

    bool Lexer::IsDigit(char c) const {
        return c >= '0' && c <= '9';
    }

    bool Lexer::IsAlpha(char c) const {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    }

    bool Lexer::IsAlphaNumeric(char c) const {
        return IsAlpha(c) || IsDigit(c);
    }

    void Lexer::SkipWhiteSpace() {
        while (!IsAtEnd() && (Peek() == ' ' || Peek() == '\t' || Peek() == '\n')) {
            Advance();
        }
    }

    Token Lexer::ScanNumber() {
        size_t start = position_;

        while (!IsAtEnd() && IsDigit(Peek())) {
            Advance();
        }
        
        std::string number = input_.substr(start, position_ - start);
        return Token(TokenType::NUMBER, number);
    }

    Token Lexer::ScanIdentifierOrKeyword() {
        size_t start = position_;

        while(!IsAtEnd() && IsAlphaNumeric(Peek())) {
            Advance();
        }

        std::string identifier_or_keyword = input_.substr(start, position_ - start);
        std::string key = identifier_or_keyword;
        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c){ return std::tolower(c); });
        auto it = keywords_.find(key);

        if (it != keywords_.end()) {
            return Token(it->second, identifier_or_keyword);
        }

        return Token(TokenType::IDENTIFIER, identifier_or_keyword);
    }

    Token Lexer::ScanString() {


        if (Peek() != '\'') {
            return Token(TokenType::INVALID, "String type not opened with single quotation mark.");
        }

        Advance();
        size_t start = position_;
        while (!IsAtEnd() && Peek() != '\'') {
            Advance();
        }

        if (IsAtEnd()) {
            return Token(TokenType::INVALID, "String type not terminated with closing single quotation mark");
        }

        std::string str = input_.substr(start, position_ - start);
        Advance();

        return Token(TokenType::STRING, str);
    }

    Token Lexer::ScanOperator() {
      char current = Peek();

      switch (current) {
          case '=':
              Advance();
              return Token(TokenType::EQUALS, "=");

          case '!':
            Advance();
              if (Peek() == '=') {  // Look ahead!
                  Advance();
                  return Token(TokenType::NOT_EQUALS, "!=");
              }
              // If not followed by '=', it's invalid
              return Token(TokenType::INVALID, "!");

          case '<':
            Advance();
              if (Peek() == '=') {
                  Advance();
                  return Token(TokenType::LESS_EQUAL, "<=");
              }
              return Token(TokenType::LESS_THAN, "<");

          case '>':
              Advance();
              if (Peek() == '=') {
                  Advance();
                  return Token(TokenType::GREATER_EQUAL, ">=");
              }
              return Token(TokenType::GREATER_THAN, ">");

          default:
              Advance();
              return Token(TokenType::INVALID, std::string(1, current));
      }
    }

    std::vector<Token> Lexer::Tokenize() {
        std::vector<Token> tokens;

        while (!IsAtEnd()) {
            SkipWhiteSpace();

            if (IsAtEnd()) break;

            char c = Peek();

            if (IsDigit(c)) {
                tokens.push_back(ScanNumber());
            }
            else if (IsAlpha(c)) {
                tokens.push_back(ScanIdentifierOrKeyword());
            }
            else if (c == '\'') {
                tokens.push_back(ScanString());
            }
            else if (c == '=' || c == '!' || c == '<' || c == '>') {
                tokens.push_back(ScanOperator());
            }
            else if (c == ',') {
                tokens.push_back(Token(TokenType::COMMA, ","));
                Advance();
            }
            else if (c == '(') {
                tokens.push_back(Token(TokenType::LPAREN, "("));
                Advance();
            }
            else if (c == ')') {
                tokens.push_back(Token(TokenType::RPAREN, ")"));
                Advance();
            }
            else if (c == ';') {
                tokens.push_back(Token(TokenType::SEMICOLON, ";"));
                Advance();
            }
            else if (c == '*') {
                tokens.push_back(Token(TokenType::STAR, "*"));
                Advance();
            }
            else {
                tokens.push_back(Token(TokenType::INVALID, std::string(1, c)));
                Advance();
            }
        }

        tokens.push_back(Token(TokenType::END_OF_FILE, ""));
        return tokens;
    }
}