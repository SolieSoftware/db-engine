#pragma once

#include <string>
#include <fstream>
#include <vector>
#include "common/config.h"

namespace dbengine {
    
    class DiskManager {
        public: 

        explicit DiskManager(const std::string &db_file);

         ~DiskManager();

         void WritePage(page_id_t page_id, const char *page_data);

         void ReadPage(page_id_t page_id, char *page_data);

         page_id_t AllocatePage();

         void DeallocatePage(page_id_t page_id);

         inline int32_t GetNumPages() const { return num_pages_; };

         private:
         std::fstream db_io_;  // Stream for database file
         std::string file_name_; // Database file name
         int32_t num_pages_;   // Number of pages in the file
         std::vector<page_id_t> free_list_;  // Deallocated pages available for reuse

    };


}