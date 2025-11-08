#include "storage/buffer/lru_replacer.h"
#include <iostream>
#include <cassert>

using namespace dbengine;

void TestSimpleLRU() {
    std::cout << "=== Test 1: Simple LRU ===" << std::endl;

    LRUReplacer replacer(3);

    // Initially empty
    assert(replacer.Size() == 0);

    // Unpin some frames
    replacer.Unpin(1);
    replacer.Unpin(2);
    replacer.Unpin(3);

    assert(replacer.Size() == 3);

    // Evict - should get frame 1 (least recent)
    frame_id_t victim;
    assert(replacer.Victim(&victim) == true);
    assert(victim == 1);
    assert(replacer.Size() == 2);

    std::cout << "✓ Simple LRU test passed" << std::endl;
}

void TestPinUnpin() {
    std::cout << "=== Test 2: Pin/Unpin ===" << std::endl;

    LRUReplacer replacer(3);

    replacer.Unpin(1);
    replacer.Unpin(2);
    replacer.Unpin(3);

    // Pin frame 2 (remove from evictable list)
    replacer.Pin(2);
    assert(replacer.Size() == 2);

    // Victim should be 1 (oldest unpinned)
    frame_id_t victim;
    assert(replacer.Victim(&victim) == true);
    assert(victim == 1);

    std::cout << "✓ Pin/Unpin test passed" << std::endl;
}

void TestRefreshLRU() {
    std::cout << "=== Test 3: Refresh LRU Position ===" << std::endl;

    LRUReplacer replacer(3);

    replacer.Unpin(1);
    replacer.Unpin(2);
    replacer.Unpin(3);
    // Order: [3, 2, 1]

    // Unpin 1 again (move to front)
    replacer.Unpin(1);
    // Order: [1, 3, 2]

    // Victim should be 2 (now oldest)
    frame_id_t victim;
    assert(replacer.Victim(&victim) == true);
    assert(victim == 2);

    std::cout << "✓ Refresh LRU test passed" << std::endl;
}

void TestEmptyVictim() {
    std::cout << "=== Test 4: Empty Victim ===" << std::endl;

    LRUReplacer replacer(3);

    frame_id_t victim;
    assert(replacer.Victim(&victim) == false);

    std::cout << "✓ Empty victim test passed" << std::endl;
}

void TestPinNonExistent() {
    std::cout << "=== Test 5: Pin Non-existent Frame ===" << std::endl;

    LRUReplacer replacer(3);

    // Pin a frame that was never unpinned - should not crash
    replacer.Pin(1);
    assert(replacer.Size() == 0);

    std::cout << "✓ Pin non-existent test passed" << std::endl;
}

int main() {
    std::cout << "=== LRU Replacer Test Suite ===" << std::endl;

    try {
        TestSimpleLRU();
        TestPinUnpin();
        TestRefreshLRU();
        TestEmptyVictim();
        TestPinNonExistent();

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