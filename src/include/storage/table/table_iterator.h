#pragma once

#include "storage/table/table_heap.h"
#include "storage/table/tuple.h"
#include "common/rid.h"

namespace dbengine {

class TableIterator {
public:
    TableIterator(TableHeap *table_heap, BufferPoolManager *bpm)
        : table_heap_(table_heap), bpm_(bpm),
          current_page_id_(table_heap->GetFirstPageId()),
          current_slot_(0), current_page_(nullptr) {

        if (current_page_id_ != INVALID_PAGE_ID) {
            current_page_ = bpm_->FetchPage(current_page_id_);
        }
    }

    ~TableIterator() {
        if (current_page_ != nullptr) {
            bpm_->UnpinPage(current_page_id_, false);
        }
    }

    bool HasNext() {
        if (current_page_ == nullptr) {
            return false;
        }

        PageHeader *header = reinterpret_cast<PageHeader *>(current_page_->GetData());

        while (current_slot_ < header->num_slots) {
            Slot *slot_array = reinterpret_cast<Slot *>(
                current_page_->GetData() + sizeof(PageHeader)
            );
            Slot *slot = &slot_array[current_slot_];

            if (slot->size > 0) {
                return true;
            }
            current_slot_++;
        }

        return false;
    }

    bool Next(Tuple &tuple, RID &rid) {
        if (!HasNext()) {
            return false;
        }

        PageHeader *header = reinterpret_cast<PageHeader *>(current_page_->GetData());
        Slot *slot_array = reinterpret_cast<Slot *>(
            current_page_->GetData() + sizeof(PageHeader)
        );

        while (current_slot_ < header->num_slots) {
            Slot *slot = &slot_array[current_slot_];

            if (slot->size > 0) {
                rid = RID(current_page_id_, current_slot_, slot->generation);

                if (table_heap_->GetTuple(rid, tuple)) {
                    current_slot_++;
                    return true;
                }
            }
            current_slot_++;
        }

        return false;
    }

private:
    TableHeap *table_heap_;
    BufferPoolManager *bpm_;
    page_id_t current_page_id_;
    uint32_t current_slot_;
    Page *current_page_;
};

}
