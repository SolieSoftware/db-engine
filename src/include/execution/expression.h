#pragma once

#include "type/value.h"
#include "catalog/schema.h"
#include "storage/table/tuple.h"
#include <memory>

namespace dbengine {

enum class ExpressionType {
    COLUMN_REF,
    CONSTANT,
    COMPARE_EQUAL,
    COMPARE_NOT_EQUAL,
    COMPARE_LESS_THAN,
    COMPARE_GREATER_THAN
};

class Expression {
public:
    explicit Expression(ExpressionType type) : type_(type) {}
    virtual ~Expression() = default;

    virtual Value Evaluate(const Tuple &tuple, const Schema *schema) const = 0;

    ExpressionType GetType() const { return type_; }

protected:
    ExpressionType type_;
};

class ColumnExpression : public Expression {
public:
    explicit ColumnExpression(uint32_t col_idx)
        : Expression(ExpressionType::COLUMN_REF), col_idx_(col_idx) {}

    Value Evaluate(const Tuple &tuple, const Schema *schema) const override {
        uint32_t offset = schema->GetColumnOffset(col_idx_);
        const Column &col = schema->GetColumn(col_idx_);
        return Value::DeserializeFrom(
            tuple.GetData() + offset,
            col.GetType(),
            col.GetFixedLength()
        );
    }

private:
    uint32_t col_idx_;
};

class ConstantExpression : public Expression {
public:
    explicit ConstantExpression(const Value &value)
        : Expression(ExpressionType::CONSTANT), value_(value) {}

    Value Evaluate(const Tuple & /* tuple */, const Schema * /* schema */) const override {
        return value_;
    }

private:
    Value value_;
};

class ComparisonExpression : public Expression {
public:
    ComparisonExpression(ExpressionType type,
                        std::unique_ptr<Expression> left,
                        std::unique_ptr<Expression> right)
        : Expression(type), left_(std::move(left)), right_(std::move(right)) {}

    Value Evaluate(const Tuple &tuple, const Schema *schema) const override {
        Value left_val = left_->Evaluate(tuple, schema);
        Value right_val = right_->Evaluate(tuple, schema);

        bool result = false;
        switch (type_) {
            case ExpressionType::COMPARE_EQUAL:
                result = (left_val == right_val);
                break;
            case ExpressionType::COMPARE_NOT_EQUAL:
                result = (left_val != right_val);
                break;
            case ExpressionType::COMPARE_LESS_THAN:
                result = (left_val < right_val);
                break;
            case ExpressionType::COMPARE_GREATER_THAN:
                result = (left_val > right_val);
                break;
            default:
                break;
        }

        return Value(static_cast<int32_t>(result));
    }

private:
    std::unique_ptr<Expression> left_;
    std::unique_ptr<Expression> right_;
};

}
