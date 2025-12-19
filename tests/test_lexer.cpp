#include "parser/lexer.h"
#include "parser/token.h"
#include <iostream>
#include <vector>

using namespace dbengine;

void PrintTestHeader(const std::string &test_name) {
    std::cout << "\n=== " << test_name << " ===\n" << std::endl;
}

void PrintTokens(const std::vector<Token> &tokens) {
    for (const auto &token : tokens) {
        std::cout << "  [" << static_cast<int>(token.GetTokenType())
                << "] \"" << token.GetTokenContent() << "\"" << std::endl;
    }
}

bool TestBasicSelect() {
    PrintTestHeader("Test 1: Basic SELECT");

    Lexer lexer("SELECT id FROM users");
    auto tokens = lexer.Tokenize();

    PrintTokens(tokens);

    if (tokens.size() != 5) { 
        std::cout << "✗ Expected 5 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    if (tokens[0].GetTokenType() != TokenType::SELECT) {
        std::cout << "✗ First token should be SELECT" << std::endl;
        return false;
    }

    if (tokens[1].GetTokenType() != TokenType::IDENTIFIER ||
        tokens[1].GetTokenContent() != "id") {
        std::cout << "✗ Second token should be IDENTIFIER 'id'" << std::endl;
        return false;
    }

    std::cout << "✓ Basic SELECT test passed!" << std::endl;
    return true;
}

bool TestSelectStar() {
    PrintTestHeader("Test 2: Select STAR");

    Lexer lexer("SELECT * FROM users");
    auto tokens = lexer.Tokenize();

    PrintTokens(tokens);

    if (tokens.size() != 5) {
        std::cout << "✗ Expected 5 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    if (tokens[0].GetTokenType() != TokenType::SELECT) {
        std::cout << "✗ First token should be SELECT" << std::endl;
        return false;
    }

    if (tokens[1].GetTokenType() != TokenType::STAR ||
        tokens[1].GetTokenContent() != "*") {
        std::cout << "✗ Second token should be IDENTIFIER '*'" << std::endl;
        return false;
    }

    std::cout << "✓ Test SELECT STAR test passed!" << std::endl;
    return true;
}

bool TestBasicInsert() {
    PrintTestHeader("Test 2: Basic INSERT");

    Lexer lexer("INSERT INTO users (id, user) VALUES (1, 'sol')");
    auto tokens = lexer.Tokenize();

    PrintTokens(tokens);

    if (tokens.size() != 15) {
        std::cout << "✗ Expected 5 tokens, got " << tokens.size() << std::endl;
        return false;
    }

    if (tokens[0].GetTokenType() != TokenType::INSERT) {
        std::cout << "✗ First token should be INSERT" << std::endl;
        return false;
    }

    if (tokens[1].GetTokenType() != TokenType::INTO) {
        std::cout << "✗ Second token should be INTO" << std::endl;
        return false;
    }

    if (tokens[2].GetTokenType() != TokenType::IDENTIFIER) {
        std::cout << "✗ Second token should be IDENTIFIER" << std::endl;
        return false;
    }

    std::cout << "✓ Test Basic INSERT passed!" << std::endl;
    return true;
}

int main() {
    std::cout << "=== Lexer Test Suite ===" << std::endl;

    if (!TestBasicSelect()) return 1;
    if (!TestSelectStar()) return 1;
    if (!TestBasicInsert()) return 1;

    std::cout << "\n=== All Tests Passed! ===" << std::endl;
    return 0;
}