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
}