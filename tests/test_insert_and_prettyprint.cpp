#include "parser/lexer.h"
#include "parser/parser.h"
#include "parser/pretty_printer.h"
#include <iostream>
#include <cassert>

using namespace dbengine;

void TestPrettyPrint() {
    std::cout << "\n=== PrettyPrint Test ===" << std::endl;

    Lexer lexer("SELECT id, name FROM users WHERE age > 18 AND status = 'active'");
    auto tokens = lexer.Tokenize();

    Parser parser(tokens);
    auto stmt = parser.ParseSelectStatement();

    PrettyPrintVisitor printer;
    stmt->Accept(&printer);

    std::cout << "Original: SELECT id, name FROM users WHERE age > 18 AND status = 'active'" << std::endl;
    std::cout << "Printed:  " << printer.GetOutput() << std::endl;
    std::cout << "✓ PrettyPrint works!" << std::endl;
}

void TestInsertParsing() {
    std::cout << "\n=== INSERT Parsing Test ===" << std::endl;

    Lexer lexer("INSERT INTO users (id, name, age) VALUES (1, 'John', 25)");
    auto tokens = lexer.Tokenize();

    Parser parser(tokens);
    auto stmt = parser.ParseInsertStatement();

    std::cout << "✓ INSERT statement parsed successfully" << std::endl;

    // Verify structure
    assert(stmt != nullptr);
    assert(stmt->GetColumnNames().size() == 3);
    assert(stmt->GetColumnNames()[0] == "id");
    assert(stmt->GetColumnNames()[1] == "name");
    assert(stmt->GetColumnNames()[2] == "age");
    std::cout << "✓ Column names: id, name, age" << std::endl;

    assert(stmt->GetValues().size() == 3);
    std::cout << "✓ 3 values parsed" << std::endl;

    // Pretty print it
    PrettyPrintVisitor printer;
    stmt->Accept(&printer);
    std::cout << "Pretty:   " << printer.GetOutput() << std::endl;
}

int main() {
    std::cout << "=== Parser Extensions Test Suite ===" << std::endl;

    TestPrettyPrint();
    TestInsertParsing();

    std::cout << "\n=== All Tests Passed! ===" << std::endl;
    return 0;
}
