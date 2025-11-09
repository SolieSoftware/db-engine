#pragma once

#include <unordered_map>
#include <list>
#include <vector>
#include "storage/page/page.h"
#include "storage/disk/disk_manager.h"
#include "storage/buffer/lru_replacer.h"
#include "common/config.h"

namespace dbengine {
    using page_id_t = int32_t;

    /**
    * BufferPoolManager manages the in-memory buffer pool of pages 

    * Responsibilities:
    * - Fetch pages from disk into memory
    * - Evict pages from memory when buffer is full
    * - Track which apges are pinned (in use)
    * - Write dirty pages back to disk
    */

    class BufferPoolManager {
        public:
        /**
        * Creates a new BufferPoolManager.
        * @param pool_size the size of the bufferv pool
        * @param disk_manager the disk manager 
        */

        BufferPoolManager(size_t pool_size, DiskManager *disk_manager);

        /**
        * Destroys the buffer pool manager and flushed all dirty pages.
        */
        ~BufferPoolManager();

        /**
        * Fetch a page from the buffer pool.
        * @param page_id the id of the page to fetch
        * @return pointer to the page, or nullptr if cannot fetch 
        */
        Page *FetchPage(page_id_t page_id);

        /**
        * Unpin a page, indicating you're done using it.
        * @param page_id the id of the page
        * @param is_dirty whether the pages was modified
        * @return false if page not in buffer or not pinned 
        */
        bool UnpinPage(page_id_t page_id, bool is_dirty);

        /**
        * Flush a page to disk (write if dirty)
        * @param page_id the id of the page
        * @return false is page not in buffer
        */
        bool FlushPage(page_id_t page_id);

        /**
        * Create a new page in the buffer pool.
        * @param[out] page_id the id of the new page
        * @return pointer to the new page, or nullptr if all frames pinned 
        */
        Page *NewPage(page_id_t * page_id);

        /**
        * Delete a page from the buffer pool and disk.
        * @param page_id the id of the page
        * @return fsle if page is pinned or doesn't exist 
        */
        bool DeletePage(page_id_t page_id);

        /**
        * Flush all dirty pages 
        */
        void FlushAllPages();

        private:
            // TODO: Add your data members here

            // Number of frames in the buffer pool
            size_t pool_size_;

            // Array of buffer pool pages
            Page *pages_;

            // Pointer to the disk manager
            DiskManager *disk_manager_;

            // Page table: maps page_id to frame_id
            std::unordered_map<page_id_t, frame_id_t> page_table_;

            // Replacer: Finds unpinned frames for eviction
            LRUReplacer *replacer_;

            // List of free frames (no page loaded)
            std::vector<size_t> pin_count_;

            // Dirty flag for each frame
            std::vector<bool> is_dirty_;

            // List of free frames (no page loaded)
            std::list<frame_id_t> free_list_;

            // Helper: Find a free frame or evict one
            bool FindVictimFrame(frame_id_t * frame_id);
    };
}