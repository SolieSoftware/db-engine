#pragma once

#include <string>
#include <cstring>
#include "catalog/schema.h"

namespace dbengine {

class Value {
public:
    Value() : type_(TypeId::INVALID), int_val_(0) {}

    explicit Value(int32_t val) : type_(TypeId::INTEGER), int_val_(val) {}

    explicit Value(const std::string &val) : type_(TypeId::VARCHAR), str_val_(val) {}

    inline TypeId GetType() const { return type_; }
    inline int32_t GetAsInt() const { return int_val_; }
    inline std::string GetAsString() const { return str_val_; }

    bool operator==(const Value &other) const {
        if (type_ != other.type_) return false;
        if (type_ == TypeId::INTEGER) {
            return int_val_ == other.int_val_;
        } else if (type_ == TypeId::VARCHAR) {
            return str_val_ == other.str_val_;
        }
        return false;
    }

    bool operator!=(const Value &other) const {
        return !(*this == other);
    }

    bool operator<(const Value &other) const {
        if (type_ != other.type_) return false;
        if (type_ == TypeId::INTEGER) {
            return int_val_ < other.int_val_;
        } else if (type_ == TypeId::VARCHAR) {
            return str_val_ < other.str_val_;
        }
        return false;
    }

    bool operator>(const Value &other) const {
        if (type_ != other.type_) return false;
        if (type_ == TypeId::INTEGER) {
            return int_val_ > other.int_val_;
        } else if (type_ == TypeId::VARCHAR) {
            return str_val_ > other.str_val_;
        }
        return false;
    }

    void SerializeTo(char *storage) const {
        if (type_ == TypeId::INTEGER) {
            std::memcpy(storage, &int_val_, sizeof(int32_t));
        } else if (type_ == TypeId::VARCHAR) {
            std::memcpy(storage, str_val_.c_str(), str_val_.length());
        }
    }

    static Value DeserializeFrom(const char *storage, TypeId type, uint32_t length) {
        if (type == TypeId::INTEGER) {
            int32_t val;
            std::memcpy(&val, storage, sizeof(int32_t));
            return Value(val);
        } else if (type == TypeId::VARCHAR) {
            std::string str(storage, length);
            return Value(str);
        }
        return Value();
    }

private:
    TypeId type_;
    int32_t int_val_;
    std::string str_val_;
};

}
