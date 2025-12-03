#include "storage/index/b_plus_tree.h"

#include <cassert>
#include <cstring>
#include <algorithm>

namespace dbengine {
    BPlusTree::BPlusTree(BufferPoolManager *bpm, uint32_t max_size) : bpm_(bpm), max_size_(max_size) {
        
        Page *root_page = bpm_->NewPage(&root_page_id_);

        BPlusTreeLeafPage root_leaf_page(root_page->GetData(), max_size_);

        root_leaf_page.SetPageType(LEAF_PAGE);
        root_leaf_page.SetSize(0);
        root_leaf_page.SetPageId(root_page_id_);
        root_leaf_page.SetParentPageId(INVALID_PAGE_ID);

        bpm_->UnpinPage(root_page_id_, true);   

    }

    BPlusTreeLeafPage* BPlusTree::FindLeaf(int32_t key) {
        page_id_t current_page_id = root_page_id_;

        while (true) {
            Page *page = bpm_->FetchPage(current_page_id);
            if (page == nullptr) {
                return nullptr;
            }

            BPlusTreePage *bplus_page = reinterpret_cast<BPlusTreePage *>(page->GetData());

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
        BPlusTreeLeafPage* leaf = FindLeaf(key);
        if (leaf == nullptr) {
            return false;
        }

        uint32_t size = leaf->GetSize();

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

            BPlusTreeLeafPage new_leaf_page(new_page->GetData(), max_size_);

            uint32_t total_size = leaf_page->GetSize();
            uint32_t mid_index = total_size / 2;

            new_leaf_page.SetPageType(LEAF_PAGE);
            new_leaf_page.SetSize(total_size - mid_index);
            new_leaf_page.SetPageId(new_page_id);
            new_leaf_page.SetParentPageId(leaf_page->GetParentPageId());
            new_leaf_page.SetNextPageId(leaf_page->GetNextPageId());
            
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

        BPlusTreeInternalPage root_internal_page(root_page->GetData(), max_size_);

        root_internal_page.SetPageType(INTERNAL_PAGE);
        root_internal_page.SetSize(1); // One key for two children
        root_internal_page.SetPageId(root_page_id_);
        root_internal_page.SetParentPageId(INVALID_PAGE_ID);
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

        // Try to merge with left sibling first (if exists)
        if (child_index > 0) {
            page_id_t left_sibling_id = parent->GetChildPageId(child_index - 1);

            uint32_t separator_key_index = child_index - 1;

            bpm_->UnpinPage(leaf_page_id, false);
            bpm_->UnpinPage(parent_page_id, false);

            return MergeLeafNodes(left_sibling_id, leaf_page_id, parent_page_id, separator_key_index);
        } else {
            // We're the leftmost child, merge with right sibling
            page_id_t right_sibling_id = parent->GetChildPageId(child_index + 1);

            // Key index that separates current node and right sibling
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

        // Copy all keys and RIDs from right to left
        uint32_t left_size = left_leaf->GetSize();
        uint32_t right_size = right_leaf->GetSize();

        for (uint32_t i = 0; i < right_size; ++i) {
            left_leaf->SetKeyAt(left_size + i, right_leaf->GetKeyAt(i));
            left_leaf->SetRID(left_size + i, right_leaf->GetRID(i));
        }

        left_leaf->SetSize(left_size + right_size);

        // Update the next pointer to skip over the right node
        left_leaf->SetNextPageId(right_leaf->GetNextPageId());

        // Unpin and delete the right node
        bpm_->UnpinPage(left_page_id, true);
        bpm_->UnpinPage(right_page_id, false);
        bpm_->DeletePage(right_page_id);

        // Remove the separator key from parent
        return DeleteFromParent(parent_page_id, key_index);
    }

    bool BPlusTree::DeleteFromParent(page_id_t parent_page_id, uint32_t key_index) {
        // Fetch parent
        Page *parent_page = bpm_->FetchPage(parent_page_id);
        if (parent_page == nullptr) {
            return false;
        }

        BPlusTreeInternalPage *parent = reinterpret_cast<BPlusTreeInternalPage *>(parent_page->GetData());

        // Remove the key and shift everything left
        for (uint32_t i = key_index; i < parent->GetSize() - 1; ++i) {
            parent->SetKeyAt(i, parent->GetKeyAt(i + 1));
        }

        // Remove the child pointer (the right child of the deleted key)
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

        // Fetch parent
        Page *parent_page = bpm_->FetchPage(parent_page_id);
        if (parent_page == nullptr) {
            bpm_->UnpinPage(internal_page_id, false);
            return false;
        }   

        BPlusTreeInternalPage *parent = reinterpret_cast<BPlusTreeInternalPage *>(parent_page->GetData());

        // Find which child index we are
        uint32_t child_index = 0;
        for (uint32_t i = 0; i <= parent->GetSize(); ++i) {
            if (parent->GetChildPageId(i) == internal_page_id) {
                child_index = i;
                break;
            }
        }

        // Try to merge with left sibling first (if exists)
        if (child_index > 0) {
            page_id_t left_sibling_id = parent->GetChildPageId(child_index - 1);

            uint32_t separator_key_index = child_index - 1;

            bpm_->UnpinPage(internal_page_id, false);
            bpm_->UnpinPage(parent_page_id, false);

            return MergeInternalNodes(left_sibling_id, internal_page_id, parent_page_id, separator_key_index);
        } else {
            // We're the leftmost child, merge with right sibling
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
        

        // Move the separator key from parent to left internal node
        Page *parent_page = bpm_->FetchPage(parent_page_id);
        if (parent_page == nullptr) {
            bpm_->UnpinPage(left_page_id, false);
            bpm_->UnpinPage(right_page_id, false);
            return false;
        }

        BPlusTreeInternalPage *parent = reinterpret_cast<BPlusTreeInternalPage *>(parent_page->GetData());
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

        left_internal->SetSize(left_size + 1 + right_size);

        // Unpin and delete the right node
        bpm_->UnpinPage(left_page_id, true);
        bpm_->UnpinPage(right_page_id, false);
        bpm_->DeletePage(right_page_id);

        // Remove the separator key from parent
        return DeleteFromParent(parent_page_id, key_index); 

    void BPlusTree::AdjustRoot() {
        // This method would handle root adjustments
        // Currently handled inline in DeleteFromParent
    }

    bool ScanKey(int32_t low_key, int32_t high_key) {

    }


}