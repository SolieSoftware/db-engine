#include "storage/buffer/buffer_pool_manager.h"
#include "storage/table/table_heap.h"
#include <iostream>
#include <cstring>
#include <cassert>

using namespace dbengine;


void PrintTestHeader(const std::string &test_name) {
    std::cout << "=== " << test_name << " ===" << std::endl;
}


void TestSimpleInsertAndGet() {
    PrintTestHeader("Test 1: Simple Insert and Get");

    std::remove("test_table_heap.db");
    DiskManager disk_manager("test_table_heap.db");
    BufferPoolManager bpm(5, &disk_manager);
    TableHeap table_heap(&bpm);

    // Create a tuple
    const char *data = "Test Tuple Data";
    Tuple tuple;
    tuple.Allocate(strlen(data) + 1);
    strcpy(tuple.GetData(), data);

    // Insert the tuple
    RID rid;
    bool insert_result = table_heap.InsertTuple(tuple, rid);
    assert(insert_result == true);

    std::cout << "✓ Tuple inserted at RID (" << rid.GetPageId() << ", " << rid.GetSlotNum() << ")" << std::endl;

    // Retrieve the tuple
    Tuple fetched_tuple;
    bool get_result = table_heap.GetTuple(rid, fetched_tuple);
    assert(get_result == true);
    assert(strcmp(fetched_tuple.GetData(), data) == 0);

    std::cout << "✓ Tuple retrieved successfully: " << fetched_tuple.GetData() << std::endl;

    std::cout << "✓ Simple insert and get test passed" << std::endl;
}


void TestMultipleInserts() {
    PrintTestHeader("Test 2: Multiple Inserts");

    std::remove("test_table_heap.db");
    DiskManager disk_manager("test_table_heap.db");
    BufferPoolManager bpm(5, &disk_manager);
    TableHeap table_heap(&bpm);

    const int num_tuples = 10;
    RID rids[num_tuples];
    const char *data_prefix = "Tuple Data ";

    // Insert multiple tuples
    for (int i = 0; i < num_tuples; i++) {
        Tuple tuple;
        tuple.Allocate(20);
        sprintf(tuple.GetData(), "%s-%d", data_prefix, i);

        bool insert_result = table_heap.InsertTuple(tuple, rids[i]);
        assert(insert_result == true);
    }

    std::cout << "✓ Inserted " << num_tuples << " tuples" << std::endl;

    // Retrieve and verify each tuple
    for (int i = 0; i < num_tuples; i++) {
        Tuple fetched_tuple;
        bool get_result = table_heap.GetTuple(rids[i], fetched_tuple);
        assert(get_result == true);

        char expected_data[20];
        sprintf(expected_data, "%s-%d", data_prefix, i);
        assert(strcmp(fetched_tuple.GetData(), expected_data) == 0);
    }

    std::cout << "✓ Retrieved and verified all tuples successfully" << std::endl;

    std::cout << "✓ Multiple inserts test passed" << std::endl;
}

void TestDeleteTuple() {
    PrintTestHeader("Test 3: Delete Tuple");

    std::remove("test_table_heap.db");
    DiskManager disk_manager("test_table_heap.db");
    BufferPoolManager bpm(5, &disk_manager);
    TableHeap table_heap(&bpm);

    const char *data = "Tuple to be deleted";
    const char *data2 = "Tuple to not be deleted";
    RID rids[2];

    // Insert both tuples
    for (int i = 0; i < 2; i++) {
        Tuple tuple;
        tuple.Allocate(strlen(i == 0 ? data : data2) + 1);
        strcpy(tuple.GetData(), i == 0 ? data : data2);

        bool insert_result = table_heap.InsertTuple(tuple, rids[i]);
        assert(insert_result == true);
    }

    // Delete the tuple
    bool delete_result = table_heap.DeleteTuple(rids[0]);
    assert(delete_result == true);

    // Fail to get deleted tuple
    Tuple fetched_tuple;
    fetched_tuple.Allocate(PAGE_SIZE - sizeof(PageHeader) - sizeof(Slot));
    bool get_result = table_heap.GetTuple(rids[0], fetched_tuple);
    assert(get_result == false);
    std::cout << "✓ Delete Tuple cannot be fetched" << std::endl;

    // Fail to update deleted tuple
    Tuple new_tuple;
    const char *update_data = "Updated Data";
    new_tuple.Allocate(strlen(update_data) + 1);
    strcpy(new_tuple.GetData(), update_data);
    bool update_result = table_heap.UpdateTuple(new_tuple, rids[0]);
    assert(update_result == false);
    std::cout << "✓ Deleted tuple cannot be updated" << std::endl;

    // Verify other tuples still exists
    Tuple fetched_tuple2;
    fetched_tuple2.Allocate(PAGE_SIZE - sizeof(PageHeader) - sizeof(Slot));
    bool get_result2 = table_heap.GetTuple(rids[1], fetched_tuple2);
    assert(get_result2 == true);
    std::cout << "✓ Other tuple still exists: " << fetched_tuple2.GetData() << std::endl;

    std::cout << "✓ Delete tuple test passed" << std::endl;

}

void TestUpdateTuple() {
    PrintTestHeader("Test 4: Update Tuple");

    std::remove("test_table_heap.db");
    DiskManager disk_manager("test_table_heap.db");
    BufferPoolManager bpm(5, &disk_manager);
    TableHeap table_heap(&bpm);

    const char *original_data = "Original Tuple Data";
    const char *updated_data = "Updated Tuple Data";
    RID rid;

    // Insert the original tuple
    Tuple tuple;
    tuple.Allocate(strlen(original_data) + 1);
    strcpy(tuple.GetData(), original_data);

    bool insert_result = table_heap.InsertTuple(tuple, rid);
    assert(insert_result == true);
    std::cout << "✓ Original tuple inserted at RID (" << rid.GetPageId() << ", " << rid.GetSlotNum() << ")" << std::endl;

    // Update the tuple
    Tuple new_tuple;
    new_tuple.Allocate(strlen(updated_data) + 1);
    strcpy(new_tuple.GetData(), updated_data);

    bool update_result = table_heap.UpdateTuple(new_tuple, rid);
    assert(update_result == true);
    std::cout << "✓ Tuple updated at RID (" << rid.GetPageId() << ", " << rid.GetSlotNum() << ")" << std::endl;

    // Retrieve and verify the updated tuple
    Tuple fetched_tuple;
    bool get_result = table_heap.GetTuple(rid, fetched_tuple);
    assert(get_result == true);
    assert(strcmp(fetched_tuple.GetData(), updated_data) == 0);

    std::cout << "✓ Tuple updated and verified successfully: " << fetched_tuple.GetData() << std::endl;

    std::cout << "✓ Update tuple test passed" << std::endl;
}

void TestMultiPageScenario() {
    PrintTestHeader("Test 5: Multi-Page Scenario");

    std::remove("test_table_heap.db");
    DiskManager disk_manager("test_table_heap.db");
    BufferPoolManager bpm(3, &disk_manager);  // Small buffer pool to force
    TableHeap table_heap(&bpm);

    const int num_tuples = 20;  // More than can fit in a single page
    RID rids[num_tuples];
    const char *data_prefix = "MultiPage Tuple ";

    // Insert multiple tuples
    for (int i = 0; i < num_tuples; i++) {
        Tuple tuple;
        tuple.Allocate(30);
        sprintf(tuple.GetData(), "%s-%d", data_prefix, i);

        bool insert_result = table_heap.InsertTuple(tuple, rids[i]);
        assert(insert_result == true);
    }
    std::cout << "✓ Inserted " << num_tuples << " tuples across multiple pages" << std::endl;

    // Verify first page id is 0
    page_id_t first_page_id = table_heap.GetFirstPageId();
    assert(first_page_id == 0);
    std::cout << "✓ First page ID is correct: " << first_page_id << std::endl;

    // Verify number of pages allocated is greater than 1
    int num_pages = disk_manager.GetNumPages();
    assert(num_pages > 1);
    std::cout << "✓ Multiple pages allocated: " << num_pages << std::endl;

    // Get tuples from both pages
    for (int i = 0; i < num_tuples; i++) {
        Tuple fetched_tuple;
        bool get_result = table_heap.GetTuple(rids[i], fetched_tuple);
        assert(get_result == true);

        char expected_data[30];
        sprintf(expected_data, "%s-%d", data_prefix, i);
        assert(strcmp(fetched_tuple.GetData(), expected_data) == 0);
    }

    std::cout << "✓ Retrieved and verified tuples from multiple pages successfully" << std::endl;

}

int main() {

    std::cout << "=== TableHeap Class Test Suite ===" << std::endl;

    try {
        std::remove("test_table_heap.db");
        TestSimpleInsertAndGet();
        TestMultipleInserts();
        TestDeleteTuple();
        TestUpdateTuple();
        TestMultiPageScenario();

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