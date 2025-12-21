#pragma once

namespace dbengine {
    class SelectStatement;
    class InsertStatement;
    class ColumnExpression;
    class LiteralExpression;
    class BinaryExpression;
    class StarExpression;

    class ASTVisitor {
        public:
            virtual ~ASTVisitor() = default;

            virtual void Visit(SelectStatement* node) = 0;
            virtual void Visit(InsertStatement* node) = 0;
            virtual void Visit(ColumnExpression* node) = 0;
            virtual void Visit(LiteralExpression* node) = 0;
            virtual void Visit(BinaryExpression* node) = 0;
            virtual void Visit(StarExpression* node) = 0;
    };
}