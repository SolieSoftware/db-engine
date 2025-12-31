#pragma once

#include "parser/expression.h"
#include "parser/ast_node.h"

#include <vector>
#include <memory>

namespace dbengine {
    class Statement : public ASTNode {
        public: 
            Statement(NodeType type, SourceLocation location) 
            : ASTNode(type, location)
            {}

            virtual ~Statement() = default;
            virtual void Accept(ASTVisitor* visitor) = 0; 
    };

    class SelectStatement : public Statement {
        private:
            std::vector<std::unique_ptr<Expression>> select_list_;
            std::unique_ptr<Expression> table_name_;
            std::unique_ptr<Expression> where_clause_;
            SourceLocation location_;

        public:
            SelectStatement(
                std::vector<std::unique_ptr<Expression>> select_list,
                std::unique_ptr<Expression> table_name,
                std::unique_ptr<Expression> where_clause,
                SourceLocation location
            ) : Statement(NodeType::SELECT_STATEMENT, location),
                select_list_(std::move(select_list)),
                table_name_(std::move(table_name)),
                where_clause_(std::move(where_clause)) {}

            std::vector<std::unique_ptr<Expression>>& GetSelectList() {
                return select_list_;
            }

            Expression* GetTableName() const {
                return table_name_.get();
            }
            
            // Return raw pointer for where => Owneship stays with the SelectStatement
            Expression* GetWhereClause() const {
                return where_clause_.get();
            }

            void Accept(ASTVisitor* visitor) override {
                visitor->Visit(this);
            }
    };

    class InsertStatement : public Statement {
        private:
            std::unique_ptr<Expression> table_name_;
            std::vector<std::string> column_names_;
            std::vector<std::unique_ptr<Expression>> values_;

        public:
            InsertStatement(
                std::unique_ptr<Expression> table_name,
                std::vector<std::string> column_names,
                std::vector<std::unique_ptr<Expression>> values,
                SourceLocation location
            ) : Statement(NodeType::INSERT_STATEMENT, location),
                table_name_(std::move(table_name)),
                column_names_(std::move(column_names)),
                values_(std::move(values)) {}

            Expression* GetTableName() const {
                return table_name_.get();
            }

            const std::vector<std::string>& GetColumnNames() const {
                return column_names_;
            }

            std::vector<std::unique_ptr<Expression>>& GetValues() {
                return values_;
            }

            void Accept(ASTVisitor* visitor) override {
                visitor->Visit(this);
            }
    };

}