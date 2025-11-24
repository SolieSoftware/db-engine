#pragma once

#include "common/config.h"
#include "common/rid.h"

#include "storage/index/b_plus_tree_leaf_page.h"
#include "storage/index/b_plus_tree_page_internal.h"
#include "storage/buffer/buffer_pool_manager.h"

#include <cstdint>

namespace dbengine {
    class BPlusTree {
        public:
        BPlusTree(BufferPoolManager *bpm, uint32_t max_size);

        bool Search(int32_t key, RID &rid);

        bool Insert(int32_t key, const RID &rid);

        bool Delete(int32_t key);

        private:
        BPlusTreeLeafPage* FindLeaf(int32_t key);
        bool Split(page_id_t page_id);

        bool CreateNewRoot(page_id_t left_page_id, page_id_t right_page_id, int32_t key);
        bool InsertIntoParent(page_id_t left_page_id, page_id_t right_page_id, int32_t key);


        BufferPoolManager *bpm_;
        page_id_t root_page_id_;
        int32_t max_size_;

    };
}