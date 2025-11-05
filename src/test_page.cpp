#include "page/page.h"
#include <iostream>
#include <cstring>

using namespace dbengine;

void PrintTestHeader(const std:string &test_name) {
    std::cout << "=== " << test_name << " ===" << std::endl;
}

void PrintPageStats(const Page &page) {
    std::cout << "Page ID: " << page.GetPageId() << std::endl;
    std::cout << "Free Space: " << page.GetFreeSpace() << " bytes" << std::endl;
}

int main() {
    std::cout << "=== Page Class Test Suite ===" << std::endl;

    try {
        // Test 1: Initialize a page
        PrintTestHeader("Test 1: Initialize Page");
        Page page;
        page.Init(0);
        PrintPageStats(page);
        std::cout << "✓ Page initialized successfully" << std::endl;

        // Test 2: Insert a single record
        PrintTestHeader("Test 2: Insert Single Record");
        const char *record1 = "Alice";
        RID rid1;
        if (page.InsertRecord(record1, strlen(record1) + 1, rid1)) {
            std::cout << "✓ Inserted record: " << record1 << std::endl;      
            std::cout << "  RID: (page=" << rid1.GetPageId()
                      << ", slot=" << rid1.GetSlotNum() << ")" << std::endl;
            PrintPageStats(page);
        } else {
            std::cout << "✗ Failed to insert record" << std::endl;
            return 1;
        }

        // Test 3: Read the record back
        PrintTestHeader("Test 3: Read Record");
        char read_buffer[100];
        if (page.GetRecord(rid1, read_buffer)) {
            std::cout << "✓ Read record: " << read_buffer << std::endl;      
            if (strcmp(read_buffer, record1) == 0) {
                std::cout << "✓ Data matches!" << std::endl;
            } else {
                std::cout << "✗ Data mismatch!" << std::endl;
                return 1;
            }
        } else {
            std::cout << "✗ Failed to read record" << std::endl;
            return 1;
        }

        // Test 4: Insert multiple records
        PrintTestHeader("Test 4: Insert Multiple Records");
        const char *record2 = "Bob";
        const char *record3 = "Charlie";
        const char *record4 = "Diana";

        RID rid2, rid3, rid4;
        page.InsertRecord(record2, strlen(record2) + 1, rid2);
        page.InsertRecord(record3, strlen(record3) + 1, rid3);
        page.InsertRecord(record4, strlen(record4) + 1, rid4);

        std::cout << "✓ Inserted 3 more records" << std::endl;
        std::cout << "  RID2: slot " << rid2.GetSlotNum() << std::endl;      
        std::cout << "  RID3: slot " << rid3.GetSlotNum() << std::endl;      
        std::cout << "  RID4: slot " << rid4.GetSlotNum() << std::endl;      
        PrintPageStats(page);

        // Test 5: Read all records
        PrintTestHeader("Test 5: Read All Records");
        char buffer[100];

        page.GetRecord(rid1, buffer);
        std::cout << "  Slot " << rid1.GetSlotNum() << ": " << buffer << std::endl;

        page.GetRecord(rid2, buffer);
        std::cout << "  Slot " << rid2.GetSlotNum() << ": " << buffer << std::endl;

        page.GetRecord(rid3, buffer);
        std::cout << "  Slot " << rid3.GetSlotNum() << ": " << buffer << std::endl;

        page.GetRecord(rid4, buffer);
        std::cout << "  Slot " << rid4.GetSlotNum() << ": " << buffer << std::endl;

        std::cout << "✓ All records read successfully" << std::endl;

        // Test 6: Delete a record
        PrintTestHeader("Test 6: Delete Record");
        if (page.DeleteRecord(rid2)) {
            std::cout << "✓ Deleted record at slot " << rid2.GetSlotNum() << std::endl;
            PrintPageStats(page);

            // Try to read deleted record
            if (!page.GetRecord(rid2, buffer)) {
                std::cout << "✓ Cannot read deleted record (expected)" << std::endl;
            } else {
                std::cout << "✗ Should not be able to read deleted record!" << std::endl;
                return 1;
            }
        } else {
            std::cout << "✗ Failed to delete record" << std::endl;
            return 1;
        }

        // Test 7: Verify other records still exist
        PrintTestHeader("Test 7: Verify Other Records After Deletion");      
        if (page.GetRecord(rid1, buffer) && strcmp(buffer, record1) == 0){
            std::cout << "✓ Record 1 still intact: " << buffer << std::endl;
        }
        if (page.GetRecord(rid3, buffer) && strcmp(buffer, record3) == 0) {
            std::cout << "✓ Record 3 still intact: " << buffer << std::endl;
        }
        if (page.GetRecord(rid4, buffer) && strcmp(buffer, record4) == 0) {
            std::cout << "✓ Record 4 still intact: " << buffer << std::endl;
        }

        // Test 8: Reuse deleted slot
        PrintTestHeader("Test 8: Reuse Deleted Slot");
        const char *record5 = "Eve";
        RID rid5;
        if (page.InsertRecord(record5, strlen(record5) + 1, rid5)) {
            std::cout << "✓ Inserted new record: " << record5 << std::endl;
            std::cout << "  RID5: slot " << rid5.GetSlotNum() << std::endl;

            if (rid5.GetSlotNum() == rid2.GetSlotNum()) {
                std::cout << "✓ Reused deleted slot " << rid2.GetSlotNum() << "!" << std::endl;
            } else {
                std::cout << "  Note: Created new slot instead of reusing" << std::endl;
            }

            PrintPageStats(page);
        }

        // Test 9: Update a record (same size)
        PrintTestHeader("Test 9: Update Record (Same Size)");
        const char *updated_record = "Frank";  // Same length as "Alice" (6 chars including null)
        if (page.UpdateRecord(rid1, updated_record, strlen(updated_record) + 1)) {
            std::cout << "✓ Updated record at slot " << rid1.GetSlotNum() << std::endl;

            page.GetRecord(rid1, buffer);
            if (strcmp(buffer, updated_record) == 0) {
                std::cout << "✓ Record updated correctly: " << buffer << std::endl;
            } else {
                std::cout << "✗ Record update failed!" << std::endl;
                return 1;
            }
        } else {
            std::cout << "✗ Failed to update record" << std::endl;
            return 1;
        }

        // Test 10: Try to delete already deleted record
        PrintTestHeader("Test 10: Double Delete Test");
        if (!page.DeleteRecord(rid2)) {
            std::cout << "✓ Cannot delete already-deleted record (expected)" << std::endl;
        } else {
            std::cout << "✗ Should not be able to delete twice!" << std::endl;
            return 1;
        }

        // Test 11: Fill page test (see how many records we can fit)
        PrintTestHeader("Test 11: Fill Page Test");
        Page full_page;
        full_page.Init(1);

        int record_count = 0;
        char small_record[10] = "test";
        RID temp_rid;

        while (full_page.InsertRecord(small_record, 5, temp_rid)) {
            record_count++;
        }

        std::cout << "✓ Inserted " << record_count << " records before page full" << std::endl;
        std::cout << "  Free space remaining: " << full_page.GetFreeSpace() << " bytes" << std::endl;

        // Test 12: Large record test
        PrintTestHeader("Test 12: Large Record Test");
        Page large_page;
        large_page.Init(2);

        char large_record[1000];
        memset(large_record, 'X', 999);
        large_record[999] = '\0';

        RID large_rid;
        if (large_page.InsertRecord(large_record, 1000, large_rid)) {        
            std::cout << "✓ Inserted 1000-byte record" << std::endl;
            PrintPageStats(large_page);

            // Verify large record
            char large_buffer[1000];
            if (large_page.GetRecord(large_rid, large_buffer)) {
                if (memcmp(large_record, large_buffer, 1000) == 0) {
                    std::cout << "✓ Large record verified correctly" << std::endl;
                } else {
                    std::cout << "✗ Large record data corrupted!" << std::endl;
                    return 1
                }
            }
        } else {
            std::cout << "✗ Failed to insert large record" << std::endl;     
            return 1;
        }

        std::cout << "\n========================================" << std::endl;
        std::cout << "✓✓✓ ALL TESTS PASSED! ✓✓✓" << std::endl;
        std::cout << "========================================" << std::endl;

    } catch (const std::exception &e) {
        std::cerr << "\n✗✗✗ TEST FAILED WITH EXCEPTION ✗✗✗" << std::endl;    
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}