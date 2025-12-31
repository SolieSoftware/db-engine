#pragma once

#include "parser/ast_visitor.h"
#include "parser/statement.h"
#include "parser/expression.h"
#include "parser/token.h"
#include <sstream>
#include <string>

namespace dbengine {

class PrettyPrintVisitor : public ASTVisitor {
public:
    PrettyPrintVisitor() : output_(), first_in_list_(true) {}

    std::string GetOutput() const { return output_.str(); }

    void Visit(SelectStatement* node) override {
        output_ << "SELECT ";

        first_in_list_ = true;
        for (auto& expr : node->GetSelectList()) {
            if (!first_in_list_) {
                output_ << ", ";
            }
            first_in_list_ = false;
            expr->Accept(this);
        }

        output_ << " FROM ";
        node->GetTableName()->Accept(this);

        if (node->GetWhereClause()) {
            output_ << " WHERE ";
            node->GetWhereClause()->Accept(this);
        }
    }

    void Visit(ColumnExpression* node) override {
        output_ << node->GetColumnName();
    }

    void Visit(LiteralExpression* node) override {
        const Value& val = node->GetValue();
        if (val.GetType() == TypeId::INTEGER) {
            output_ << val.GetAsInt();
        } else if (val.GetType() == TypeId::VARCHAR) {
            output_ << "'" << val.GetAsString() << "'";
        }
    }

    void Visit(BinaryExpression* node) override {
        node->GetLeft()->Accept(this);
        output_ << " " << TokenTypeToString(node->GetOperator()) << " ";
        node->GetRight()->Accept(this);
    }

    void Visit(StarExpression* node) override {
        (void)node;  // Unused
        output_ << "*";
    }

    void Visit(InsertStatement* node) override {
        output_ << "INSERT INTO ";
        node->GetTableName()->Accept(this);

        output_ << " (";
        first_in_list_ = true;
        for (const auto& col : node->GetColumnNames()) {
            if (!first_in_list_) output_ << ", ";
            first_in_list_ = false;
            output_ << col;
        }
        output_ << ")";

        output_ << " VALUES (";
        first_in_list_ = true;
        for (auto& val : const_cast<InsertStatement*>(node)->GetValues()) {
            if (!first_in_list_) output_ << ", ";
            first_in_list_ = false;
            val->Accept(this);
        }
        output_ << ")";
    }

private:
    std::stringstream output_;
    bool first_in_list_;

    std::string TokenTypeToString(TokenType type) const {
        switch(type) {
            case TokenType::AND: return "AND";
            case TokenType::OR: return "OR";
            case TokenType::EQUALS: return "=";
            case TokenType::NOT_EQUALS: return "!=";
            case TokenType::LESS_THAN: return "<";
            case TokenType::GREATER_THAN: return ">";
            case TokenType::LESS_EQUAL: return "<=";
            case TokenType::GREATER_EQUAL: return ">=";
            default: return "?";
        }
    }
};

} // namespace dbengine
