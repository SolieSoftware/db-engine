#pragma once
#include <cstring>
#include <string>
#include "common/rid.h"

namespace dbengine {

    class Tuple {
        // Constructors
        Tuple() : data_(nullptr), size_(0) {}

        Tuple(const char *data, uint32_t size) : data_(data), size_(size) {}

        ~Tuple() { delete[] data_; }

        // Copy constructor and assignment (important!)
        Tuple(const Tuple &other) : size_(other.size_) {
            data_ = new char[size_];
            std::memcpy(data_, other.data_, size_);
        }

        Tuple& operator=(const Tuple &other) {
            if (this != &other) {
                delete[] data_;
                data_ = new char[other.size_];
                std::memcpy(data_, other.data_, other.size_);
            }
            return *this;
        }

        // Getters
        inline const char* GetData() const { return data_; }
        inline uint32_t GetSize() const { return size_; }

        // RID Management
        inline RID GetRID() const { return rid_; }
        inline void SetRID(const RID &rid) { rid_ = rid; }
    
    private:
        char *data_;
        uint32_t size_;
        RID rid_;
    };
}