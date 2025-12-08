#pragma once

#include "storage/buffer/buffer_pool_manager.h"
#include "catalog/schema.h"
#include <unordered_map>
#include <string>

namespace dbengine {

class TableHeap;

class ExecutionContext {
public:
    ExecutionContext(BufferPoolManager *bpm) : bpm_(bpm) {}

    inline BufferPoolManager* GetBufferPoolManager() { return bpm_; }

    void RegisterTable(const std::string &table_name, TableHeap *table_heap, Schema *schema) {
        tables_[table_name] = table_heap;
        schemas_[table_name] = schema;
    }

    TableHeap* GetTable(const std::string &table_name) {
        auto it = tables_.find(table_name);
        if (it != tables_.end()) {
            return it->second;
        }
        return nullptr;
    }

    Schema* GetSchema(const std::string &table_name) {
        auto it = schemas_.find(table_name);
        if (it != schemas_.end()) {
            return it->second;
        }
        return nullptr;
    }

private:
    BufferPoolManager *bpm_;
    std::unordered_map<std::string, TableHeap*> tables_;
    std::unordered_map<std::string, Schema*> schemas_;
};

}
