#pragma once


#include "common/config.h"
#include "common/rid.h"

#include <cstdint>


namespace dbengine {
    struct BPlusTreeLeafPageHeader {
        page_id_t parent_page_id;
        page_id_t next_page_id;
        uint32_t page_type;
        uint32_t size;
        uint32_t max_size;
    };

    class BPlusTreeLeafPage {
        // Add your public and private members here
        public:
        void BPlusTreePage(page_idt_t page_id, page_id_t parent_id = INVALID_PAGE_ID, uint32_t page_type, uint32_t size, int32_t max_size = LEAF_PAGE_SIZE);

        uint32_t GetSize() const {
            return header_.size;
        }

        uint32_t GetMaxSize() const {
            return header_.max_size;
        }

        // Key getters and setters

        uint32_t GetKeyAt(int32_t index) const;

        void SetKeyAt(int32_t index, int32_t key);

        // Size getter and setters
        int32_t GetSize() { return header_.size; };

        void SetSize(int32_t size);

        // Page ID getters and setters

        int32_t GetPageId() const {
            return page_id_;
        }

        void SetPageId(page_id_t page_id);

        uint32_t GetPageType() const {
            return header_.page_type;
        }

        private:
        BPlusTreeLeafPageHeader &GetHeader() {
            return header_;
        }

        BPlusTreeLeafPageHeader header_;



    };

}