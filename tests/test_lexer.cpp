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

    // Verify we got the right tokens
    if (tokens.size() != 5) {  // SELECT, id, FROM, users, EOF
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

    // ... more checks ...

    std::cout << "✓ Basic SELECT test passed!" << std::endl;
    return true;
}

bool TestBasicInsert() {
    PrintTestHeader("Test 2: Basic INSERT");

    Lexer lexer("INSERT INTO users (id, user) VALUES (1, 'sol')");
    auto tokens = lexer.Tokenize();

    PrintTokens(tokens);

    // Verify we got the right tokens
}

int main() {
    std::cout << "=== Lexer Test Suite ===" << std::endl;

    if (!TestBasicSelect()) return 1;
    // Add more tests here...

    std::cout << "\n=== All Tests Passed! ===" << std::endl;
    return 0;
}