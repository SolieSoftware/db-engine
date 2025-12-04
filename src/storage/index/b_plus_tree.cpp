#include "storage/index/b_plus_tree.h"

#include <cassert>
#include <cstring>
#include <algorithm>

namespace dbengine {
    BPlusTree::BPlusTree(BufferPoolManager *bpm, uint32_t max_size) : bpm_(bpm), max_size_(max_size) {

        Page *root_page = bpm_->NewPage(&root_page_id_);

        if (root_page == nullptr) {
            throw std::runtime_error("Failed to allocate root page for B+ tree");
        }

        // Initialize the page data to zero
        std::memset(root_page->GetData(), 0, PAGE_SIZE);

        BPlusTreeLeafPage root_leaf_page(root_page->GetData(), max_size_);

        // Initialize the header - CRITICAL: must set max_size in header!
        BPlusTreePageHeader *header = reinterpret_cast<BPlusTreePageHeader *>(root_page->GetData());
        header->max_size = max_size_;
        header->page_type = LEAF_PAGE;
        header->size = 0;
        header->page_id = root_page_id_;
        header->parent_page_id = INVALID_PAGE_ID;

        bpm_->UnpinPage(root_page_id_, true);

    }

    BPlusTreeLeafPage* BPlusTree::FindLeaf(int32_t key) {
        page_id_t current_page_id = root_page_id_;
        std::cout << "[FindLeaf] Starting search, root_page_id=" << root_page_id_ << std::endl;

        while (true) {
            std::cout << "[FindLeaf] Fetching page_id=" << current_page_id << std::endl;
            Page *page = bpm_->FetchPage(current_page_id);
            if (page == nullptr) {
                std::cout << "[FindLeaf] FetchPage returned nullptr!" << std::endl;
                return nullptr;
            }

            std::cout << "[FindLeaf] Got page, getting data..." << std::endl;
            BPlusTreePage *bplus_page = reinterpret_cast<BPlusTreePage *>(page->GetData());
            std::cout << "[FindLeaf] About to check page type..." << std::endl;

            if (bplus_page->GetPageType() == LEAF_PAGE) {
                return reinterpret_cast<BPlusTreeLeafPage*>(page->GetData());
            }

            BPlusTreeInternalPage *internal_page = reinterpret_cast<BPlusTreeInternalPage *>(bplus_page);
            uint32_t child_index = internal_page->ValueIndex(key);
            current_page_id = internal_page->GetChildPageId(child_index);
            bpm_->UnpinPage(page->GetPageId(), false);
        }
    }


    bool BPlusTree::Search(int32_t key, RID &rid) {
        BPlusTreeLeafPage* leaf = FindLeaf(key);
        if (leaf == nullptr) {
            return false;
        }

        int32_t *found = std::lower_bound(
            leaf->GetKeys(),
            leaf->GetKeys() + leaf->GetSize(),
            key
        );

        if (found != leaf->GetKeys() + leaf->GetSize() && *found == key) {
            uint32_t index = found - leaf->GetKeys();
            rid = leaf->GetRID(index);
            bpm_->UnpinPage(leaf->GetPageId(), false);
            return true;
        }

        bpm_->UnpinPage(leaf->GetPageId(), false);
        return false;
    }

    bool BPlusTree::Insert(int32_t key, const RID &rid) {
        std::cout << "[Insert] Starting insert for key=" << key << std::endl;
        BPlusTreeLeafPage* leaf = FindLeaf(key);
        std::cout << "[Insert] FindLeaf returned: " << (leaf ? "valid" : "null") << std::endl;
        if (leaf == nullptr) {
            return false;
        }

        std::cout << "[Insert] About to call GetSize()..." << std::endl;
        uint32_t size = leaf->GetSize();
        std::cout << "[Insert] Leaf size=" << size << std::endl;

        if (size >= max_size_) {
            Split(leaf->GetPageId());
            bpm_->UnpinPage(leaf->GetPageId(), false);
            return Insert(key, rid);
        }

        int32_t *insert_pos = std::lower_bound(
            leaf->GetKeys(),
            leaf->GetKeys() + size,
            key
        );

        uint32_t index = insert_pos - leaf->GetKeys();

        for (uint32_t i = size; i > index; --i) {
            leaf->SetKeyAt(i, leaf->GetKeyAt(i - 1));
            leaf->SetRID(i, leaf->GetRID(i - 1));
        }

        leaf->SetKeyAt(index, key);
        leaf->SetRID(index, rid);
        leaf->SetSize(size + 1);

        bpm_->UnpinPage(leaf->GetPageId(), true);
        return true;
    }

    bool BPlusTree::Split(page_id_t page_id) {
        Page *page = bpm_->FetchPage(page_id);
        if (page == nullptr) {
            return false;
        }

        BPlusTreePage *bplus_page = reinterpret_cast<BPlusTreePage *>(page->GetData());

        if (bplus_page->GetPageType() == LEAF_PAGE) {
            BPlusTreeLeafPage *leaf_page = reinterpret_cast<BPlusTreeLeafPage *>(bplus_page);

            page_id_t new_page_id;
            Page *new_page = bpm_->NewPage(&new_page_id);
            if (new_page == nullptr) {
                bpm_->UnpinPage(page_id, false);
                return false;
            }

            // Initialize new page data
            std::memset(new_page->GetData(), 0, PAGE_SIZE);

            BPlusTreeLeafPage new_leaf_page(new_page->GetData(), max_size_);

            uint32_t total_size = leaf_page->GetSize();
            uint32_t mid_index = total_size / 2;

            // Initialize header for new leaf page
            BPlusTreeLeafPageHeader *new_header = reinterpret_cast<BPlusTreeLeafPageHeader *>(new_page->GetData());
            new_header->max_size = max_size_;
            new_header->page_type = LEAF_PAGE;
            new_header->size = total_size - mid_index;
            new_header->page_id = new_page_id;
            new_header->parent_page_id = leaf_page->GetParentPageId();
            new_header->next_page_id = leaf_page->GetNextPageId();
            
            uint32_t new_size = 0;
            for (uint32_t i = mid_index; i < total_size; ++i) {
                new_leaf_page.SetKeyAt(i - mid_index, leaf_page->GetKeyAt(i));
                new_leaf_page.SetRID(i - mid_index, leaf_page->GetRID(i));
                new_size++;
            }

            leaf_page->SetSize(mid_index);
            leaf_page->SetNextPageId(new_page_id);

            new_leaf_page.SetSize(new_size);
            new_leaf_page.SetNextPageId(leaf_page->GetNextPageId());

            bool result;

            if (leaf_page->GetParentPageId() == INVALID_PAGE_ID) {
                result = CreateNewRoot(leaf_page->GetPageId(), new_page_id, new_leaf_page.GetKeyAt(0));
            } else {
                result = InsertIntoParent(leaf_page->GetPageId(), new_page_id, new_leaf_page.GetKeyAt(0));
            }

            if (!result) { 
                bpm_->UnpinPage(page_id, true);
                return false;
            }

            bpm_->UnpinPage(page_id, true);
            bpm_->UnpinPage(new_page_id, true);

            return true;
        } else {
            // Internal page split
            BPlusTreeInternalPage *internal_page = reinterpret_cast<BPlusTreeInternalPage *>(bplus_page);

            page_id_t new_page_id;
            Page *new_page = bpm_->NewPage(&new_page_id);
            if (new_page == nullptr) {
                bpm_->UnpinPage(page_id, false);
                return false;
            }

            // Initialize new page data
            std::memset(new_page->GetData(), 0, PAGE_SIZE);

            BPlusTreeInternalPage new_internal_page(new_page->GetData(), max_size_);

            uint32_t total_size = internal_page->GetSize();
            uint32_t mid_index = total_size / 2;

            // The key at mid_index will be promoted to parent
            int32_t separator_key = internal_page->GetKeyAt(mid_index);

            // Initialize header for new internal page
            BPlusTreePageHeader *new_header = reinterpret_cast<BPlusTreePageHeader *>(new_page->GetData());
            new_header->max_size = max_size_;
            new_header->page_type = INTERNAL_PAGE;
            new_header->page_id = new_page_id;
            new_header->parent_page_id = internal_page->GetParentPageId();

            // Move keys [mid+1...total_size) to new page
            // Move children [mid+1...total_size] to new page
            uint32_t new_size = 0;
            for (uint32_t i = mid_index + 1; i < total_size; ++i) {
                new_internal_page.SetKeyAt(new_size, internal_page->GetKeyAt(i));
                new_internal_page.SetChildPageId(new_size, internal_page->GetChildPageId(i));
                new_size++;
            }
            // Don't forget the last child pointer
            new_internal_page.SetChildPageId(new_size, internal_page->GetChildPageId(total_size));
            new_internal_page.SetSize(new_size);

            // Update left page size (keys [0...mid-1])
            internal_page->SetSize(mid_index);

            // Update parent pointers for children that moved to new page
            for (uint32_t i = 0; i <= new_size; ++i) {
                page_id_t child_id = new_internal_page.GetChildPageId(i);
                Page *child_page = bpm_->FetchPage(child_id);
                if (child_page != nullptr) {
                    BPlusTreePage *child = reinterpret_cast<BPlusTreePage *>(child_page->GetData());
                    child->SetParentPageId(new_page_id);
                    bpm_->UnpinPage(child_id, true);
                }
            }

            bool result;
            if (internal_page->GetParentPageId() == INVALID_PAGE_ID) {
                result = CreateNewRoot(page_id, new_page_id, separator_key);
            } else {
                result = InsertIntoParent(page_id, new_page_id, separator_key);
            }

            if (!result) {
                bpm_->UnpinPage(page_id, true);
                bpm_->UnpinPage(new_page_id, true);
                return false;
            }

            bpm_->UnpinPage(page_id, true);
            bpm_->UnpinPage(new_page_id, true);

            return true;
        }
    }

    bool BPlusTree::CreateNewRoot(page_id_t left_page_id, page_id_t right_page_id, int32_t key) {
        Page *root_page = bpm_->NewPage(&root_page_id_);
        if (root_page == nullptr) {
            return false;
        }

        // Initialize new root page data
        std::memset(root_page->GetData(), 0, PAGE_SIZE);

        BPlusTreeInternalPage root_internal_page(root_page->GetData(), max_size_);

        // Initialize header for new root
        BPlusTreePageHeader *root_header = reinterpret_cast<BPlusTreePageHeader *>(root_page->GetData());
        root_header->max_size = max_size_;
        root_header->page_type = INTERNAL_PAGE;
        root_header->size = 1; // One key for two children
        root_header->page_id = root_page_id_;
        root_header->parent_page_id = INVALID_PAGE_ID;

        root_internal_page.SetKeyAt(0, key);
        root_internal_page.SetChildPageId(0, left_page_id);
        root_internal_page.SetChildPageId(1, right_page_id);

        Page *left_page = bpm_->FetchPage(left_page_id);
        if (left_page == nullptr) {
            bpm_->UnpinPage(root_page_id_, false);
            return false;
        }   

        BPlusTreePage *left_bplus_page = reinterpret_cast<BPlusTreePage *>(left_page->GetData());
        left_bplus_page->SetParentPageId(root_page_id_);

        Page *right_page = bpm_->FetchPage(right_page_id);
        if (right_page == nullptr) {
            bpm_->UnpinPage(left_page_id, false);
            bpm_->UnpinPage(root_page_id_, false);
            return false;
        }

        BPlusTreePage *right_bplus_page = reinterpret_cast<BPlusTreePage *>(right_page->GetData());
        right_bplus_page->SetParentPageId(root_page_id_);

        bpm_->UnpinPage(left_page_id, true);
        bpm_->UnpinPage(right_page_id, true);
        bpm_->UnpinPage(root_page_id_, true);

        return true;
    }

    bool BPlusTree::InsertIntoParent(page_id_t left_page_id, page_id_t right_page_id, int32_t key) {
        Page *left_page = bpm_->FetchPage(left_page_id);
        if (left_page == nullptr) {
            return false;
        }
        BPlusTreePage *left_bplus_page = reinterpret_cast<BPlusTreePage *>(left_page->GetData());

        page_id_t parent_page_id = left_bplus_page->GetParentPageId();

        Page *parent_page = bpm_->FetchPage(parent_page_id);
        if (parent_page == nullptr) {
            bpm_->UnpinPage(left_page_id, false);
            return false;
        }

        BPlusTreeInternalPage *parent_internal_page = reinterpret_cast<BPlusTreeInternalPage *>(parent_page->GetData());

        uint32_t size = parent_internal_page->GetSize();
        if (size >= max_size_) {
            Split(parent_page_id);
            bpm_->UnpinPage(left_page_id, false);
            bpm_->UnpinPage(parent_page_id, false);
            return InsertIntoParent(left_page_id, right_page_id, key);
        }

        int32_t *insert_pos = std::lower_bound(
            parent_internal_page->GetKeys(),
            parent_internal_page->GetKeys() + size,
            key
        );

        uint32_t index = insert_pos - parent_internal_page->GetKeys();

        for (uint32_t i = size; i > index; --i) {
            parent_internal_page->SetKeyAt(i, parent_internal_page->GetKeyAt(i - 1));
            parent_internal_page->SetChildPageId(i + 1, parent_internal_page->GetChildPageId(i));
        }

        parent_internal_page->SetKeyAt(index, key);
        parent_internal_page->SetChildPageId(index + 1, right_page_id);
        parent_internal_page->SetSize(size + 1);
        bpm_->UnpinPage(left_page_id, true);
        bpm_->UnpinPage(parent_page_id, true);

        return true;
    }

    bool BPlusTree::Delete(int32_t key) {
        // Simple deletion without rebalancing and not addressing underflow issue
        BPlusTreeLeafPage* leaf = FindLeaf(key);
        if (leaf == nullptr) {
            return false;
        }

        int32_t *found = std::lower_bound(
            leaf->GetKeys(),
            leaf->GetKeys() + leaf->GetSize(),
            key
        );

        if (found == leaf->GetKeys() + leaf->GetSize() || *found != key) {
            bpm_->UnpinPage(leaf->GetPageId(), false);
            return false;
        }

        uint32_t index = found - leaf->GetKeys();
        for (uint32_t i = index; i < leaf->GetSize() - 1; ++i) {
            leaf->SetKeyAt(i, leaf->GetKeyAt(i + 1));
            leaf->SetRID(i, leaf->GetRID(i + 1));
        }

        leaf->SetSize(leaf->GetSize() - 1);

        // Underflow check
        if (leaf->GetSize() >= MIN_KEY_SIZE || leaf->GetParentPageId() == INVALID_PAGE_ID) {
            bpm_->UnpinPage(leaf->GetPageId(), true);
            return true;
        }

        // Handle underflow
        page_id_t leaf_page_id = leaf->GetPageId();
        bpm_->UnpinPage(leaf_page_id, true);

        return HandleLeafUnderflow(leaf_page_id);
    }

    bool BPlusTree::HandleLeafUnderflow(page_id_t leaf_page_id) {
        // Fetch the underflowed leaf
        Page *leaf_page = bpm_->FetchPage(leaf_page_id);
        if (leaf_page == nullptr) {
            return false;
        }

        BPlusTreeLeafPage *leaf = reinterpret_cast<BPlusTreeLeafPage *>(leaf_page->GetData());
        page_id_t parent_page_id = leaf->GetParentPageId();

        // Fetch parent
        Page *parent_page = bpm_->FetchPage(parent_page_id);
        if (parent_page == nullptr) {
            bpm_->UnpinPage(leaf_page_id, false);
            return false;
        }

        BPlusTreeInternalPage *parent = reinterpret_cast<BPlusTreeInternalPage *>(parent_page->GetData());

        // Find which child index we are in the parent
        uint32_t child_index = 0;
        for (uint32_t i = 0; i <= parent->GetSize(); ++i) {
            if (parent->GetChildPageId(i) == leaf_page_id) {
                child_index = i;
                break;
            }
        }


        if (child_index > 0) {
            page_id_t left_sibling_id = parent->GetChildPageId(child_index - 1);

            uint32_t separator_key_index = child_index - 1;

            bpm_->UnpinPage(leaf_page_id, false);
            bpm_->UnpinPage(parent_page_id, false);

            return MergeLeafNodes(left_sibling_id, leaf_page_id, parent_page_id, separator_key_index);
        } else {

            page_id_t right_sibling_id = parent->GetChildPageId(child_index + 1);

            uint32_t separator_key_index = child_index;

            bpm_->UnpinPage(leaf_page_id, false);
            bpm_->UnpinPage(parent_page_id, false);

            return MergeLeafNodes(leaf_page_id, right_sibling_id, parent_page_id, separator_key_index);
        }
    }

    bool BPlusTree::MergeLeafNodes(page_id_t left_page_id, page_id_t right_page_id,
                                    page_id_t parent_page_id, uint32_t key_index) {

        Page *left_page = bpm_->FetchPage(left_page_id);
        Page *right_page = bpm_->FetchPage(right_page_id);

        if (left_page == nullptr || right_page == nullptr) {
            if (left_page != nullptr) bpm_->UnpinPage(left_page_id, false);
            if (right_page != nullptr) bpm_->UnpinPage(right_page_id, false);
            return false;
        }

        BPlusTreeLeafPage *left_leaf = reinterpret_cast<BPlusTreeLeafPage *>(left_page->GetData());
        BPlusTreeLeafPage *right_leaf = reinterpret_cast<BPlusTreeLeafPage *>(right_page->GetData());

        uint32_t left_size = left_leaf->GetSize();
        uint32_t right_size = right_leaf->GetSize();

        for (uint32_t i = 0; i < right_size; ++i) {
            left_leaf->SetKeyAt(left_size + i, right_leaf->GetKeyAt(i));
            left_leaf->SetRID(left_size + i, right_leaf->GetRID(i));
        }

        left_leaf->SetSize(left_size + right_size);


        left_leaf->SetNextPageId(right_leaf->GetNextPageId());

        bpm_->UnpinPage(left_page_id, true);
        bpm_->UnpinPage(right_page_id, false);
        bpm_->DeletePage(right_page_id);
        


        return DeleteFromParent(parent_page_id, key_index);
    }

    bool BPlusTree::DeleteFromParent(page_id_t parent_page_id, uint32_t key_index) {

        Page *parent_page = bpm_->FetchPage(parent_page_id);
        if (parent_page == nullptr) {
            return false;
        }

        BPlusTreeInternalPage *parent = reinterpret_cast<BPlusTreeInternalPage *>(parent_page->GetData());


        for (uint32_t i = key_index; i < parent->GetSize() - 1; ++i) {
            parent->SetKeyAt(i, parent->GetKeyAt(i + 1));
        }


        for (uint32_t i = key_index + 1; i < parent->GetSize(); ++i) {
            parent->SetChildPageId(i, parent->GetChildPageId(i + 1));
        }

        parent->SetSize(parent->GetSize() - 1);

        // Check if this is the root
        if (parent_page_id == root_page_id_) {
            // If root becomes empty (0 keys, 1 child), make that child the new root
            if (parent->GetSize() == 0) {
                page_id_t new_root_id = parent->GetChildPageId(0);

                // Update the new root's parent to INVALID
                Page *new_root_page = bpm_->FetchPage(new_root_id);
                if (new_root_page != nullptr) {
                    BPlusTreePage *new_root = reinterpret_cast<BPlusTreePage *>(new_root_page->GetData());
                    new_root->SetParentPageId(INVALID_PAGE_ID);
                    bpm_->UnpinPage(new_root_id, true);
                }

                // Delete old root and update root_page_id_
                bpm_->UnpinPage(parent_page_id, false);
                bpm_->DeletePage(parent_page_id);
                root_page_id_ = new_root_id;

                return true;
            }

            bpm_->UnpinPage(parent_page_id, true);
            return true;
        }

        if (parent->GetSize() < MIN_KEY_SIZE) {
            // Parent underflows - need to handle it recursively
            bpm_->UnpinPage(parent_page_id, true);
            return HandleInternalUnderflow(parent_page_id);
        }

        bpm_->UnpinPage(parent_page_id, true);
        return true;
    }

    bool BPlusTree::HandleInternalUnderflow(page_id_t internal_page_id) {

        Page *internal_page = bpm_->FetchPage(internal_page_id);
        if (internal_page == nullptr) {
            return false;
        }

        BPlusTreeInternalPage *internal = reinterpret_cast<BPlusTreeInternalPage *>(internal_page->GetData());
        page_id_t parent_page_id = internal->GetParentPageId();

        Page *parent_page = bpm_->FetchPage(parent_page_id);
        if (parent_page == nullptr) {
            bpm_->UnpinPage(internal_page_id, false);
            return false;
        }   

        BPlusTreeInternalPage *parent = reinterpret_cast<BPlusTreeInternalPage *>(parent_page->GetData());

        uint32_t child_index = 0;
        for (uint32_t i = 0; i <= parent->GetSize(); ++i) {
            if (parent->GetChildPageId(i) == internal_page_id) {
                child_index = i;
                break;
            }
        }

        if (child_index > 0) {
            page_id_t left_sibling_id = parent->GetChildPageId(child_index - 1);

            uint32_t separator_key_index = child_index - 1;

            bpm_->UnpinPage(internal_page_id, false);
            bpm_->UnpinPage(parent_page_id, false);

            return MergeInternalNodes(left_sibling_id, internal_page_id, parent_page_id, separator_key_index);
        } else {
            page_id_t right_sibling_id = parent->GetChildPageId(child_index + 1);

            // Key index that separates current node and right sibling
            uint32_t separator_key_index = child_index;

            bpm_->UnpinPage(internal_page_id, false);
            bpm_->UnpinPage(parent_page_id, false);

            return MergeInternalNodes(internal_page_id, right_sibling_id, parent_page_id, separator_key_index);
        }
    }

    bool BPlusTree::MergeInternalNodes(page_id_t left_page_id, page_id_t right_page_id,
                                       page_id_t parent_page_id, uint32_t key_index) {
        Page *left_page = bpm_->FetchPage(left_page_id);
        Page *right_page = bpm_->FetchPage(right_page_id);
        Page *parent_page = bpm_->FetchPage(parent_page_id);

        if (left_page == nullptr || right_page == nullptr) {
            if (left_page != nullptr) bpm_->UnpinPage(left_page_id, false);
            if (right_page != nullptr) bpm_->UnpinPage(right_page_id, false);
            if (parent_page != nullptr) bpm_->UnpinPage(parent_page_id, false);
            return false;
        }

        BPlusTreeInternalPage *left_internal = reinterpret_cast<BPlusTreeInternalPage *>(left_page->GetData());
        BPlusTreeInternalPage *right_internal = reinterpret_cast<BPlusTreeInternalPage *>(right_page->GetData());
        BPlusTreeInternalPage *parent = reinterpret_cast<BPlusTreeInternalPage *>(parent_page->GetData());

        // Move the separator key from parent to left internal node
        int32_t separator_key = parent->GetKeyAt(key_index);

        uint32_t left_size = left_internal->GetSize();
        left_internal->SetKeyAt(left_size, separator_key);
        left_internal->SetSize(left_size + 1);

        // Copy all keys and child pointers from right to left
        uint32_t right_size = right_internal->GetSize();
        for (uint32_t i = 0; i < right_size; ++i) {
            left_internal->SetKeyAt(left_size + 1 + i, right_internal->GetKeyAt(i));
            left_internal->SetChildPageId(left_size + 1 + i, right_internal->GetChildPageId(i));
        }
        left_internal->SetChildPageId(left_size + 1 + right_size, right_internal->GetChildPageId(right_size));

        // This is what makes internal node merge different from leaf merge
        for (uint32_t i = 0; i <= right_size; ++i) {
            page_id_t child_id = right_internal->GetChildPageId(i);
            Page *child_page = bpm_->FetchPage(child_id);
            if (child_page != nullptr) {
                BPlusTreePage *child = reinterpret_cast<BPlusTreePage *>(child_page->GetData());
                child->SetParentPageId(left_page_id);
                bpm_->UnpinPage(child_id, true);
            }
        }

        left_internal->SetSize(left_size + 1 + right_size);

        // Unpin and delete the right node
        bpm_->UnpinPage(left_page_id, true);
        bpm_->UnpinPage(right_page_id, false);
        bpm_->UnpinPage(parent_page_id, false);
        bpm_->DeletePage(right_page_id);

        // Remove the separator key from parent (may cause recursive underflow)
        return DeleteFromParent(parent_page_id, key_index);
    }

}