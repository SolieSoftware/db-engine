#include "parser/lexer.h"
#include "parser/parser.h"
#include "parser/statement.h"
#include "parser/expression.h"
#include "parser/ast_node.h"
#include <iostream>
#include <cassert>

using namespace dbengine;

void PrintTestHeader(const std::string& test_name) {
    std::cout << "\n=== " << test_name << " ===\n" << std::endl;
}

// Helper function to convert TokenType to string for better error messages
std::string TokenTypeToString(TokenType type) {
    switch(type) {
        case TokenType::SELECT: return "SELECT";
        case TokenType::FROM: return "FROM";
        case TokenType::WHERE: return "WHERE";
        case TokenType::AND: return "AND";
        case TokenType::OR: return "OR";
        case TokenType::EQUALS: return "=";
        case TokenType::NOT_EQUALS: return "!=";
        case TokenType::LESS_THAN: return "<";
        case TokenType::GREATER_THAN: return ">";
        case TokenType::LESS_EQUAL: return "<=";
        case TokenType::GREATER_EQUAL: return ">=";
        default: return "UNKNOWN";
    }
}

// Helper function to convert NodeType to string
std::string NodeTypeToString(NodeType type) {
    switch(type) {
        case NodeType::SELECT_STATEMENT: return "SelectStatement";
        case NodeType::COLUMN_EXPRESSION: return "ColumnExpression";
        case NodeType::LITERAL_EXPRESSION: return "LiteralExpression";
        case NodeType::BINARY_EXPRESSION: return "BinaryExpression";
        case NodeType::STAR_EXPRESSION: return "StarExpression";
        default: return "UNKNOWN";
    }
}

bool TestBasicSelect() {
    PrintTestHeader("Test 1: SELECT * FROM users");

    try {
        Lexer lexer("SELECT * FROM users");
        auto tokens = lexer.Tokenize();

        Parser parser(tokens);
        auto stmt = parser.ParseSelectStatement();

        // Verify we got a SelectStatement
        assert(stmt != nullptr);
        std::cout << "✓ Parser returned a valid SelectStatement" << std::endl;

        // Verify select list has 1 item (the *)
        assert(stmt->GetSelectList().size() == 1);
        std::cout << "✓ Select list has 1 item" << std::endl;

        // Verify it's a StarExpression
        Expression* first_expr = stmt->GetSelectList()[0].get();
        assert(first_expr->GetNodeType() == NodeType::STAR_EXPRESSION);
        std::cout << "✓ First item is StarExpression" << std::endl;

        // Verify table name
        Expression* table_expr = stmt->GetTableName();
        assert(table_expr != nullptr);
        assert(table_expr->GetNodeType() == NodeType::COLUMN_EXPRESSION);

        ColumnExpression* table_col = static_cast<ColumnExpression*>(table_expr);
        assert(table_col->GetColumnName() == "users");
        std::cout << "✓ Table name is 'users'" << std::endl;

        // Verify no WHERE clause
        assert(stmt->GetWhereClause() == nullptr);
        std::cout << "✓ No WHERE clause (correct)" << std::endl;

        std::cout << "✓ Test passed!" << std::endl;
        return true;

    } catch (const ParseException& e) {
        std::cout << "✗ ParseException: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cout << "✗ Exception: " << e.what() << std::endl;
        return false;
    }
}

bool TestSelectSingleColumn() {
    PrintTestHeader("Test 2: SELECT id FROM users");

    try {
        Lexer lexer("SELECT id FROM users");
        auto tokens = lexer.Tokenize();

        Parser parser(tokens);
        auto stmt = parser.ParseSelectStatement();

        assert(stmt != nullptr);
        std::cout << "✓ Parser returned a valid SelectStatement" << std::endl;

        // Verify select list has 1 column
        assert(stmt->GetSelectList().size() == 1);
        std::cout << "✓ Select list has 1 item" << std::endl;

        // Verify it's a ColumnExpression with name "id"
        Expression* col_expr = stmt->GetSelectList()[0].get();
        assert(col_expr->GetNodeType() == NodeType::COLUMN_EXPRESSION);

        ColumnExpression* column = static_cast<ColumnExpression*>(col_expr);
        assert(column->GetColumnName() == "id");
        std::cout << "✓ Column name is 'id'" << std::endl;

        std::cout << "✓ Test passed!" << std::endl;
        return true;

    } catch (const ParseException& e) {
        std::cout << "✗ ParseException: " << e.what() << std::endl;
        return false;
    }
}

bool TestSelectMultipleColumns() {
    PrintTestHeader("Test 3: SELECT id, name, age FROM users");

    try {
        Lexer lexer("SELECT id, name, age FROM users");
        auto tokens = lexer.Tokenize();

        Parser parser(tokens);
        auto stmt = parser.ParseSelectStatement();

        assert(stmt != nullptr);
        std::cout << "✓ Parser returned a valid SelectStatement" << std::endl;

        // Verify select list has 3 columns
        assert(stmt->GetSelectList().size() == 3);
        std::cout << "✓ Select list has 3 items" << std::endl;

        // Verify column names
        ColumnExpression* col1 = static_cast<ColumnExpression*>(stmt->GetSelectList()[0].get());
        ColumnExpression* col2 = static_cast<ColumnExpression*>(stmt->GetSelectList()[1].get());
        ColumnExpression* col3 = static_cast<ColumnExpression*>(stmt->GetSelectList()[2].get());

        assert(col1->GetColumnName() == "id");
        assert(col2->GetColumnName() == "name");
        assert(col3->GetColumnName() == "age");
        std::cout << "✓ Column names are 'id', 'name', 'age'" << std::endl;

        std::cout << "✓ Test passed!" << std::endl;
        return true;

    } catch (const ParseException& e) {
        std::cout << "✗ ParseException: " << e.what() << std::endl;
        return false;
    }
}

bool TestSelectWithSimpleWhere() {
    PrintTestHeader("Test 4: SELECT id FROM users WHERE age > 18");

    try {
        Lexer lexer("SELECT id FROM users WHERE age > 18");
        auto tokens = lexer.Tokenize();

        Parser parser(tokens);
        auto stmt = parser.ParseSelectStatement();

        assert(stmt != nullptr);
        std::cout << "✓ Parser returned a valid SelectStatement" << std::endl;

        // Verify WHERE clause exists
        Expression* where = stmt->GetWhereClause();
        assert(where != nullptr);
        std::cout << "✓ WHERE clause exists" << std::endl;

        // Verify it's a BinaryExpression
        assert(where->GetNodeType() == NodeType::BINARY_EXPRESSION);
        BinaryExpression* binary = static_cast<BinaryExpression*>(where);
        std::cout << "✓ WHERE clause is a BinaryExpression" << std::endl;

        // Verify operator is GREATER_THAN
        assert(binary->GetOperator() == TokenType::GREATER_THAN);
        std::cout << "✓ Operator is >" << std::endl;

        // Verify left side is column "age"
        Expression* left = binary->GetLeft();
        assert(left->GetNodeType() == NodeType::COLUMN_EXPRESSION);
        ColumnExpression* left_col = static_cast<ColumnExpression*>(left);
        assert(left_col->GetColumnName() == "age");
        std::cout << "✓ Left operand is column 'age'" << std::endl;

        // Verify right side is literal 18
        Expression* right = binary->GetRight();
        assert(right->GetNodeType() == NodeType::LITERAL_EXPRESSION);
        LiteralExpression* right_lit = static_cast<LiteralExpression*>(right);
        assert(right_lit->GetValue().GetType() == TypeId::INTEGER);
        assert(right_lit->GetValue().GetAsInt() == 18);
        std::cout << "✓ Right operand is literal 18" << std::endl;

        std::cout << "✓ Test passed!" << std::endl;
        return true;

    } catch (const ParseException& e) {
        std::cout << "✗ ParseException: " << e.what() << std::endl;
        return false;
    }
}

bool TestSelectWithComplexWhere() {
    PrintTestHeader("Test 5: SELECT name FROM users WHERE age > 18 AND status = 'active'");

    try {
        Lexer lexer("SELECT name FROM users WHERE age > 18 AND status = 'active'");
        auto tokens = lexer.Tokenize();

        Parser parser(tokens);
        auto stmt = parser.ParseSelectStatement();

        assert(stmt != nullptr);
        std::cout << "✓ Parser returned a valid SelectStatement" << std::endl;

        // Verify WHERE clause exists and is a BinaryExpression with AND
        Expression* where = stmt->GetWhereClause();
        assert(where != nullptr);
        assert(where->GetNodeType() == NodeType::BINARY_EXPRESSION);

        BinaryExpression* and_expr = static_cast<BinaryExpression*>(where);
        assert(and_expr->GetOperator() == TokenType::AND);
        std::cout << "✓ Top-level WHERE operator is AND" << std::endl;

        // Verify left side: age > 18
        Expression* left = and_expr->GetLeft();
        assert(left->GetNodeType() == NodeType::BINARY_EXPRESSION);
        BinaryExpression* left_binary = static_cast<BinaryExpression*>(left);
        assert(left_binary->GetOperator() == TokenType::GREATER_THAN);

        ColumnExpression* age_col = static_cast<ColumnExpression*>(left_binary->GetLeft());
        assert(age_col->GetColumnName() == "age");

        LiteralExpression* age_val = static_cast<LiteralExpression*>(left_binary->GetRight());
        assert(age_val->GetValue().GetAsInt() == 18);
        std::cout << "✓ Left side is: age > 18" << std::endl;

        // Verify right side: status = 'active'
        Expression* right = and_expr->GetRight();
        assert(right->GetNodeType() == NodeType::BINARY_EXPRESSION);
        BinaryExpression* right_binary = static_cast<BinaryExpression*>(right);
        assert(right_binary->GetOperator() == TokenType::EQUALS);

        ColumnExpression* status_col = static_cast<ColumnExpression*>(right_binary->GetLeft());
        assert(status_col->GetColumnName() == "status");

        LiteralExpression* status_val = static_cast<LiteralExpression*>(right_binary->GetRight());
        assert(status_val->GetValue().GetType() == TypeId::VARCHAR);
        assert(status_val->GetValue().GetAsString() == "active");
        std::cout << "✓ Right side is: status = 'active'" << std::endl;

        std::cout << "✓ Test passed!" << std::endl;
        return true;

    } catch (const ParseException& e) {
        std::cout << "✗ ParseException: " << e.what() << std::endl;
        return false;
    }
}

bool TestSelectWithOr() {
    PrintTestHeader("Test 6: SELECT id FROM users WHERE age < 18 OR age > 65");

    try {
        Lexer lexer("SELECT id FROM users WHERE age < 18 OR age > 65");
        auto tokens = lexer.Tokenize();

        Parser parser(tokens);
        auto stmt = parser.ParseSelectStatement();

        assert(stmt != nullptr);

        // Verify WHERE clause has OR operator
        Expression* where = stmt->GetWhereClause();
        assert(where->GetNodeType() == NodeType::BINARY_EXPRESSION);

        BinaryExpression* or_expr = static_cast<BinaryExpression*>(where);
        assert(or_expr->GetOperator() == TokenType::OR);
        std::cout << "✓ Top-level WHERE operator is OR" << std::endl;

        // Verify left: age < 18
        BinaryExpression* left = static_cast<BinaryExpression*>(or_expr->GetLeft());
        assert(left->GetOperator() == TokenType::LESS_THAN);
        std::cout << "✓ Left side uses < operator" << std::endl;

        // Verify right: age > 65
        BinaryExpression* right = static_cast<BinaryExpression*>(or_expr->GetRight());
        assert(right->GetOperator() == TokenType::GREATER_THAN);
        std::cout << "✓ Right side uses > operator" << std::endl;

        std::cout << "✓ Test passed!" << std::endl;
        return true;

    } catch (const ParseException& e) {
        std::cout << "✗ ParseException: " << e.what() << std::endl;
        return false;
    }
}

bool TestSelectWithParentheses() {
    PrintTestHeader("Test 7: SELECT id FROM users WHERE (age > 18)");

    try {
        Lexer lexer("SELECT id FROM users WHERE (age > 18)");
        auto tokens = lexer.Tokenize();

        Parser parser(tokens);
        auto stmt = parser.ParseSelectStatement();

        assert(stmt != nullptr);

        // Parentheses don't create extra nodes, just affect parsing order
        Expression* where = stmt->GetWhereClause();
        assert(where->GetNodeType() == NodeType::BINARY_EXPRESSION);

        BinaryExpression* binary = static_cast<BinaryExpression*>(where);
        assert(binary->GetOperator() == TokenType::GREATER_THAN);
        std::cout << "✓ Parentheses parsed correctly (no extra nodes)" << std::endl;

        std::cout << "✓ Test passed!" << std::endl;
        return true;

    } catch (const ParseException& e) {
        std::cout << "✗ ParseException: " << e.what() << std::endl;
        return false;
    }
}

bool TestOperatorPrecedence() {
    PrintTestHeader("Test 8: Operator Precedence - a OR b AND c");

    try {
        // Should parse as: a OR (b AND c)
        // NOT as: (a OR b) AND c
        Lexer lexer("SELECT id FROM users WHERE a = 1 OR b = 2 AND c = 3");
        auto tokens = lexer.Tokenize();

        Parser parser(tokens);
        auto stmt = parser.ParseSelectStatement();

        assert(stmt != nullptr);

        Expression* where = stmt->GetWhereClause();
        assert(where->GetNodeType() == NodeType::BINARY_EXPRESSION);

        BinaryExpression* root = static_cast<BinaryExpression*>(where);

        // Root should be OR (lowest precedence)
        assert(root->GetOperator() == TokenType::OR);
        std::cout << "✓ Root operator is OR (correct precedence)" << std::endl;

        // Left of OR should be simple comparison: a = 1
        BinaryExpression* left = static_cast<BinaryExpression*>(root->GetLeft());
        assert(left->GetOperator() == TokenType::EQUALS);
        std::cout << "✓ Left of OR is: a = 1" << std::endl;

        // Right of OR should be AND
        BinaryExpression* right = static_cast<BinaryExpression*>(root->GetRight());
        assert(right->GetOperator() == TokenType::AND);
        std::cout << "✓ Right of OR is AND expression (b = 2 AND c = 3)" << std::endl;

        std::cout << "✓ Test passed! AND binds tighter than OR" << std::endl;
        return true;

    } catch (const ParseException& e) {
        std::cout << "✗ ParseException: " << e.what() << std::endl;
        return false;
    }
}

bool TestErrorMissingSELECT() {
    PrintTestHeader("Test 9: Error - Missing SELECT keyword");

    try {
        Lexer lexer("id FROM users");
        auto tokens = lexer.Tokenize();

        Parser parser(tokens);
        auto stmt = parser.ParseSelectStatement();

        std::cout << "✗ Should have thrown ParseException!" << std::endl;
        return false;

    } catch (const ParseException& e) {
        std::cout << "✓ Caught expected ParseException: " << e.what() << std::endl;
        return true;
    }
}

bool TestErrorMissingFROM() {
    PrintTestHeader("Test 10: Error - Missing FROM keyword");

    try {
        Lexer lexer("SELECT id users");
        auto tokens = lexer.Tokenize();

        Parser parser(tokens);
        auto stmt = parser.ParseSelectStatement();

        std::cout << "✗ Should have thrown ParseException!" << std::endl;
        return false;

    } catch (const ParseException& e) {
        std::cout << "✓ Caught expected ParseException: " << e.what() << std::endl;
        return true;
    }
}

bool TestErrorMissingTableName() {
    PrintTestHeader("Test 11: Error - Missing table name");

    try {
        Lexer lexer("SELECT id FROM");
        auto tokens = lexer.Tokenize();

        Parser parser(tokens);
        auto stmt = parser.ParseSelectStatement();

        std::cout << "✗ Should have thrown ParseException!" << std::endl;
        return false;

    } catch (const ParseException& e) {
        std::cout << "✓ Caught expected ParseException: " << e.what() << std::endl;
        return true;
    }
}

bool TestSelectWithSemicolon() {
    PrintTestHeader("Test 12: SELECT with optional semicolon");

    try {
        Lexer lexer("SELECT id FROM users;");
        auto tokens = lexer.Tokenize();

        Parser parser(tokens);
        auto stmt = parser.ParseSelectStatement();

        assert(stmt != nullptr);
        std::cout << "✓ Parser handles optional semicolon correctly" << std::endl;

        std::cout << "✓ Test passed!" << std::endl;
        return true;

    } catch (const ParseException& e) {
        std::cout << "✗ ParseException: " << e.what() << std::endl;
        return false;
    }
}

int main() {
    std::cout << "=== Parser Test Suite ===" << std::endl;
    std::cout << "Testing recursive descent parser with operator precedence\n" << std::endl;

    int passed = 0;
    int total = 0;

    // Basic SELECT tests
    total++; if (TestBasicSelect()) passed++;
    total++; if (TestSelectSingleColumn()) passed++;
    total++; if (TestSelectMultipleColumns()) passed++;

    // WHERE clause tests
    total++; if (TestSelectWithSimpleWhere()) passed++;
    total++; if (TestSelectWithComplexWhere()) passed++;
    total++; if (TestSelectWithOr()) passed++;
    total++; if (TestSelectWithParentheses()) passed++;

    // Operator precedence test
    total++; if (TestOperatorPrecedence()) passed++;

    // Error handling tests
    total++; if (TestErrorMissingSELECT()) passed++;
    total++; if (TestErrorMissingFROM()) passed++;
    total++; if (TestErrorMissingTableName()) passed++;

    // Edge cases
    total++; if (TestSelectWithSemicolon()) passed++;

    std::cout << "\n===========================================\n";
    std::cout << "Test Results: " << passed << "/" << total << " passed";

    if (passed == total) {
        std::cout << " ✓✓✓\n";
        std::cout << "All tests passed! Parser is working correctly.\n";
        std::cout << "===========================================\n";
        return 0;
    } else {
        std::cout << " ✗✗✗\n";
        std::cout << (total - passed) << " test(s) failed.\n";
        std::cout << "===========================================\n";
        return 1;
    }
}
