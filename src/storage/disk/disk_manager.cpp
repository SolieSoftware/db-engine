#include "storage/disk/disk_manager.h" // Your header file
#include <iostream> // For error messages
#include <cstring> // For memset (optional, to-zero-out buffers)


namespace dbengine {
DiskManager::DiskManager(const std::string &db_file): file_name_(db_file), num_pages_(0) {
    std::ifstream check_file(file_name_);
    if (!check_file.good()) {
        std::ofstream create_file(file_name_, std::ios::binary);
        if (!create_file.is_open()) {
            throw std::runtime_error("Failed to create database file: " + file_name_);
        }
        create_file.close();
    }
    check_file.close();

    // Try to open the file for read and write in binary mode.
    db_io_.open(file_name_, std::ios::in | std::ios::out | std::ios::binary);
    if (!db_io_.is_open()) {
        throw std::runtime_error("Failed to open database file: " + file_name_);
    }

    // Get the number of pages in the file.
    db_io_.seekg(0, std::ios::end);
    uint64_t file_size_bytes = static_cast<uint64_t>(db_io_.tellg());
    num_pages_ = file_size_bytes / PAGE_SIZE;
    db_io_.seekg(0, std::ios::beg);
}

DiskManager::~DiskManager() {
    // If the file is open, flush and close it.
    if (db_io_.is_open()) {
        db_io_.flush();
        db_io_.close();
    }
}

void DiskManager::WritePage(page_id_t page_id, const char *page_data) {
    // Using uint64_t for offset to avoid potential large page IDs causing overflows.
    uint64_t offset = static_cast<uint64_t>(page_id) * PAGE_SIZE;

    // Seek put position.
    db_io_.seekp(offset, std::ios::beg);

    // Write the page data to the file.
    db_io_.write(page_data, PAGE_SIZE);

    if (!db_io_) {
        throw std::runtime_error("Failed to write page to database file: " + file_name_);
    }

    // Flush the file to ensure data is written to disk.
    db_io_.flush();
 }

 void DiskManager::ReadPage(page_id_t page_id, char *page_data) {
    if (page_id >= num_pages_) {
        throw std::out_of_range("Page ID out of range: " + std::to_string(page_id));
    }
    uint64_t offset = static_cast<uint64_t>(page_id) * PAGE_SIZE;

    db_io_.seekg(offset, std::ios::beg);

    db_io_.read(page_data, PAGE_SIZE);

    if (!db_io_) {
        throw std::runtime_error("Failed to read page from database file: " + file_name_);
    }
 }

 page_id_t DiskManager::AllocatePage() {
    // First, check if we have any deallocated pages to reuse
    if (!free_list_.empty()) {
        page_id_t reused_page_id = free_list_.back();
        free_list_.pop_back();
        return reused_page_id;
    }

    // No free pages available, allocate a new page at the end
    return num_pages_++;
 }

 void DiskManager::DeallocatePage(page_id_t page_id) {
    // Validate the page_id
    if (page_id < 0 || page_id >= num_pages_) {
        // Invalid page_id, ignore the request
        return;
    }

    // Add the page to the free list for future reuse
    free_list_.push_back(page_id);

    // Note: We don't actually zero out the page data on disk
    // The page will be overwritten when it's reused by AllocatePage
 }
}