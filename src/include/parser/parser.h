#pragma once

#include "parser/token.h"
#include "parser/statement.h"
#include "parser/expression.h"
#include "parser/ast_node.h"

#include <vector>
#include <memory>
#include <string>
#include <stdexcept>

  namespace dbengine {

  // Exception for parser errors
  class ParseException : public std::runtime_error {
  public:
      ParseException(const std::string& message, const Token& token)
          : std::runtime_error(message + " at token: " + token.GetTokenContent()) {}
  };

  class Parser {
  private:
      std::vector<Token> tokens_;
      size_t current_;

      // Helper methods
      Token Peek() const;
      Token Previous() const;
      Token Advance();
      bool Check(TokenType type) const;
      bool Match(TokenType type);
      Token Expect(TokenType type, const std::string& message);
      bool IsAtEnd() const;
      SourceLocation GetCurrentLocation() const;

      // Parsing methods 
      std::vector<std::unique_ptr<Expression>> ParseSelectList();
      std::unique_ptr<Expression> ParseExpression();
      std::unique_ptr<Expression> ParseOrExpression();
      std::unique_ptr<Expression> ParseAndExpression();
      std::unique_ptr<Expression> ParseComparison();
      std::unique_ptr<Expression> ParsePrimary();

  public:
      explicit Parser(const std::vector<Token>& tokens);

      std::unique_ptr<SelectStatement> ParseSelectStatement();
      std::unique_ptr<InsertStatement> ParseInsertStatement();
  };

  } 