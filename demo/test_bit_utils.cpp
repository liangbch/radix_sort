#include "bit_utils.hpp"
#include <iostream>
#include <cassert>

void test_highest_bit_position() {
    std::cout << "Testing highest_bit_position..." << std::endl;

    // Test cases: value -> expected result
    struct TestCase {
        uint32_t value;
        int expected;
    };

    TestCase tests[] = {
        {0, -1},       // all zeros, return -1
        {1, 0},        // 0b0001 -> bit 0
        {2, 1},        // 0b0010 -> bit 1
        {3, 1},        // 0b0011 -> bit 1
        {4, 2},        // 0b0100 -> bit 2
        {5, 2},        // 0b0101 -> bit 2
        {7, 2},        // 0b0111 -> bit 2
        {8, 3},        // 0b1000 -> bit 3
        {11, 3},       // 0b1011 -> bit 3
        {0x80000000U, 31}, // highest bit at position 31
        {0xFFFFFFFFU, 31}, // all ones, highest bit at position 31
        {0x55555555U, 30}, // 0b010101... highest bit at position 30
    };

    for (const auto& test : tests) {
        int result = highest_bit_position(test.value);
        std::cout << "  highest_bit_position(0x" << std::hex << test.value
                  << std::dec << ") = " << result;
        if (result == test.expected) {
            std::cout << " OK" << std::endl;
        } else {
            std::cout << " FAILED (expected " << test.expected << ")" << std::endl;
            assert(false);
        }
    }

    std::cout << "All tests passed!" << std::endl << std::endl;
}

void test_lowest_bit_position() {
    std::cout << "Testing lowest_bit_position..." << std::endl;

    struct TestCase {
        uint32_t value;
        int expected;
    };

    TestCase tests[] = {
        {0, -1},        // all zeros, return -1
        {1, 0},         // 0b0001 -> bit 0
        {2, 1},         // 0b0010 -> bit 1
        {4, 2},         // 0b0100 -> bit 2
        {8, 3},         // 0b1000 -> bit 3
        {6, 1},         // 0b0110 -> bit 1
        {12, 2},        // 0b1100 -> bit 2
        {0x80000000U, 31}, // only highest bit
        {0xFFFFFFFFU, 0},  // all ones, lowest bit at position 0
    };

    for (const auto& test : tests) {
        int result = lowest_bit_position(test.value);
        std::cout << "  lowest_bit_position(0x" << std::hex << test.value
                  << std::dec << ") = " << result;
        if (result == test.expected) {
            std::cout << " OK" << std::endl;
        } else {
            std::cout << " FAILED (expected " << test.expected << ")" << std::endl;
            assert(false);
        }
    }

    std::cout << "All tests passed!" << std::endl << std::endl;
}

void test_popcount() {
    std::cout << "Testing popcount..." << std::endl;

    struct TestCase {
        uint32_t value;
        int expected;
    };

    TestCase tests[] = {
        {0, 0},         // all zeros, 0 ones
        {1, 1},         // 0b0001 -> 1 one
        {2, 1},         // 0b0010 -> 1 one
        {3, 2},         // 0b0011 -> 2 ones
        {0xF, 4},       // 0b1111 -> 4 ones
        {0xFF, 8},      // 8 ones
        {0xFFFF, 16},   // 16 ones
        {0xFFFFFFFFU, 32}, // 32 ones
        {0x55555555U, 16}, // 16 ones (alternating bits)
        {0xAAAAAAAAU, 16}, // 16 ones (alternating bits, shifted)
    };

    for (const auto& test : tests) {
        int result = popcount(test.value);
        std::cout << "  popcount(0x" << std::hex << test.value
                  << std::dec << ") = " << result;
        if (result == test.expected) {
            std::cout << " OK" << std::endl;
        } else {
            std::cout << " FAILED (expected " << test.expected << ")" << std::endl;
            assert(false);
        }
    }

    std::cout << "All tests passed!" << std::endl << std::endl;
}

int main() {
    std::cout << "Bit Utils Test Program" << std::endl;
    std::cout << "======================" << std::endl << std::endl;

    test_highest_bit_position();
    test_lowest_bit_position();
    test_popcount();

    std::cout << "======================" << std::endl;
    std::cout << "All tests completed successfully!" << std::endl;

    return 0;
}
