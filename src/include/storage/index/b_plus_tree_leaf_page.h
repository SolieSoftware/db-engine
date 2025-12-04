#pragma once


#include "common/config.h"
#include "common/rid.h"
#include "storage/index/b_plus_tree_page.h"

#include <cstdint>


namespace dbengine {
    struct BPlusTreeLeafPageHeader : BPlusTreePageHeader {
        page_id_t next_page_id;
    };

    class BPlusTreeLeafPage : public BPlusTreePage {
        // Add your public and private members here
        public:
        BPlusTreeLeafPage(char *data, uint32_t max_size) : BPlusTreePage(data, max_size) {
            rids_ = reinterpret_cast<RID *>(data_ + sizeof(BPlusTreeLeafPageHeader) + max_size * sizeof(int32_t));
        };

        // RIDs arrays - Store RIDs (one per key)
        inline RID GetRID(uint32_t index) const { return rids_[index]; }

        void SetRID(uint32_t index, const RID &rid);

        // Next page ID getters and setters
        page_id_t GetNextPageId() { return GetHeader()->next_page_id; }

        void SetNextPageId(page_id_t next_page_id);
        
        private:
            BPlusTreeLeafPageHeader* GetHeader() {
                return reinterpret_cast<BPlusTreeLeafPageHeader *>(data_);
            };

            BPlusTreeLeafPageHeader const* GetHeader() const {
                return reinterpret_cast<BPlusTreeLeafPageHeader const*>(data_);
            };

            RID *rids_;



    };

}