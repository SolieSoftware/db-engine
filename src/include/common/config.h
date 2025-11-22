#pragma once

#include <cstdint>


namespace dbengine {

    // Page size - typically 4KB
    constexpr uint32_t PAGE_SIZE = 4096;

    // Internal B Plus Tree Node page size
    constexpr int32_t INTERNAL_NODE_SIZE = 500;

    // Leaf B Plus Tree Node page size
    constexpr int32_t LEAF_PAGE_SIZE = 250;

    // Type alis for page IDs
    using page_id_t = int32_t;

    // Invalid page ID constant
    constexpr page_id_t INVALID_PAGE_ID = -1;


} // namespace dbengine