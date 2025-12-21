#pragma once

#include "type/value.h"
#include "parser/ast_node.h"
#include "parser/ast_visitor.h"
#include "parser/token.h"

#include <memory>
#include <string>
#include <vector>
#include <variant>

namespace dbengine {
    class Expression : public ASTNode {
        public:
            Expression(NodeType type, SourceLocation location) : ASTNode(type, location) {}

            virtual ~Expression() = default;
            
            virtual void Accept(ASTVisitor* visitor) = 0;
    };

    class ColumnExpression : public Expression {
        private:
            std::string column_name_;

        public:
            ColumnExpression(const std::string& column_name, SourceLocation location) : Expression(NodeType::COLUMN_EXPRESSION, location),
            column_name_(column_name) {}

            const std::string& GetColumnName() const {
                return column_name_;
            }

            void Accept(ASTVisitor* visitor) override {
                visitor->Visit(this);
            }
    };

    class LiteralExpression : public Expression {
        private:
            Value value_;
        
        public:
            LiteralExpression(const Value& value, SourceLocation location)
            : Expression(NodeType::LITERAL_EXPRESSION, location), 
                value_(value)
            {}

            const Value& GetValue() const {return value_; }

            void Accept(ASTVisitor* visitor) override {
                visitor->Visit(this);
            }
    };

    class BinaryExpression : public Expression {
        private:
            std::unique_ptr<Expression> left_;
            TokenType operator_;
            std::unique_ptr<Expression> right_;

        public:
            BinaryExpression(
                std::unique_ptr<Expression> left, 
                TokenType op, 
                std::unique_ptr<Expression> right,
                SourceLocation location
            ) : Expression(NodeType::BINARY_EXPRESSION, location), 
                left_(std::move(left)), 
                operator_(op), 
                right_(std::move(right)) 
            {}

            Expression* GetLeft() const {
                return left_.get();
            }

            TokenType GetOperator() const {
                return operator_;
            }

            Expression* GetRight() const {
                return right_.get();
            }

            void Accept(ASTVisitor* visitor) override {
                visitor->Visit(this);
            }

    };
 
    class StarExpression : public Expression {
        public:
            StarExpression(SourceLocation location)
            : Expression(NodeType::STAR_EXPRESSION, location)
            {}

            void Accept(ASTVisitor* visitor) override {
                visitor->Visit(this);
            }
    };
}