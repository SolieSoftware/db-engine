
  #include "parser/parser.h"                                                                                                                           
                                                                                                                                                       
  namespace dbengine {                                                                                                                                 
                                                                                                                                                       
  // Constructor                                                                                                                                       
  Parser::Parser(const std::vector<Token>& tokens)                                                                                                     
      : tokens_(tokens), current_(0)                                                                                                                   
  {}                                                                                                                                                   
                                                                                                                                                       
  // Helper method implementations                                                                                                                     
  Token Parser::Peek() const {                                                                                                                         
      if (current_ >= tokens_.size()) {                                                                                                                
          return tokens_.back();                                                                                                                       
      }                                                                                                                                                
      return tokens_[current_];                                                                                                                        
  }                                                                                                                                                    
                                                                                                                                                       
  Token Parser::Previous() const {                                                                                                                     
      return tokens_[current_ - 1];                                                                                                                    
  }                                                                                                                                                    
                                                                                                                                                       
  Token Parser::Advance() {                                                                                                                            
      if (!IsAtEnd()) current_++;                                                                                                                      
      return Previous();                                                                                                                               
  }                                                                                                                                                    
                                                                                                                                                       
  bool Parser::Check(TokenType type) const {                                                                                                           
      if (IsAtEnd()) return false;                                                                                                                     
      return Peek().GetTokenType() == type;                                                                                                            
  }                                                                                                                                                    
                                                                                                                                                       
  bool Parser::Match(TokenType type) {                                                                                                                 
      if (Check(type)) {                                                                                                                               
          Advance();                                                                                                                                   
          return true;                                                                                                                                 
      }                                                                                                                                                
      return false;                                                                                                                                    
  }                                                                                                                                                    
                                                                                                                                                       
  Token Parser::Expect(TokenType type, const std::string& message) {                                                                                   
      if (Check(type)) {                                                                                                                               
          return Advance();                                                                                                                            
      }                                                                                                                                                
      throw ParseException(message, Peek());                                                                                                           
  }                                                                                                                                                    
                                                                                                                                                       
  bool Parser::IsAtEnd() const {                                                                                                                       
      return Peek().GetTokenType() == TokenType::END_OF_FILE;                                                                                          
  }                                                                                                                                                    
                                                                                                                                                       
  SourceLocation Parser::GetCurrentLocation() const {                                                                                                  
      Token current_token = Peek();                                                                                                                    
      // For now, return a simple location                                                                                                                                                                                                        
      return SourceLocation{0, 0, ""};                                                                                                                 
  }                                                                                                                                                    
                                                                                                                                                       
                                                                                                      
  std::unique_ptr<SelectStatement> Parser::ParseSelectStatement() {                                                                                    
      SourceLocation start_location = GetCurrentLocation();                                                                                            
                                                                                                                                                                                                                                                                        
      Expect(TokenType::SELECT, "Expected SELECT keyword");                                                                                            
                                                                                                                                                                                                                                                                                
      auto select_list = ParseSelectList();                                                                                                            
                                                                                                                                                                                                                                                                               
      Expect(TokenType::FROM, "Expected FROM keyword");                                                                                                
                                                                                                                                                                                                                                                                         
      Token table_token = Expect(TokenType::IDENTIFIER, "Expected table name");                                                                        
      auto table_expr = std::make_unique<ColumnExpression>(
        table_token.GetTokenContent(),
        GetCurrentLocation()
      );                                                                                        
                                                                                                                                                                                                                                                                        
      std::unique_ptr<Expression> where_clause = nullptr;                                                                                              
      if (Match(TokenType::WHERE)) {                                                                                                                   
          where_clause = ParseExpression();                                                                                                            
      }                                                                                                                                                
                                                                                                                                                                                                                                                                              
      Match(TokenType::SEMICOLON);                                                                                                                     
                                                                                                                                                                                                                                                                                
      return std::make_unique<SelectStatement>(                                                                                                        
          std::move(select_list),                                                                                                                      
          std::move(table_expr),                                                                                                                                  
          std::move(where_clause),                                                                                                                     
          start_location                                                                                                                               
      );                                                                                                                                               
  }                                                                                                                                                    
                                                                                                                                                                                                                                                              
  std::vector<std::unique_ptr<Expression>> Parser::ParseSelectList() {     
    std::vector<std::unique_ptr<Expression>> select_list;

    // Handle SELECT *
    if (Match(TokenType::STAR)) {
        SourceLocation loc = GetCurrentLocation();
        select_list.push_back(std::make_unique<StarExpression>(loc));
        return select_list;
    }

    select_list.push_back(ParsePrimary());

    while (Match(TokenType::COMMA)) {
        select_list.push_back(ParsePrimary());
    }

    return select_list;
}

std::unique_ptr<Expression> Parser::ParseExpression() {      
    return ParseOrExpression();                                                                                                                                                                       
  }
  
  std::unique_ptr<Expression> Parser::ParseOrExpression() {
    auto left = ParseAndExpression();

    while (Match(TokenType::OR)) {
        SourceLocation loc = GetCurrentLocation();
        TokenType op = TokenType::OR;
        auto right = ParseAndExpression();

        left = std::make_unique<BinaryExpression>(
            std::move(left),
            op,
            std::move(right),
            loc
        );
    }
    return left;
  }

  std::unique_ptr<Expression> Parser::ParseAndExpression() {
    auto left = ParseComparison();

    while (Match(TokenType::AND)) {
        SourceLocation loc = GetCurrentLocation();
        TokenType op = TokenType::AND;
        auto right = ParseComparison();

        left = std::make_unique<BinaryExpression>(
            std::move(left),
            op,
            std::move(right),
            loc
        );
    }
    return left;
  }

  std::unique_ptr<Expression> Parser::ParseComparison() {
    auto left = ParsePrimary();

    if (Match(TokenType::EQUALS) || Match(TokenType::NOT_EQUALS) ||
        Match(TokenType::LESS_THAN) || Match(TokenType::GREATER_THAN) ||
        Match(TokenType::LESS_EQUAL) || Match(TokenType::GREATER_EQUAL)) {
            SourceLocation loc = GetCurrentLocation();
            TokenType op = Previous().GetTokenType();
            auto right = ParsePrimary();

            return std::make_unique<BinaryExpression>(
                std::move(left),
                op,
                std::move(right),
                loc
            );
        }


        return left;
  }

  std::unique_ptr<Expression> Parser::ParsePrimary() {
    SourceLocation loc = GetCurrentLocation();

    // Number literal
    if (Check(TokenType::NUMBER)) {
        Token num_token = Advance();
        int value = std::stoi(num_token.GetTokenContent());
        return std::make_unique<LiteralExpression>(
            Value(value),
            loc
        );
    }

    // String literal
    if (Check(TokenType::STRING)) {
        Token str_token = Advance();
        return std::make_unique<LiteralExpression>(
            Value(str_token.GetTokenContent()),
            loc
        );
    }

    // Column reference (identifier)
    if (Check(TokenType::IDENTIFIER)) {
        Token id_token = Advance();
        return std::make_unique<ColumnExpression>(
            id_token.GetTokenContent(),
            loc
        );
    }

    // Parenthesized expression
    if (Match(TokenType::LPAREN)) {
        auto expr = ParseExpression();
        Expect(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }

    throw ParseException("Expected expression", Peek());
}

std::unique_ptr<InsertStatement> Parser::ParseInsertStatement() {
    SourceLocation start_location = GetCurrentLocation();

    Expect(TokenType::INSERT, "Expected INSERT keyword");
    Expect(TokenType::INTO, "Expected INTO keyword");

    Token table_token = Expect(TokenType::IDENTIFIER, "Expected table name");
    auto table_expr = std::make_unique<ColumnExpression>(
        table_token.GetTokenContent(),
        GetCurrentLocation()
    );

    // Parse column list: (col1, col2, col3)
    std::vector<std::string> column_names;
    Expect(TokenType::LPAREN, "Expected '(' after table name");

    if (!Check(TokenType::RPAREN)) {
        Token col = Expect(TokenType::IDENTIFIER, "Expected column name");
        column_names.push_back(col.GetTokenContent());

        while (Match(TokenType::COMMA)) {
            Token col = Expect(TokenType::IDENTIFIER, "Expected column name");
            column_names.push_back(col.GetTokenContent());
        }
    }

    Expect(TokenType::RPAREN, "Expected ')' after column list");

    // Parse VALUES keyword and value list
    Expect(TokenType::VALUES, "Expected VALUES keyword");
    Expect(TokenType::LPAREN, "Expected '(' after VALUES");

    std::vector<std::unique_ptr<Expression>> values;
    if (!Check(TokenType::RPAREN)) {
        values.push_back(ParsePrimary());

        while (Match(TokenType::COMMA)) {
            values.push_back(ParsePrimary());
        }
    }

    Expect(TokenType::RPAREN, "Expected ')' after value list");
    Match(TokenType::SEMICOLON);  // Optional semicolon

    return std::make_unique<InsertStatement>(
        std::move(table_expr),
        std::move(column_names),
        std::move(values),
        start_location
    );
}

}  // namespace dbengine                                                                                                                         
                      