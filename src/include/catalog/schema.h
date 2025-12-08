#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace dbengine {

enum class TypeId {
    INTEGER,
    VARCHAR,
    INVALID
};

class Column {
public:
    Column(const std::string &name, TypeId type, uint32_t length = 0)
        : name_(name), type_(type), length_(length) {}

    inline std::string GetName() const { return name_; }
    inline TypeId GetType() const { return type_; }
    inline uint32_t GetLength() const { return length_; }
    inline uint32_t GetFixedLength() const {
        if (type_ == TypeId::INTEGER) {
            return 4;
        }
        return length_;
    }

private:
    std::string name_;
    TypeId type_;
    uint32_t length_;
};

class Schema {
public:
    Schema(const std::vector<Column> &columns) : columns_(columns) {
        uint32_t offset = 0;
        for (const auto &col : columns_) {
            offset += col.GetFixedLength();
        }
        tuple_size_ = offset;
    }

    inline const std::vector<Column>& GetColumns() const { return columns_; }
    inline uint32_t GetColumnCount() const { return columns_.size(); }
    inline const Column& GetColumn(uint32_t idx) const { return columns_[idx]; }
    inline uint32_t GetTupleSize() const { return tuple_size_; }

    uint32_t GetColumnOffset(uint32_t col_idx) const {
        uint32_t offset = 0;
        for (uint32_t i = 0; i < col_idx; ++i) {
            offset += columns_[i].GetFixedLength();
        }
        return offset;
    }

private:
    std::vector<Column> columns_;
    uint32_t tuple_size_;
};

}
