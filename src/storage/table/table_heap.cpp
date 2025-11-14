#include "storage/table/table_heap.h"
#include <cassert>

namespace dbengine {

    TableHeap::TableHeap(BufferPoolManager *bpm) : buffer_pool_manager_(bpm), first_page_id_(INVALID_PAGE_ID), last_page_id_(INVALID_PAGE_ID) {}

    // TODO: Allocate the first page for this table
    // Hint: Use buffer_pool_manager_->NewPage()
    // Hint: Initialize the page using Page::Init()
    // Hint: Set both first_page_id_ and last_page_id_ to this new page
    // Hint: Don't forget to unpin the page when done!
    
}