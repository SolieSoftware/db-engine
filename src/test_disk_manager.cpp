#include "storage/disk_manager.h"
#include <iostream>
#include <cstring>

using namespace dbengine;

int main() {
    std::cout << "=== DiskManager Test ===" << std::endl;

    try {
        // Test 1: Create DiskManager (creates/opens file)
        std::cout << "Test 1: Create DiskManager (creates/opens file)" << std::endl;
        DiskManager disk_manager("test.db");
        std::cout << "DiskManager create successfully" << std::endl;

        // Test 2: Allocate pages
        std::cout << "[Test 2] Allocating pages..." << std::endl;
        page_id_t page1 = disk_manager.AllocatePage();
        page_id_t page2 = disk_manager.AllocatePage();
        std::cout << "[Success] Allocated pages: " << page1 << ", " << page2 << std::endl;

        // Test 3: Write data to a page
        std::cout << "[Test 3] Writing and reading pages..." << std::endl;
        char write_buffer[PAGE_SIZE];
        memset(write_buffer, 0, PAGE_SIZE);
        strcpy(write_buffer, "Hello, Database World!");
        disk_manager.WritePage(page1, write_buffer);
        std::cout << "Wrote: \"" << write_buffer << "\"" << std::endl;

        // Test 4: Read data from a page
        std::cout << "[Test 4] Reading page..." << std::endl;
        char read_buffer[PAGE_SIZE];
        memset(read_buffer, 0, PAGE_SIZE);
        disk_manager.ReadPage(page1, read_buffer);
        std::cout << "Read: \"" << read_buffer << "\"" << std::endl;

        // Test 5: Verify data matches
        std::cout << "[Test 5] Verifying data matches..." << std::endl;
        if (strcmp(write_buffer, read_buffer) == 0) {
            std::cout << "[Success] Write and read data matches!" << std::endl;
        } else {
            std::cout << "[Failure] Data mismatches!" << std::endl;
            return 1;
        }

        // Test 6: Write to another page
        std::cout << "[Test 6] Writing to page 1..." << std::endl;
        char write_buffer2[PAGE_SIZE]; 
        memset(write_buffer2, 0, PAGE_SIZE);
        strcpy(write_buffer2, "This is page 1 data");
        disk_manager.WritePage(page2, write_buffer2);
        std::cout << "Wrote: \"" << write_buffer2 << "\" to page 1" << std::endl;

        // Test 7: Read both pages to ensure they're independent
        std::cout << "[Test 7] Verifying pages are independent..." << std::endl;
        char read_buffer0[PAGE_SIZE];
        char read_buffer1[PAGE_SIZE];
        disk_manager.ReadPage(page1, read_buffer0);
        disk_manager.ReadPage(page2, read_buffer1);
        std::cout << "Page 0: \"" << read_buffer0 << "\"" << std::endl;
        std::cout << "Page 1: \"" << read_buffer1 << "\"" << std::endl;
        if (strcmp(write_buffer, read_buffer0) == 0 && strcmp(write_buffer2, read_buffer1) == 0) {
            std::cout << "[Success] Pages are independent!" << std::endl;
        } else {
            std::cout << "[Failure] Pages are not independent!" << std::endl;
            return 1;
        }

    // Test 8: Try reading from unallocated page
    std::cout << "[Test 8] Testing error handling (reading page 100).." << std::endl;
    try {
         char error_buffer[PAGE_SIZE];
         disk_manager.ReadPage(100, error_buffer);
         std::cout << "[Failure] Should have thrown an error!" << std::endl;
         return 1;

    } catch (const std::out_of_range &e) {
        std::cout << "[Success] Caught expected out of range error: " << e.what() << std::endl;
    }
    
    std::cout << "[ALL TESTS PASSED SUCCESSFULLY!]" << std::endl;

    } catch (const std::exception &e) {
        std::cerr << "[Error] Test failed: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}