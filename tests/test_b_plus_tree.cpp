#include "storage/disk/disk_manager.h"
#include "storage/buffer/buffer_pool_manager.h"
#include "storage/index/b_plus_tree.h"
#include <iostream>
#include <cstring>
#include <cassert>


using namespace dbengine;

void PrintTestHeader(const std::string &test_name) {
    std::cout << "==== " << test_name << " =====" << std::endl;
}

void PrintTestSuccess(const std::string &test_name) {
    std::cout << "==== GREAT SUCESS ====" << std::endl;
    std::cout << test_name << " passed all tests!" << std::endl;
}

void InsertAndSearch() {
    std::string test_name = "Test 1: Simple Insert and Search";
    PrintTestHeader(test_name);
    std::cout << "1. Removing old DB file..." << std::endl;
    std::remove("test_bp.db");
    std::cout << "2. Creating DiskManager..." << std::endl;
    DiskManager disk_manager("test_bp.db");
    std::cout << "3. Creating BufferPoolManager..." << std::endl;
    BufferPoolManager bpm(10, &disk_manager);
    std::cout << "4. Creating BPlusTree..." << std::endl;
    BPlusTree bpt(&bpm, 15);

    std::cout << "5. Creating RID and key..." << std::endl;
    RID rid(1, 5, 0);
    int32_t key = 45;

    std::cout << "6. Inserting key=" << key << "..." << std::endl;
    bool inserted = bpt.Insert(key, rid);
    std::cout << "7. Insert returned: " << inserted << std::endl;

    // Insert operation successful
    assert(inserted);

    RID search_rid;
    bool search = bpt.Search(key, search_rid);
    assert(search);

    assert(search_rid.GetPageId() == rid.GetPageId() &&
            search_rid.GetSlotNum() == rid.GetSlotNum() &&
            search_rid.GetGeneration() == rid.GetGeneration());

    PrintTestSuccess(test_name);
}


int main() {
    InsertAndSearch();
    return 0;
}