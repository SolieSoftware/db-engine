#pragma once

#include "execution/executor.h"
#include "storage/table/table_iterator.h"
#include <string>
#include <memory>

namespace dbengine {

class SeqScanExecutor : public Executor {
public:
    SeqScanExecutor(ExecutionContext *context, const std::string &table_name)
        : Executor(context), table_name_(table_name), iterator_(nullptr) {}

    ~SeqScanExecutor() override = default;

    void Init() override {
        TableHeap *table = context_->GetTable(table_name_);
        if (table == nullptr) {
            throw std::runtime_error("Table not found: " + table_name_);
        }

        iterator_ = std::make_unique<TableIterator>(
            table, context_->GetBufferPoolManager()
        );
    }

    bool Next(Tuple &tuple, RID &rid) override {
        if (iterator_ == nullptr) {
            return false;
        }

        return iterator_->Next(tuple, rid);
    }

private:
    std::string table_name_;
    std::unique_ptr<TableIterator> iterator_;
};

}
