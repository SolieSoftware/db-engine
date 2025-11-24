#include "storage/index/b_plus_tree_leaf_page.h"

namespace dbengine {
    void BPlusTreeLeafPage::SetRID(uint32_t index, const RID &rid) {
        rids_[index] = rid;
    }

    void BPlusTreeLeafPage::SetNextPageId(page_id_t next_page_id) {
        GetHeader()->next_page_id = next_page_id;
    }
}