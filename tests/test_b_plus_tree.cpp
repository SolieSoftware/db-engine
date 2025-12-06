#include "storage/disk/disk_manager.h"
#include "storage/buffer/buffer_pool_manager.h"
#include "storage/index/b_plus_tree.h"
#include <iostream>
#include <cstring>
#include <cassert>
#include <vector>
#include <algorithm>
#include <random>

using namespace dbengine;

void PrintTestHeader(const std::string &test_name) {
    std::cout << "\n==== " << test_name << " =====" << std::endl;
}

void PrintTestSuccess(const std::string &test_name) {
    std::cout << "[SUCCESS] " << test_name << " passed!" << std::endl;
}

// Test 1: Single Insert and Search
void TestSimpleInsertAndSearch() {
    std::string test_name = "Test 1: Simple Insert and Search";
    PrintTestHeader(test_name);

    std::remove("test_bp.db");
    DiskManager disk_manager("test_bp.db");
    BufferPoolManager bpm(10, &disk_manager);
    BPlusTree bpt(&bpm, 15);

    RID rid(1, 5, 0);
    int32_t key = 45;

    // Insert
    bool inserted = bpt.Insert(key, rid);
    assert(inserted && "Insert should succeed");

    // Search
    RID search_rid;
    bool found = bpt.Search(key, search_rid);
    assert(found && "Search should find the key");
    assert(search_rid.GetPageId() == rid.GetPageId());
    assert(search_rid.GetSlotNum() == rid.GetSlotNum());
    assert(search_rid.GetGeneration() == rid.GetGeneration());

    PrintTestSuccess(test_name);
}

// Test 2: Multiple Inserts (No Split)
void TestMultipleInsertsNoSplit() {
    std::string test_name = "Test 2: Multiple Inserts (No Split)";
    PrintTestHeader(test_name);

    std::remove("test_bp2.db");
    DiskManager disk_manager("test_bp2.db");
    BufferPoolManager bpm(10, &disk_manager);
    BPlusTree bpt(&bpm, 15);

    // Insert 10 keys (less than max_size of 15)
    std::vector<int32_t> keys = {50, 20, 80, 10, 60, 30, 70, 40, 90, 25};

    for (size_t i = 0; i < keys.size(); ++i) {
        RID rid(1, i, 0);
        bool inserted = bpt.Insert(keys[i], rid);
        assert(inserted && "Insert should succeed");
    }

    // Verify all keys can be found
    for (size_t i = 0; i < keys.size(); ++i) {
        RID search_rid;
        bool found = bpt.Search(keys[i], search_rid);
        assert(found && "All inserted keys should be found");
        assert(search_rid.GetSlotNum() == static_cast<int32_t>(i));
    }

    PrintTestSuccess(test_name);
}

// Test 3: Search for Non-Existent Keys
void TestSearchNonExistent() {
    std::string test_name = "Test 3: Search for Non-Existent Keys";
    PrintTestHeader(test_name);

    std::remove("test_bp3.db");
    DiskManager disk_manager("test_bp3.db");
    BufferPoolManager bpm(10, &disk_manager);
    BPlusTree bpt(&bpm, 15);

    // Insert some keys
    std::vector<int32_t> keys = {10, 20, 30, 40, 50};
    for (size_t i = 0; i < keys.size(); ++i) {
        RID rid(1, i, 0);
        bpt.Insert(keys[i], rid);
    }

    // Search for keys that don't exist
    std::vector<int32_t> missing_keys = {5, 15, 25, 35, 60, 100};
    for (auto key : missing_keys) {
        RID search_rid;
        bool found = bpt.Search(key, search_rid);
        assert(!found && "Non-existent keys should not be found");
    }

    PrintTestSuccess(test_name);
}

// Test 4: Insert with Split (Exceeds max_size)
void TestInsertWithSplit() {
    std::string test_name = "Test 4: Insert with Split";
    PrintTestHeader(test_name);

    std::remove("test_bp4.db");
    DiskManager disk_manager("test_bp4.db");
    BufferPoolManager bpm(20, &disk_manager);
    BPlusTree bpt(&bpm, 5);  // Small max_size to force splits

    std::cout << "Inserting 10 keys (max_size=5, will cause splits)..." << std::endl;

    // Insert 10 keys (will cause multiple splits)
    for (int32_t i = 1; i <= 10; ++i) {
        RID rid(1, i, 0);
        bool inserted = bpt.Insert(i * 10, rid);
        assert(inserted && "Insert should succeed even with splits");
        std::cout << "Inserted key: " << (i * 10) << std::endl;
    }

    // Verify all keys are still searchable after splits
    std::cout << "Verifying all keys after splits..." << std::endl;
    for (int32_t i = 1; i <= 10; ++i) {
        RID search_rid;
        int32_t key = i * 10;
        bool found = bpt.Search(key, search_rid);
        if (!found) {
            std::cout << "ERROR: Could not find key " << key << std::endl;
        }
        assert(found && "All keys should be found after split");
        assert(search_rid.GetSlotNum() == i);
    }

    PrintTestSuccess(test_name);
}

// Test 5: Insert Sequential Keys
void TestSequentialInsert() {
    std::string test_name = "Test 5: Sequential Insert";
    PrintTestHeader(test_name);

    std::remove("test_bp5.db");
    DiskManager disk_manager("test_bp5.db");
    BufferPoolManager bpm(50, &disk_manager);
    BPlusTree bpt(&bpm, 10);

    std::cout << "Inserting 50 sequential keys..." << std::endl;

    // Insert sequential keys
    for (int32_t i = 0; i < 50; ++i) {
        RID rid(2, i, 0);
        bool inserted = bpt.Insert(i, rid);
        assert(inserted && "Sequential insert should succeed");
    }

    // Verify in order
    std::cout << "Verifying sequential access..." << std::endl;
    for (int32_t i = 0; i < 50; ++i) {
        RID search_rid;
        bool found = bpt.Search(i, search_rid);
        assert(found && "Sequential key should be found");
        assert(search_rid.GetPageId() == 2);
        assert(search_rid.GetSlotNum() == i);
    }

    PrintTestSuccess(test_name);
}

// Test 6: Insert Random Keys
void TestRandomInsert() {
    std::string test_name = "Test 6: Random Insert";
    PrintTestHeader(test_name);

    std::remove("test_bp6.db");
    DiskManager disk_manager("test_bp6.db");
    BufferPoolManager bpm(50, &disk_manager);
    BPlusTree bpt(&bpm, 8);

    std::cout << "Inserting 30 random keys..." << std::endl;

    // Generate random keys
    std::vector<int32_t> keys;
    for (int32_t i = 0; i < 30; ++i) {
        keys.push_back(i * 10);
    }

    // Shuffle them
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(keys.begin(), keys.end(), g);

    // Insert in random order
    for (size_t i = 0; i < keys.size(); ++i) {
        RID rid(3, i, 0);
        bool inserted = bpt.Insert(keys[i], rid);
        assert(inserted && "Random insert should succeed");
    }

    // Verify all keys
    std::cout << "Verifying all random keys..." << std::endl;
    for (size_t i = 0; i < keys.size(); ++i) {
        RID search_rid;
        bool found = bpt.Search(keys[i], search_rid);
        assert(found && "Random key should be found");
    }

    PrintTestSuccess(test_name);
}

// Test 7: Delete Single Key
void TestSimpleDelete() {
    std::string test_name = "Test 7: Simple Delete";
    PrintTestHeader(test_name);

    std::remove("test_bp7.db");
    DiskManager disk_manager("test_bp7.db");
    BufferPoolManager bpm(20, &disk_manager);
    BPlusTree bpt(&bpm, 10);

    // Insert some keys
    std::vector<int32_t> keys = {10, 20, 30, 40, 50};
    for (size_t i = 0; i < keys.size(); ++i) {
        RID rid(1, i, 0);
        bpt.Insert(keys[i], rid);
    }

    // Delete middle key
    std::cout << "Deleting key 30..." << std::endl;
    bool deleted = bpt.Delete(30);
    assert(deleted && "Delete should succeed");

    // Verify 30 is gone
    RID search_rid;
    bool found = bpt.Search(30, search_rid);
    assert(!found && "Deleted key should not be found");

    // Verify others still exist
    for (auto key : {10, 20, 40, 50}) {
        bool exists = bpt.Search(key, search_rid);
        assert(exists && "Other keys should still exist");
    }

    PrintTestSuccess(test_name);
}

// Test 8: Delete Non-Existent Key
void TestDeleteNonExistent() {
    std::string test_name = "Test 8: Delete Non-Existent Key";
    PrintTestHeader(test_name);

    std::remove("test_bp8.db");
    DiskManager disk_manager("test_bp8.db");
    BufferPoolManager bpm(10, &disk_manager);
    BPlusTree bpt(&bpm, 10);

    // Insert some keys
    bpt.Insert(10, RID(1, 0, 0));
    bpt.Insert(20, RID(1, 1, 0));

    // Try to delete non-existent key
    std::cout << "Attempting to delete non-existent key 30..." << std::endl;
    bool deleted = bpt.Delete(30);
    assert(!deleted && "Delete of non-existent key should return false");

    // Verify original keys still exist
    RID search_rid;
    assert(bpt.Search(10, search_rid) && "Original keys should still exist");
    assert(bpt.Search(20, search_rid) && "Original keys should still exist");

    PrintTestSuccess(test_name);
}

// Test 9: Duplicate Key Insert
void TestDuplicateInsert() {
    std::string test_name = "Test 9: Duplicate Key Insert";
    PrintTestHeader(test_name);

    std::remove("test_bp9.db");
    DiskManager disk_manager("test_bp9.db");
    BufferPoolManager bpm(10, &disk_manager);
    BPlusTree bpt(&bpm, 10);

    // Insert a key
    RID rid1(1, 5, 0);
    bool inserted1 = bpt.Insert(100, rid1);
    assert(inserted1 && "First insert should succeed");

    // Try to insert same key again (current implementation may allow duplicates)
    RID rid2(2, 10, 0);
    bool inserted2 = bpt.Insert(100, rid2);

    std::cout << "Duplicate insert result: " << (inserted2 ? "allowed" : "rejected") << std::endl;

    // Search and see what we get
    RID search_rid;
    bool found = bpt.Search(100, search_rid);
    assert(found && "Key should be found");

    std::cout << "Found RID: page=" << search_rid.GetPageId()
              << ", slot=" << search_rid.GetSlotNum() << std::endl;

    PrintTestSuccess(test_name);
}

// Test 10: Stress Test - Large Number of Keys
void TestStressLargeInsert() {
    std::string test_name = "Test 10: Stress Test - 100 Keys";
    PrintTestHeader(test_name);

    std::remove("test_bp10.db");
    DiskManager disk_manager("test_bp10.db");
    BufferPoolManager bpm(100, &disk_manager);
    BPlusTree bpt(&bpm, 10);

    const int NUM_KEYS = 100;
    std::cout << "Inserting " << NUM_KEYS << " keys..." << std::endl;

    // Insert many keys
    for (int32_t i = 0; i < NUM_KEYS; ++i) {
        RID rid(i / 10, i % 10, 0);
        bool inserted = bpt.Insert(i, rid);
        assert(inserted && "Stress insert should succeed");

        if ((i + 1) % 20 == 0) {
            std::cout << "Inserted " << (i + 1) << " keys..." << std::endl;
        }
    }

    std::cout << "Verifying all " << NUM_KEYS << " keys..." << std::endl;

    // Verify all keys
    for (int32_t i = 0; i < NUM_KEYS; ++i) {
        RID search_rid;
        bool found = bpt.Search(i, search_rid);
        assert(found && "Stress test key should be found");

        if ((i + 1) % 20 == 0) {
            std::cout << "Verified " << (i + 1) << " keys..." << std::endl;
        }
    }

    PrintTestSuccess(test_name);
}

int main() {
    std::cout << "\n";
    std::cout << "========================================" << std::endl;
    std::cout << "   B+ Tree Test Suite                  " << std::endl;
    std::cout << "========================================" << std::endl;

    try {
        // Basic tests
        TestSimpleInsertAndSearch();
        TestMultipleInsertsNoSplit();
        TestSearchNonExistent();

        // Split tests
        TestInsertWithSplit();
        TestSequentialInsert();
        TestRandomInsert();

        // Delete tests
        TestSimpleDelete();
        TestDeleteNonExistent();

        // Edge cases
        TestDuplicateInsert();

        // Stress test
        TestStressLargeInsert();

        std::cout << "\n========================================" << std::endl;
        std::cout << "   ALL TESTS PASSED!                   " << std::endl;
        std::cout << "========================================\n" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "\n[ERROR] Test failed with exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
