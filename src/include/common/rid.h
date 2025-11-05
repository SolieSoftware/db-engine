#pragma once

#include <cstdint>

namespace dbengine {
    /**
    * Record ID - Identifies a record by page_id and slot number
     */

    class RID {
        public:
        RID() : page_id_(-1), slot_num_(-1) {}

        RID(int32_t page_id, int32_t slot_num) : page_id_(page_id), slot_num_(slot_num) {}

        int32_t GetPageId() const { return page_id_; }
        int32_t GetSlotNum() const { return slot_num_; }

        bool IsValid() const { return page_id_ >= 0 && slot_num_ >= 0; }


        private:
            int32_t page_id_;
            int32_t slot_num_;
        
    };
}