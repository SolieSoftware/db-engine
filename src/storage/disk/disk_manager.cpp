#include "storage/disk/disk_manager.h" // Your header file
#include <iostream> // For error messages
#include <cstring> // For memset (optional, to-zero-out buffers)


namespace dbengine {
DiskManager::DiskManager(const std::string &db_file): file_name_(db_file), num_pages_(0) {
    // TODO: Initialize the file name and number of pages.
    std::ifstream check_file(file_name_);
    if (!check_file.good()) {
        // File doesn't exists, create an empty one
        std::ofstream create_file(file_name_, std::ios::binary);
        if (!create_file.is_open()) {
            throw std::runtime_error("Failed to create database file: " + file_name_);
        }
        create_file.close();
    }
    check_file.close();

    // Try to open the file for read and write in binary mode.
    db_io_.open(file_name_, std::ios::in | std::ios::out | std::ios::binary);
    // If the file cannot be opened, throw an error.
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

    // Seek to the correct position in the file.
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
    return num_pages_++;
 }

 void DiskManager::DeallocatePage(page_id_t page_id) {
    // Simple implementation: do nothing
    (void) page_id; // Suppres unused parameter warning
    
 }
}