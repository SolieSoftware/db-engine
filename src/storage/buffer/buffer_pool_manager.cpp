#include "storage/buffer/buffer_pool_manager.h"

namespace dbengine {

BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager *disk_manager) : pool_size_(pool_size), disk_manager_(disk_manager) {

    //Allocate the buffer pool (array of Pages)
    pages_ = new Page[pool_size_];

    // Create the LRU replacer (can track all frames)
    replacer_ = new LRUReplacer(pool_size_);

    // Initialize the pin counts and dirty flags fro each frame
    pin_count_.resize(pool_size_, 0);
    is_dirty_.resize(pool_size_, false);

    // All frames start as free (no pages loaded)
    for (size_t i = 0; i < pool_size_; i++) {
        free_list_.push_back(static_cast<frame_id_t>(i));
    }

    // page_table_ starts empty (no mappings yet)
}

BufferPoolManager::~BufferPoolManager() {
    // Flush all dirty pages before destruction
    FlushAllPages();
    // Free the buffer pool array
    delete[] pages_;
    // Free the LRU replacer
    delete replacer_;

    // Note: We don't delete disk_manager_ because we don't own it
    // (It was passed in by the caller)
}

void BufferPoolManager::FlushAllPages() {
    // Incrementing through all frames
    for (size_t i = 0; i < pool_size_; i++) {

        // If page is not dirty, skip it
        if (!is_dirty_[i]) {
            continue;
        }

        // Get the page from the buffer pool
        Page *page = &pages_[i];

        // If page is not valid, skip it
        if (page->GetPageId() == INVALID_PAGE_ID) {
            continue;
        }

        FlushPage(page->GetPageId());
    }
    }

bool BufferPoolManager::FindVictimFrame(frame_id_t *frame_id) {
    // Step 1: Check if there's a free frame (easiest case)
    if (!free_list_.empty()) {
        *frame_id = free_list_.front();
        free_list_.pop_front();
        return true;
    }

    // Step 2: No free frames, try to evict a frame using the replacer
    if (!replacer_->Victim(frame_id)) {
        return false; // All frames are pinned, can't evict.
    }
    
    // Step 3: We have a victim frame, need to evict it
    // Get the page currently in this frame
    Page *victim_page = &pages_[*frame_id];
    page_id_t victim_page_id = victim_page->GetPageId();

    // If victim page is dirty flush it to disk
    if (is_dirty_[*frame_id]) {
        disk_manager_->WritePage(victim_page_id, victim_page->GetData());
        is_dirty_[*frame_id] = false;
    }

    // Remove victim page from the page table
    page_table_.erase(victim_page_id);

    return true;
}

Page *BufferPoolManager::FetchPage(page_id_t page_id) {
    // Step 1: Check if page is already in buffer pool
    if (page_table_.find(page_id) != page_table_.end()) {
        // Page hit! Get the frame it's in 
        frame_id_t frame_id = page_table_[page_id];
        Page *page = &pages_[frame_id];

        // Increment pin count and return the page
        pin_count_[frame_id]++;

        // If this is the first pin, remove from replacer
        // (Can't evict a pinned page)
        if (pin_count_[frame_id] == 1) {
            replacer_->Pin(frame_id);
        }

        return page;
    }

    // Step 2: Page not in buffer - need to load it.
    // Find a victim from (iehter free or evict)
    frame_id_t frame_id;
    if (!FindVictimFrame(&frame_id)) {
        return nullptr; // No frames available, can't load page.
    }

    // Step 3: Load page from disk into the frame
    Page *page = &pages_[frame_id];
    disk_manager_->ReadPage(page_id, page->GetData());
    
    // Step 4: Update buffer pool metadata
    page_table_[page_id] = frame_id; // Map page to frame
    pin_count_[frame_id] = 1; // Pin it
    is_dirty_[frame_id] = false; // Not dirty yet

    // Frame is pinned, so remove from replacer
    replacer_->Pin(frame_id);

    return page;
}

bool BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty) {
    // Step 1: Check if page is in buffer pool
    if (page_table_.find(page_id) == page_table_.end()) {
        return false;
    }

    frame_id_t frame_id = page_table_[page_id];

    // Step 2: Check if page is already pinned
    if (pin_count_[frame_id] == 0) {
        return false;
    }

    // Step 3: Decrement pin count
    pin_count_[frame_id]--;

    // Step 4: Mark dirty if requested
    if (is_dirty) {
        is_dirty_[frame_id] = true;
    }

    // Step 5: If pin count reaches 0, add to replacer
    if (pin_count_[frame_id] == 0) {
        replacer_->Unpin(frame_id);
    }

    return true;
}

bool BufferPoolManager::FlushPage(page_id_t page_id) {
    // Step 1: Check if page is in buffer pool
    if (page_table_.find(page_id) == page_table_.end()) {
        return false; // Page not in buffer pool, can't flush
    } 

    frame_id_t frame_id = page_table_[page_id];

    if (!is_dirty_[frame_id]) {
        return true;
    }

    Page *page = &pages_[frame_id];
    disk_manager_->WritePage(page_id, page->GetData());

    is_dirty_[frame_id] = false;

    return true;
}

Page *BufferPoolManager::NewPage(page_id_t *page_id) {
    // Step 1: Find a victim frame
    frame_id_t frame_id;
    if (!FindVictimFrame(&frame_id)) {
        return nullptr;
    }

    // Step 2: Allocate a new page id from disk manager
    page_id_t new_page_id = disk_manager_->AllocatePage();

    // Step 3: Initialize the new page (make it empty)
    Page *page = &pages_[frame_id];
    page->Init(new_page_id);

    // Step 4: Update buffer pool metadata
    page_table_[new_page_id] = frame_id;
    pin_count_[frame_id] = 1;
    is_dirty_[frame_id] = true;

    // Step 5: Frame is pinned, so remove from replacer
    replacer_->Pin(frame_id);

    // Step 6: Return the new page_id to caller
    *page_id = new_page_id;

    return page;
}

bool BufferPoolManager::DeletePage(page_id_t page_id) {
    // Step 1: Check if page is in buffer pool
    auto it = page_table_.find(page_id);

    if (it == page_table_.end()) {
        // Page is in buffer pool
        frame_id_t frame_id = it->second;

        // Step 2: Check if page is pinned
        if (pin_count_[frame_id] > 0) {
            return false; // Page is pinned, can't delete
        }

        // Step 3: Remove from replacer (if it's there)
        replacer_->Pin(frame_id);

        // Step 4: Remove from page table
        page_table_.erase(page_id);

        // Step 5: Reset from metadata
        is_dirty_[frame_id] = false;

        // Step 6: Add from back to free list
        free_list_.push_back(frame_id);
    }

    // Step 7: Tell DiskManager to deallocate the page
    // (Whether page was in buffer or not, we need to free it on disk)
    disk_manager_->DeallocatePage(page_id);

    return true;
}

}
