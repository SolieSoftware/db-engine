#pragma once
#include "common/config.h"
#include "common/rid.h"

#include <cstdint>


namespace dbengine {
    struct BPlusTreePageHeader {
        page_id_t parent_page_id;
        page_id_t page_id;
        uint32_t page_type;
        uint32_t size;
        uint32_t max_size;
    };

    class BPlusTreePage {
        // Add your public and private members here
        public:
        BPlusTreePage(char *data, uint32_t max_size) {
            data_ = data;
            keys_ = reinterpret_cast<int32_t *>(data_ + sizeof(BPlusTreePageHeader));
        };

        // Keys array getter
        int32_t* GetKeys() { return keys_; }
        const int32_t* GetKeys() const { return keys_; }

        // Key getters and setters
        int32_t GetKeyAt(uint32_t index) const;

        void SetKeyAt(uint32_t index, int32_t key);

        // Size getter and setters
        uint32_t GetSize() const { return GetHeader()->size; };

        void SetSize(uint32_t size);

        // Page ID getters and setters
        uint32_t GetPageId() const { return GetHeader()->page_id; }

        void SetPageId(page_id_t page_id);

        // Parent page ID getters and setters

        page_id_t GetParentPageId() const {
            return GetHeader()->parent_page_id;
        }

        void SetParentPageId(page_id_t parent_page_id);

        // Page type getters and setters

        uint32_t GetPageType() const {
            return GetHeader()->page_type;
        }

        void SetPageType(uint32_t page_type);

        // Max size getters

        uint32_t GetMaxSize() const {
            return GetHeader()->max_size;
        }

        protected:
            char *data_;
            int32_t *keys_;

        private:
           BPlusTreePageHeader *GetHeader() {
            return reinterpret_cast<BPlusTreePageHeader *>(data_);
           }

           const BPlusTreePageHeader *GetHeader() const {
            return reinterpret_cast<const BPlusTreePageHeader *>(data_);
           }
    };
}