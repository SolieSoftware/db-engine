#include "storage/table/table_heap.h"
#include "common/config.h"
#include <cassert>

namespace dbengine {

    TableHeap::TableHeap(BufferPoolManager *bpm) : bpm_(bpm), first_page_id_(INVALID_PAGE_ID), last_page_id_(INVALID_PAGE_ID) {

    Page *first_page = bpm_->NewPage(&first_page_id_);
    last_page_id_ = first_page_id_;
    bpm_->UnpinPage(first_page_id_, false);

    }

    TableHeap::~TableHeap() {}

    bool TableHeap::InsertTuple(const Tuple &tuple, RID &rid) {

        // Evauate whether the tuple can fit into a page.
        if (tuple.GetSize() > PAGE_SIZE - sizeof(PageHeader) - sizeof(Slot)) {
            return false;
        }

        Page *page = bpm_->FetchPage(last_page_id_);
        if (page->InsertRecord(tuple.GetData(), tuple.GetSize(), rid)) {
            bpm_->UnpinPage(last_page_id_, true);
            return true;
        }

        bpm_->UnpinPage(last_page_id_, false);

        Page *new_page = bpm_->NewPage(&last_page_id_);

        if (new_page->InsertRecord(tuple.GetData(), tuple.GetSize(), rid)) {
            bpm_->UnpinPage(last_page_id_, true);
            return true;
        }

        bpm_->UnpinPage(last_page_id_, false);

        return false;
    }

    bool TableHeap::GetTuple(const RID &rid, Tuple &tuple) {
        page_id_t page_id = rid.GetPageId();
        Page *page = bpm_->FetchPage(page_id);

        tuple.Allocate(PAGE_SIZE - sizeof(PageHeader) - sizeof(Slot));

        if (page->GetRecord(rid, tuple.GetData())) {

            bpm_->UnpinPage(page_id, false);
            tuple.SetRID(rid);
            return true;
        }

        bpm_->UnpinPage(page_id, false);
        return false;
    }

    bool TableHeap::DeleteTuple(const RID &rid) {
        page_id_t page_id = rid.GetPageId();
        Page *page = bpm_->FetchPage(page_id);
        if (page->DeleteRecord(rid)) {
            bpm_->UnpinPage(page_id, true);
            return true;
        }

        bpm_->UnpinPage(page_id, false);
        return false;
    }

    bool TableHeap::UpdateTuple(const Tuple &new_tuple, const RID &rid) {
        page_id_t page_id = rid.GetPageId();
        Page *page = bpm_->FetchPage(page_id);
        if (page->UpdateRecord(rid, new_tuple.GetData(), new_tuple.GetSize())) {
            bpm_->UnpinPage(page_id, true);
            return true;
        }

        bpm_->UnpinPage(page_id, false);
        return false;
    }
}