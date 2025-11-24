#include "storage/index/b_plus_tree_page.h"

#include <cassert>
#include <cstring>

// Using uint32_t for parameter types follows principle of "making illegal states unrepresentable"

namespace dbengine {
    

    int32_t BPlusTreePage::GetKeyAt(uint32_t index) const {
        assert(index < GetMaxSize());
        return keys_[index];
    }

    void BPlusTreePage::SetKeyAt(uint32_t index, int32_t key) {
        assert(index < GetMaxSize());
        keys_[index] = key;
    }

    void BPlusTreePage::SetSize(uint32_t size) {
        assert(size <= GetMaxSize());
        GetHeader()->size = size;
    }

    void BPlusTreePage::SetPageId(page_id_t page_id) {
        GetHeader()->page_id = page_id;
    }

    void BPlusTreePage::SetParentPageId(page_id_t parent_page_id) {
        GetHeader()->parent_page_id = parent_page_id;
    }

    void BPlusTreePage::SetPageType(uint32_t page_type) {
        GetHeader()->page_type = page_type;
    }

}