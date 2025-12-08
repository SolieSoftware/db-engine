#pragma once

#include "execution/executor.h"
#include "catalog/schema.h"
#include "type/value.h"
#include <vector>
#include <string>

namespace dbengine {

class InsertExecutor : public Executor {
public:
    InsertExecutor(ExecutionContext *context, const std::string &table_name,
                   const std::vector<std::vector<Value>> &values)
        : Executor(context), table_name_(table_name), values_(values), cursor_(0) {}

    ~InsertExecutor() override = default;

    void Init() override {
        table_ = context_->GetTable(table_name_);
        schema_ = context_->GetSchema(table_name_);

        if (table_ == nullptr || schema_ == nullptr) {
            throw std::runtime_error("Table or schema not found: " + table_name_);
        }

        cursor_ = 0;
    }

    bool Next(Tuple &tuple, RID &rid) override {
        if (cursor_ >= values_.size()) {
            return false;
        }

        const auto &row = values_[cursor_];

        if (row.size() != schema_->GetColumnCount()) {
            throw std::runtime_error("Column count mismatch in INSERT");
        }

        uint32_t tuple_size = schema_->GetTupleSize();
        char *data = new char[tuple_size];
        std::memset(data, 0, tuple_size);

        for (size_t i = 0; i < row.size(); ++i) {
            uint32_t offset = schema_->GetColumnOffset(i);
            row[i].SerializeTo(data + offset);
        }

        Tuple insert_tuple(data, tuple_size);
        delete[] data;

        if (!table_->InsertTuple(insert_tuple, rid)) {
            return false;
        }

        tuple = insert_tuple;
        tuple.SetRID(rid);
        cursor_++;
        return true;
    }

private:
    std::string table_name_;
    std::vector<std::vector<Value>> values_;
    size_t cursor_;
    TableHeap *table_;
    Schema *schema_;
};

}
