#pragma once

#include <string>


namespace dbengine {
    class ASTVisitor;

    struct SourceLocation {
            int line = 0;
            int column = 0;
            std::string filename;
    };

    enum class NodeType {
        SELECT_STATEMENT,
        INSERT_STATEMENT,
        WHERE_CLAUSE,
        BINARY_EXPRESSION,
        COLUMN_EXPRESSION,
        STAR_EXPRESSION,
        LITERAL_EXPRESSION,
        TABLE_REF,
        FROM_STATEMENT
    };

    class ASTNode {
        protected:
            NodeType node_type_;
            SourceLocation location_;

        public:
            ASTNode(NodeType type, SourceLocation loc) : node_type_(type), location_(loc) {}

            virtual ~ASTNode() = default;
            virtual void Accept(ASTVisitor* visitor) = 0;

            NodeType GetNodeType() const { return node_type_; }
            SourceLocation GetLocation() const { return location_; }
    };
}