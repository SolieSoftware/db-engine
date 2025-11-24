#pragma once

#include "common/config.h"
#include "common/rid.h"
#include "storage/index/b_plus_tree_page.h"

#include <cstdint>
#include <algorithm>

namespace dbengine {

    class BPlusTreeInternalPage : public BPlusTreePage {
        // Add your public and private members here
        public:
        BPlusTreeInternalPage(char *data, uint32_t max_size) : BPlusTreePage(data, max_size) {
            child_page_ids_ = reinterpret_cast<page_id_t *>(data_ + sizeof(BPlusTreePageHeader) + max_size * sizeof(int32_t));
        };

        // Child page IDs arrays - Store child page IDs (one more than keys)
        inline page_id_t GetChildPageId(uint32_t index) const {
            return child_page_ids_[index];
        }

        inline void SetChildPageId(uint32_t index, page_id_t child_page_id) {
            child_page_ids_[index] = child_page_id;
        }

        // Given a key, find which child to follow
        uint32_t ValueIndex(int32_t key) const {

            int32_t *first_greater_equal = std::lower_bound(
                keys_, keys_ + GetSize(), key
            );

            return first_greater_equal - keys_;
        }

        private:
            page_id_t *child_page_ids_;

        };
    }