#pragma once
#include "storage/page/page.h"
#include "storage/table/tuple.h"
#include "storage/buffer/buffer_pool_manager.h"

namespace dbengine {
    class TableHeap {
        public:
        // Constructor
        TableHeap(BufferPoolManager *bpm) : bpm_(bpm) {}

        // Insert a tuple, return RID where it was stored
        bool InsertTuple(const Tuple &tuple, RID &rid);

        // Get a tuple by RID
        bool GetTuple(const RID &rid, Tuple &tuple);

        // Delete a tuple by RID
        bool DeleteTuple(const RID &rid);

        // Update a tuple by RID
        bool UpdateTuple(const Tuple& new_tuple, const RID &rid);

        inline page_id_t GetFirstPageId() const { return first_page_id_; }

        private:
            BufferPoolManager *bpm_;
            page_id_t first_page_id_;
            page_id_t last_page_id_;
    }
}
