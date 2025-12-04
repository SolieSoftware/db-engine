# Database Engine (C++)

A learning-focused relational database engine implementation in C++17, built from the ground up to understand fundamental database system concepts.

## Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [Components](#components)
- [Building the Project](#building-the-project)
- [Running Tests](#running-tests)
- [Project Structure](#project-structure)
- [Key Concepts](#key-concepts)
- [Usage Examples](#usage-examples)
- [Contributing](#contributing)

---

## Overview

This project implements core components of a relational database management system (RDBMS), focusing on the storage layer and indexing mechanisms. It's designed as an educational tool to learn about:

- **Disk-based storage management**
- **Buffer pool management with LRU eviction**
- **B+ tree indexing**
- **Page-oriented architecture**
- **Transaction concepts** (future work)

The engine uses a page-based storage model (4KB pages) similar to production databases like PostgreSQL and MySQL/InnoDB.

---

## Architecture

The database engine follows a **layered architecture**:

```
┌─────────────────────────────────────────┐
│          Query Layer (Future)           │
├─────────────────────────────────────────┤
│         Index Layer (B+ Tree)           │
├─────────────────────────────────────────┤
│      Table Heap / Tuple Management      │
├─────────────────────────────────────────┤
│      Buffer Pool Manager (LRU)          │
├─────────────────────────────────────────┤
│         Disk Manager (I/O)              │
└─────────────────────────────────────────┘
```

### Design Principles

1. **Separation of Concerns**: Each layer has a well-defined responsibility
2. **Page-Based Storage**: All data is organized into fixed-size pages (4KB)
3. **Buffer Pool Abstraction**: In-memory caching layer sits between storage and upper layers
4. **Indirection via RIDs**: Record Identifiers (RID) provide stable references to tuples

---

## Components

### 1. Disk Manager (`src/storage/disk/disk_manager.cpp`)

Manages raw I/O operations to/from disk.

**Responsibilities:**
- Allocate and deallocate pages
- Read/write pages from/to disk files
- Track free pages and next page ID

**Key Methods:**
- `AllocatePage()`: Allocates a new page and returns its ID
- `ReadPage(page_id, data)`: Reads a page from disk into memory
- `WritePage(page_id, data)`: Writes a page from memory to disk

### 2. Page (`src/storage/page/page.cpp`)

Represents a 4KB block of data with slotted page structure.

**Structure:**
```
┌──────────────────────────────────────────┐
│ PageHeader (num_slots, free_space, etc) │
├──────────────────────────────────────────┤
│ Slot Array (offset, size, generation)   │
├──────────────────────────────────────────┤
│              Free Space                  │
├──────────────────────────────────────────┤
│         Record Data (grows ←)            │
└──────────────────────────────────────────┘
```

**Responsibilities:**
- Insert, update, delete, and retrieve records
- Manage free space within the page
- Support slot-based record addressing

### 3. Buffer Pool Manager (`src/storage/buffer/buffer_pool_manager.cpp`)

Manages an in-memory cache of pages using LRU eviction policy.

**Responsibilities:**
- Fetch pages from disk into memory
- Evict pages when buffer is full (LRU policy)
- Track pinned pages (in-use pages that cannot be evicted)
- Write dirty pages back to disk

**Key Methods:**
- `FetchPage(page_id)`: Brings a page into the buffer pool
- `UnpinPage(page_id, is_dirty)`: Marks page as no longer in use
- `NewPage(page_id*)`: Allocates a new page
- `FlushPage(page_id)`: Writes a dirty page to disk

**Pin/Unpin Protocol:**
- Pages must be **pinned** before use (prevents eviction)
- Pages must be **unpinned** after use (allows eviction)
- Forgetting to unpin causes buffer pool exhaustion

### 4. LRU Replacer (`src/storage/buffer/lru_replacer.cpp`)

Implements Least Recently Used eviction policy for the buffer pool.

**Responsibilities:**
- Track which frames are unpinned and available for eviction
- Select victim frames when buffer pool is full
- Maintain LRU ordering

### 5. Table Heap (`src/storage/table/table_heap.cpp`)

Manages a collection of pages storing table tuples.

**Responsibilities:**
- Insert tuples into pages
- Retrieve tuples by RID
- Update and delete tuples
- Manage multiple pages as a heap

### 6. B+ Tree Index (`src/storage/index/b_plus_tree.cpp`)

Disk-based B+ tree index for fast key lookups.

**Features:**
- Supports insertion, search, and deletion
- Automatic node splitting when full
- Internal node splitting with key promotion
- Leaf node merging on underflow
- Leaf nodes linked for efficient range scans

**Key Invariants:**
- All leaves at same level (balanced)
- Internal nodes: n keys, n+1 children
- Leaf nodes: n keys, n RIDs
- Keys sorted within nodes
- Minimum occupancy maintained (except root)

**Structure:**

```
Internal Nodes:
┌─────────────────────────────────────────────┐
│ [Header] [Keys] [Child Page IDs]           │
└─────────────────────────────────────────────┘

Leaf Nodes:
┌─────────────────────────────────────────────┐
│ [Header] [Keys] [RIDs] [next_page_id]      │
└─────────────────────────────────────────────┘
```

---

## Building the Project

### Prerequisites

- **C++17 compatible compiler** (GCC 7+, Clang 5+, MSVC 2017+)
- **CMake 3.15+**
- **Make** or **Ninja**

### Build Steps

```bash
# Clone the repository
git clone <repository-url>
cd db-engine

# Create build directory
mkdir -p build
cd build

# Generate build files
cmake ..

# Compile the project
make

# Or compile specific test
make test_b_plus_tree
```

### Build Artifacts

After building, you'll find executables in the `build/` directory:
- `test_disk_manager`
- `test_page`
- `test_lru_replacer`
- `test_buffer_pool_manager`
- `test_table_heap`
- `test_b_plus_tree`

---

## Running Tests

Each component has a corresponding test executable:

```bash
# From build directory
./test_disk_manager
./test_buffer_pool_manager
./test_b_plus_tree
```

Tests use simple assertion-based validation and print detailed output showing operations and results.

### Example Test Output

```
==== Test 1: Simple Insert and Search =====
1. Removing old DB file...
2. Creating DiskManager...
3. Creating BufferPoolManager...
4. Creating BPlusTree...
5. Creating RID and key...
6. Inserting key=45...
7. Insert returned: 1
==== GREAT SUCCESS ====
Test 1: Simple Insert and Search passed all tests!
```

---

## Project Structure

```
db-engine/
├── CMakeLists.txt              # Build configuration
├── README.md                   # This file
├── src/
│   ├── include/                # Header files
│   │   ├── common/
│   │   │   ├── config.h       # Global constants (PAGE_SIZE, etc)
│   │   │   └── rid.h          # Record Identifier definition
│   │   └── storage/
│   │       ├── disk/
│   │       │   └── disk_manager.h
│   │       ├── page/
│   │       │   └── page.h
│   │       ├── buffer/
│   │       │   ├── buffer_pool_manager.h
│   │       │   └── lru_replacer.h
│   │       ├── table/
│   │       │   ├── table_heap.h
│   │       │   └── tuple.h
│   │       └── index/
│   │           ├── b_plus_tree.h
│   │           ├── b_plus_tree_page.h
│   │           ├── b_plus_tree_leaf_page.h
│   │           └── b_plus_tree_page_internal.h
│   └── storage/                # Implementation files (.cpp)
│       ├── disk/
│       ├── page/
│       ├── buffer/
│       ├── table/
│       └── index/
├── tests/                      # Test executables
│   ├── test_disk_manager.cpp
│   ├── test_buffer_pool_manager.cpp
│   └── test_b_plus_tree.cpp
└── build/                      # Build artifacts (generated)
```

---

## Key Concepts

### Record Identifier (RID)

An RID uniquely identifies a tuple's physical location:

```cpp
class RID {
    int32_t page_id_;      // Which page contains the record
    int32_t slot_num_;     // Which slot within that page
    uint32_t generation_;  // Generation number (for slot reuse)
};
```

**Why use RIDs?**
- Provides **indirection**: Indices point to RIDs, not physical addresses
- Enables **data reorganization** without rebuilding indices
- Supports **slot reuse** via generation numbers

### Page-Based Storage

All data is organized into fixed-size **pages** (4KB):

**Benefits:**
- **I/O efficiency**: Read/write entire pages at once
- **Cache alignment**: Matches OS page size
- **Simplifies memory management**: Fixed-size allocation
- **Enables buffer pool**: Easy to manage in-memory cache

### Buffer Pool Manager

The buffer pool is the **heart of the database engine**:

1. **Demand Paging**: Only load pages needed for queries
2. **LRU Eviction**: Keep hot pages in memory, evict cold pages
3. **Dirty Tracking**: Only write modified pages to disk
4. **Pin/Unpin Protocol**: Prevents eviction of in-use pages

**Performance Impact:**
- Hit rate = (cache hits) / (total accesses)
- Higher hit rate = fewer disk I/Os = better performance
- Typical production systems aim for >99% hit rate

### B+ Tree Indexing

B+ trees provide **O(log n)** search, insert, and delete:

**Key Properties:**
- **Balanced**: All leaves at same depth
- **High fanout**: Each node has many children (reduces tree height)
- **Sorted**: Enables efficient range queries
- **Disk-friendly**: Nodes match page size

**Leaf vs Internal Nodes:**
- **Internal**: Store routing keys, point to child pages
- **Leaf**: Store actual data (keys + RIDs), linked for scans

---

## Usage Examples

### Example 1: Basic B+ Tree Operations

```cpp
#include "storage/disk/disk_manager.h"
#include "storage/buffer/buffer_pool_manager.h"
#include "storage/index/b_plus_tree.h"

using namespace dbengine;

int main() {
    // Create storage components
    DiskManager disk_manager("test.db");
    BufferPoolManager bpm(10, &disk_manager);
    BPlusTree tree(&bpm, 15);  // max 15 keys per node

    // Insert key-value pairs
    RID rid1(1, 0, 0);
    tree.Insert(100, rid1);

    RID rid2(1, 1, 0);
    tree.Insert(200, rid2);

    // Search for a key
    RID result;
    if (tree.Search(100, result)) {
        std::cout << "Found: page=" << result.GetPageId()
                  << ", slot=" << result.GetSlotNum() << std::endl;
    }

    // Delete a key
    tree.Delete(100);

    return 0;
}
```

### Example 2: Table Heap Usage

```cpp
#include "storage/table/table_heap.h"

using namespace dbengine;

int main() {
    DiskManager disk_manager("data.db");
    BufferPoolManager bpm(10, &disk_manager);
    TableHeap table(&bpm);

    // Insert a record
    const char *data = "Hello, Database!";
    RID rid;
    table.InsertTuple(data, strlen(data), &rid);

    // Retrieve the record
    char buffer[100];
    if (table.GetTuple(rid, buffer)) {
        std::cout << "Retrieved: " << buffer << std::endl;
    }

    return 0;
}
```

---

## Key Learnings & Design Decisions

### 1. Memory Safety
- Always check for `nullptr` after `FetchPage()` or `NewPage()`
- Properly initialize page headers (especially `max_size` field)
- Use `std::memset()` to zero out new pages before use

### 2. Resource Management
- Follow pin/unpin protocol strictly
- Unpin pages even on error paths
- Flush dirty pages before shutdown

### 3. Type Choices
- `uint32_t` for page fields (fixed-width for disk serialization)
- `size_t` for memory-related sizes (pool capacity, counts)
- `int32_t` for IDs that need sentinel values (INVALID_PAGE_ID = -1)

### 4. Data Structure Layout
B+ tree uses pointer arithmetic for in-page layout:
```cpp
data_ ───→ [Header][Keys Array][Values Array]
           ↑       ↑            ↑
           |       |            |
           0    sizeof(Header)  sizeof(Header) + max_size*sizeof(key)
```

This provides **cache efficiency** and **disk I/O optimization**.

---

## Future Work

- [ ] Query execution engine
- [ ] SQL parser and planner
- [ ] Transaction support (ACID properties)
- [ ] Write-ahead logging (WAL)
- [ ] Concurrency control (locking, MVCC)
- [ ] Additional index types (hash, GiST)
- [ ] Query optimization
- [ ] Statistics and cost-based planning

---

## Contributing

This is a learning project. Contributions that improve code clarity, add documentation, or implement new features are welcome!

### Guidelines
- Follow existing code style
- Add tests for new features
- Document complex algorithms
- Keep commits focused and atomic

---

## References

- **Database System Concepts** by Silberschatz, Korth, Sudarshan
- **Database Management Systems** by Ramakrishnan, Gehrke
- **CMU 15-445/645: Database Systems** course materials
- **PostgreSQL Internals** documentation
- **SQLite Architecture** documentation

---

## License

This project is for educational purposes. Feel free to use and modify for learning.

---

## Acknowledgments

Built as a learning exercise to understand database internals. Inspired by academic database courses and production database systems like PostgreSQL, MySQL, and SQLite.
