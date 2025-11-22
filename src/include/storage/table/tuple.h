#pragma once
#include <cstring>
#include <string>
#include "common/rid.h"

namespace dbengine {

    class Tuple {
        // Constructors
    public:
        Tuple() : data_(nullptr), size_(0) {}; 

        Tuple(const char *data, uint32_t size) : size_(size) {
            data_ = new char[size_];
            std::memcpy(data_, data, size_);
        }; 

        ~Tuple() { delete[] data_; };

        // Copy constructor and assignment (important!)
        Tuple(const Tuple &other) : size_(other.size_) {
            data_ = new char[size_];
            std::memcpy(data_, other.data_, size_);
        };

        Tuple& operator=(const Tuple &other) {
            if (this != &other) {
                delete[] data_;
                data_ = new char[other.size_];
                std::memcpy(data_, other.data_, other.size_);
            }
            return *this;
        };

        // Getters
        inline const char* GetData() const { return data_; }
        inline char* GetData() { return data_; }
        inline uint32_t GetSize() const { return size_; }

        // RID Management
        inline RID GetRID() const { return rid_; }
        inline void SetRID(const RID &rid) { rid_ = rid; }

        void Allocate(uint32_t size) {
            delete[] data_;
            data_ = new char[size];
            size_ = size;
        }
    
    private:
        char *data_;
        uint32_t size_;
        RID rid_;
    };
}