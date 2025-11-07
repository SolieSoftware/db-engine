#pragma once

#include <cstdint>

namespace dbengine {
    /**
    * Record ID - Identifies a record by page_id and slot number
     */

    class RID {
        public:
        RID() : page_id_(-1), slot_num_(-1), generation_(0) {}

        RID(int32_t page_id, int32_t slot_num, uint32_t generation) : page_id_(page_id), slot_num_(slot_num), generation_(generation) {}

        int32_t GetPageId() const { return page_id_; }
        int32_t GetSlotNum() const { return slot_num_; }
        uint32_t GetGeneration() const { return generation_; }

        bool IsValid() const { return page_id_ >= 0 && slot_num_ >= 0; }


        private:
            int32_t page_id_;
            int32_t slot_num_;
            uint32_t generation_;
    };
}