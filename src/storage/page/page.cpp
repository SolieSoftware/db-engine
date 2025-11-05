#include "page/page.h"
#include <cstring>
#include <algorithm>

namespace dbengine {

    /**
    * Initialize an empty page 
    */

    void Page::Init(int32_t page_id) {
        // Zero out all the page data
        memset(data_, 0, PAGE_SIZE);

        // Set up the header
        PageHeader *header = GetHeader();
        header->num_slots = 0;
        header->num_records = 0;
        header->free_space_pointer = PAGE_SIZE; // Start at END
        header->page_id = page_id;
    }

    /** 
    * Get page ID 
    */

    int32_t Page::GetPageId() const {
        return GetHeader()->page_id;
    }

    /**
    * Get free space available in the page
     */

     uint32_t Page::GetFreeSpace() const {
        const PageHeader *header = GetHeader();

        // Free space is between the end of slot array and the start of records
        uint32_t slot_array_end = sizeof(PageHeader) + (header->num_slots * sizeof(Slot));
        uint32_t records_start = header->free_space_pointer;

        // If we need to add a new slot, acccount for that space 
        if (records_start >= slot_array_end) {
            return records_start - slot_array_end;
        }

        return 0; // No free space (shouldn't happend in normal operation)
     }

     /** 
      * Insert a record into the page
      */

      bool Page::InsertRecord(const char *data, uint32_t size, RID &rid) {
            PageHeader *header = GetHeader();

            // Calculate space needed
            uint32_t slot_array_end = sizeof(PageHeader) + ((header->num_slots + 1) * sizeof(Slot));

            uint32_t space_needed = size;

            // Check if there's engouh free space
            // Free space is between slot array (growing down) and records (growing up from bottom)

            uint32_t free_space_Start = sizeof(PageHeader) + (header->num_slots * sizeof(Slot));
            uint32_t free_space_end = header->free_space_pointer;

            // Need space for nes slot AND the record
            if (header->free_space_pointer + space_needed > PAGE_SIZE - (slot_array_end - sizeof(PageHeader))) {
                return false; // Not enough space
            }

            // Find an empty slot (deleted record) to reuse, or create a new one
            uint32_t slot_num = -1;
            for (uint32_t i = 0; i < header->num_slots; i++) {
                Slot *slot = GetSlot(i);
                if (slot->size == 0) { // Deleted slot, reuse it
                    slot_num = i;
                    break;
                }
            }

            // If no empty slot found, create a new one
            if (slot_num == -1) {
                slot_num = header->num_slots;
                header->num_slots++;
            }

            // Place record at the END, growing backwards. 
            uint32_t record_offset = header->free_space_pointer - size;

            // Copy the record data into the page
            memcpy(data_ + record_offset, data, size);

            // Update the slot
            Slot *slot = GetSlot(slot_num);
            slot->offset = record_offset;
            slot->size = size;

            // Update header
            header->num_records++;
            header->free_space_pointer = record_offset; // Move free space pointer backwards

            // Set the output RID
            rid = RID(header->page_id, slot_num);

            return true;
      }
      
      /*
      * Get a record from the page
      */
      bool Page::GetRecord(const RID &rid, char *date) {
        const PageHeader *header = GetHeader();

        // Validate slot number
        int32_t slot_num = rid.GetSlotNum();
        if (slot_num < 0 || static_cast<uint32_t>(slot_num) >= header->num_slots) {
            return false; // Invalid slot number
        }

        // Get the slot
        const Slot *slot = GetSlot(slot_num);

        // Check if record is deleted
        if (slot->size == 0) {
            return false; // Record was deleted
        }

        // Copy record data to output buffer
        memcpy(data, data_ + slot->offset, slot->size);

        return true;
      }

      /** 
      * Delete a record from the page
        */
    bool Page::DeleteRecord(const RID &rid) {
        PageHeader *header = GetHeader();

        // Validate slot number
        int32_t slot_num = rid.GetSlotNum();
        if (slot_num < 0 || static_cast<uint32_t>(slot_num) >= header->num_slots) {
            return false; // Invalid slot number
        }

        // Get the slot
        Slot *slot = GetSlot(slot_num);

        if (slot->size == 0) {
            return false; // Already deleted
        }

        // Mark as deleted (set size to 0)
        slot->size = 0;
        // Note: We don't actually remove the data or reclaim space
        // A real databse would do "compaction" to reclaim space

        // Update header
        header->num_records--;

        return true;
    }

    /** 
    * Update a record in place
     */
     bool Page::UpdateRecord(const RID &rid, const char *data, uint32_t size) {
        PageHeader *header = GetHeader();

        // Validate the slot number
        int32_t slot_num = rid.GetSlotNum();
        if (slot_num < 0 || static_cast<uint32_t>(slot_num) >= header->num_slots) {
            return false; // Invalid slot number
        }

        // Get the slot
        Slot *slot = GetSlot(slot_num);

        if (slot->size == 0) {
            return false; // Record was deleted
        }

        // Simple case: new data fits in old spacev (same size or smaller)
        if (size <= slot->size) {
            // Overwrite in place
            memcpy(data_ + slot->offset, data, size);
            slot->size = size;
            return true;
        }

        // Complex case: new data is larger than the old psace
        // For now we'll delete the old record and insert a new one'
        // A real implementation would do compaction
        DeleteRecord(rid);
        RID new_rid;
        if (InsertRecrd(data, size, new_rid)) {
            // Copy the new RID's slot to the old slot position to maintain RID
            Slot *new_slot = GetSlot(new_rid.GetSlotNum());
            slot->offset = new_slot->offset;
            slot->size = new_slot->size;

            // Mark the new slot as empty
            new_slot->size = 0;

            header->num_records--; // Complete for the extra increment

            return true;
        }

        return false; // Not enough space for larger record
        
    }
    }