#pragma once

#include <cstdint>
#include <cstring>
#include "config.h"
#include "common/rid.h"


namespace dbengine {

    struct Slot {
        uint32_t offset;
        uint32_t size;
    };


    struct PageHeader {
        uint32_t num_slots;
        uint32_t num_records;
        uint32_t free_space_pointer;
        uint32_t page_id;
    };


    class Page {
        public:
        /**
        * Initialize an empty page 
        */

        void Init(int32_t page_id);

        /**
        * Insert a record into the page.
        * @param data pointer to record data
        * @param size size of the record
        * @param rid output parameter - the RID of the inserted record
        * @return true if the insert succeeds, false if not enough space 
        */

        bool InsertRecord(const char *data, uint32_t size, RID &rid);

        /**
        * Get a record from the page.
        * @param rid the record ID to fetch
        * @param data output buffer to write record data
        * @return true if the record exists, false if deleted or invalid
         */
         bool GetRecord(const RID &rid, char *data) const;

         /**
         * Delete a record from the page.
         * @param rid th record ID to delete
         * @return true if delete succeeds, false if already deleted or invalid 
         */
         bool DeleteRecord(const RID &rid);

         /**
         * Update a record in palce (if new size fits in old space) 
         * @param rid the record ID to update
         * @param data new record data
         * @param size size of new record
         * @return true if udpate succeeds, false if not enough space 
         */
         bool UpdateRecord(const RID &rid, const char *data, uint32_t size);

         /**
         * Get the raw page data (for writing to disk).
         * @return pointer to the raw page data
          */
          char *GetData() { return data_; }
          const char *GetData() const { return data_; }

          /** 
           * Get page ID
           */
           int32_t GetPageId() const;

           /**
           * Get free space available in the page 
           */
           uint32_t GetFreeSpace() const;

           private: 
           // Helper: Get pointer to page header
           PageHeader *GetHeader() {
            return reinterpret_cast<PageHeader *>(data_);
           }

           const PageHeader *GetHeader() const {
            return reinterpret_cast<const PageHeader *>(data_);
           }

           // Helper: Get pointer to slot array
           Slot *GetSlotArray() {
            return reinterpret_cast<Slot *>(data_ + sizeof(PageHeader));
           }

           const Slot *GetSlotArray() const {
            return reinterpret_cast<const Slot *>(data_ + sizeof(PageHeader));
           }

           // Helper: Get pointer to record data
           Slot *GetSlot(uint32_t slot_num) {
            return GetSlotArray() + slot_num;
           }

           const Slot *GetSlot(int32_t slot_num) const {
            return GetSlotArray() + slot_num;
           }

           // The actual page data (4KB)
           char data_[PAGE_SIZE];
    };
}