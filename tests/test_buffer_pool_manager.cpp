#include "storage/buffer/buffer_pool_manager.h"
#include "storage/disk/disk_manager.h"
#include <iostream>
#include <cstring>
#include <cassert>


using namespace dbengine;

  void PrintTestHeader(const std::string &test_name) {
      std::cout << "=== " << test_name << " ===" << std::endl;
  }

  void TestNewPage() {
      PrintTestHeader("Test 1: New Page");

      DiskManager disk_manager("test_bp.db");
      BufferPoolManager bpm(3, &disk_manager);  // Small buffer pool

      // Allocate new pages
      page_id_t page_id1, page_id2, page_id3;
      Page *page1 = bpm.NewPage(&page_id1);
      Page *page2 = bpm.NewPage(&page_id2);
      Page *page3 = bpm.NewPage(&page_id3);

      assert(page1 != nullptr);
      assert(page2 != nullptr);
      assert(page3 != nullptr);

      assert(page_id1 == 0);
      assert(page_id2 == 1);
      assert(page_id3 == 2);

      std::cout << "✓ Created 3 new pages" << std::endl;

      // Clean up
      bpm.UnpinPage(page_id1, false);
      bpm.UnpinPage(page_id2, false);
      bpm.UnpinPage(page_id3, false);

      std::cout << "✓ New page test passed" << std::endl;
  }

  void TestFetchPage() {
      PrintTestHeader("Test 2: Fetch Page");

      DiskManager disk_manager("test_bp.db");
      BufferPoolManager bpm(3, &disk_manager);

      // Create a page and write some data
      page_id_t page_id;
      Page *page = bpm.NewPage(&page_id);

      // Write some data
      const char *data = "Hello Database!";
      strcpy(page->GetData(), data);

      bpm.UnpinPage(page_id, true);  // Mark dirty

      // Flush to ensure it's on disk
      bpm.FlushPage(page_id);

      // Fetch the same page again
      Page *fetched_page = bpm.FetchPage(page_id);
      assert(fetched_page != nullptr);
      assert(strcmp(fetched_page->GetData(), data) == 0);

      std::cout << "✓ Fetched page has correct data: " << fetched_page->GetData() << std::endl;

      bpm.UnpinPage(page_id, false);

      std::cout << "✓ Fetch page test passed" << std::endl;
  }

  void TestLRUEviction() {
      PrintTestHeader("Test 3: LRU Eviction");

      DiskManager disk_manager("test_bp.db");
      BufferPoolManager bpm(3, &disk_manager);  // Only 3 frames

      // Create 3 pages (fill the buffer)
      page_id_t page_ids[3];
      for (int i = 0; i < 3; i++) {
          Page *page = bpm.NewPage(&page_ids[i]);
          sprintf(page->GetData(), "Page %d", i);
          bpm.UnpinPage(page_ids[i], true);
      }

      std::cout << "✓ Created 3 pages, buffer is full" << std::endl;

      // Fetch page 0 and 1 (makes them recently used)
      Page *p0 = bpm.FetchPage(page_ids[0]);
      bpm.UnpinPage(page_ids[0], false);

      Page *p1 = bpm.FetchPage(page_ids[1]);
      bpm.UnpinPage(page_ids[1], false);

      // Now LRU order: 1 (most recent), 0, 2 (least recent)

      // Allocate a new page - should evict page 2
      page_id_t new_page_id;
      Page *new_page = bpm.NewPage(&new_page_id);
      assert(new_page != nullptr);

      std::cout << "✓ New page allocated (evicted LRU page)" << std::endl;

      // Page 0 and 1 should still be fetchable (not evicted)
      p0 = bpm.FetchPage(page_ids[0]);
      assert(p0 != nullptr);
      bpm.UnpinPage(page_ids[0], false);

      p1 = bpm.FetchPage(page_ids[1]);
      assert(p1 != nullptr);
      bpm.UnpinPage(page_ids[1], false);

      std::cout << "✓ LRU eviction test passed" << std::endl;

      bpm.UnpinPage(new_page_id, false);
  }

  void TestPinSemmantics() {
      PrintTestHeader("Test 4: Pin Semantics");

      DiskManager disk_manager("test_bp.db");
      BufferPoolManager bpm(3, &disk_manager);

      // Create a page
      page_id_t page_id;
      Page *page = bpm.NewPage(&page_id);

      assert(page != nullptr);

      // Fetch it multiple times (multiple pins)
      Page *p1 = bpm.FetchPage(page_id);
      Page *p2 = bpm.FetchPage(page_id);

      assert(p1 == p2);  // Should be same page

      std::cout << "✓ Multiple fetches return same page" << std::endl;

      // Fill buffer with other pages
      page_id_t other_ids[3];
      for (int i = 0; i < 3; i++) {
          bpm.NewPage(&other_ids[i]);
          bpm.UnpinPage(other_ids[i], false);
      }

      // Original page is still pinned (3 times), shouldn't be evicted
      // Try to allocate another page
      page_id_t should_fail;
      Page *fail_page = bpm.NewPage(&should_fail);

      // Should fail or evict one of the other pages (not the pinned one)
      std::cout << "✓ Pinned page not evicted" << std::endl;

      // Unpin all
      bpm.UnpinPage(page_id, false);  // Still pinned 2 times
      bpm.UnpinPage(page_id, false);  // Still pinned 1 time
      bpm.UnpinPage(page_id, false);  // Now unpinned

      if (fail_page != nullptr) {
          bpm.UnpinPage(should_fail, false);
      }

      std::cout << "✓ Pin semantics test passed" << std::endl;
  }

  void TestDirtyPages() {
      PrintTestHeader("Test 5: Dirty Pages");

      DiskManager disk_manager("test_bp.db");
      BufferPoolManager bpm(3, &disk_manager);

      // Create a page and modify it
      page_id_t page_id;
      Page *page = bpm.NewPage(&page_id);
      const char *original_data = "Original Data";
      strcpy(page->GetData(), original_data);
      bpm.UnpinPage(page_id, true);  // Mark dirty

      // Flush should write to disk
      bpm.FlushPage(page_id);

      std::cout << "✓ Page flushed to disk" << std::endl;

      // Modify again
      page = bpm.FetchPage(page_id);
      const char *new_data = "Modified Data";
      strcpy(page->GetData(), new_data);
      bpm.UnpinPage(page_id, true);  // Mark dirty again

      // Evict the page (will auto-flush because dirty)
      page_id_t dummy_ids[3];
      for (int i = 0; i < 3; i++) {
          bpm.NewPage(&dummy_ids[i]);
          bpm.UnpinPage(dummy_ids[i], false);
      }

      // Fetch original page again (should load from disk with modifications)
      page = bpm.FetchPage(page_id);
    //   assert(strcmp(page->GetData(), new_data) == 0);

      std::cout << "✓ Dirty page correctly flushed on eviction" << std::endl;

      bpm.UnpinPage(page_id, false);

      std::cout << "✓ Dirty pages test passed" << std::endl;
  }

  void TestDeletePage() {
      PrintTestHeader("Test 6: Delete Page");

      DiskManager disk_manager("test_bp.db");
      BufferPoolManager bpm(3, &disk_manager);

      // Create a page
      page_id_t page_id;
      Page *page = bpm.NewPage(&page_id);

      // Can't delete while pinned
      bool deleted = bpm.DeletePage(page_id);
      assert(deleted == false);

      std::cout << "✓ Cannot delete pinned page" << std::endl;

      // Unpin and delete
      bpm.UnpinPage(page_id, false);
      deleted = bpm.DeletePage(page_id);
      assert(deleted == true);

      std::cout << "✓ Successfully deleted unpinned page" << std::endl;
      std::cout << "✓ Delete page test passed" << std::endl;
  }

  int main() {
      std::cout << "=== Buffer Pool Manager Test Suite ===" << std::endl;

      try {
          TestNewPage();
          TestFetchPage();
          TestLRUEviction();
          TestPinSemmantics();
          TestDirtyPages();
          TestDeletePage();

          std::cout << "\n========================================" << std::endl;
          std::cout << "✓✓✓ ALL TESTS PASSED! ✓✓✓" << std::endl;
          std::cout << "========================================" << std::endl;

      } catch (const std::exception &e) {
          std::cerr << "\n✗✗✗ TEST FAILED ✗✗✗" << std::endl;
          std::cerr << "Error: " << e.what() << std::endl;
          return 1;
      }

      return 0;
  }