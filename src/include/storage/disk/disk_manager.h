#pragma once

#include <string>
#include <fstream>
#include "common/config.h"

namespace dbengine {
    
    class DiskManager {
        public: 
        /**
        * Creates a new disk manager that writes to the specified database file.
        * @param db_file the file name of the databse file 
        */
        explicit DiskManager(const std::string &db_file);

        /**
        * Destructor - ensure file is properly clsoed 
         */
         ~DiskManager();

         /**
         * Write a page to the databse file.
         * @param page_id id of the page to write
         * @param page_data raw page data to write 
         */
         void WritePage(page_id_t page_id, const char *page_data);

         /**
         * Read a page from the databse file.
         * @param page_id id of the page to read
         * @param page_data output buffer for page data 
         */
         void ReadPage(page_id_t page_id, char *page_data);

         /**
         * Allocate a new page in the databse file.
         * @return the id of the allocated page
         */
         page_id_t AllocatePage();

         void DeallocatePage(page_id_t page_id);

         private:
         std::fstream db_io_;  // Stream for database file
         std::string file_name_; // Database file name
         int32_t num_pages_;   // Number of pages in the file

    };


}