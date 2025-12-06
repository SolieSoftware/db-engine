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
            // Use upper_bound to find the first key GREATER than the search key
            // This ensures that if key == separator, we go to the right child
            // Example: keys [30, 50], children [0, 1, 2]
            //   key=10 -> upper_bound points to 30 (index 0) -> child 0
            //   key=30 -> upper_bound points to 50 (index 1) -> child 1
            //   key=50 -> upper_bound points to end (index 2) -> child 2
            //   key=60 -> upper_bound points to end (index 2) -> child 2
            int32_t *first_greater = std::upper_bound(
                keys_, keys_ + GetSize(), key
            );

            return first_greater - keys_;
        }

        private:
            page_id_t *child_page_ids_;

        };
    }