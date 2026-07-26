#ifndef BIT_UTILS_HPP
#define BIT_UTILS_HPP

#include <cstdint>

// Get the position of the highest bit 1 in a uint32_t number (counting from 0)
// If value is 0, return -1
// Examples:
//   value = 0b0001 (1)  -> returns 0
//   value = 0b0010 (2)  -> returns 1
//   value = 0b0100 (4)  -> returns 2
//   value = 0b1000 (8)  -> returns 3
//   value = 0b1011 (11) -> returns 3
//   value = 0           -> returns -1
inline int highest_bit_position(uint32_t value) {
    if (value == 0) {
        return -1;
    }

#if defined(_MSC_VER)
    // Windows/MSVC: use _BitScanReverse
    unsigned long index;
    _BitScanReverse(&index, value);
    return static_cast<int>(index);
#elif defined(__GNUC__) || defined(__clang__)
    // Linux/GCC/Clang: use __builtin_clz
    // __builtin_clz returns the count of leading zeros, so highest bit position = 31 - leading_zeros
    return 31 - __builtin_clz(value);
#else
    // Generic implementation (binary search)
    int pos = -1;
    if (value & 0xFFFF0000U) { value >>= 16; pos = 16; }
    if (value & 0x0000FF00U) { value >>= 8;  pos += 8; }
    if (value & 0x000000F0U) { value >>= 4;  pos += 4; }
    if (value & 0x0000000CU) { value >>= 2;  pos += 2; }
    if (value & 0x00000002U) {              pos += 1; }
    return pos;
#endif
}

// Get the position of the lowest bit 1 in a uint32_t number (counting from 0)
// If value is 0, return -1
inline int lowest_bit_position(uint32_t value) {
    if (value == 0) {
        return -1;
    }

#if defined(_MSC_VER)
    // Windows/MSVC: use _BitScanForward
    unsigned long index;
    _BitScanForward(&index, value);
    return static_cast<int>(index);
#elif defined(__GNUC__) || defined(__clang__)
    // Linux/GCC/Clang: use __builtin_ctz
    // __builtin_ctz returns the count of trailing zeros
    return static_cast<int>(__builtin_ctz(value));
#else
    // Generic implementation
    int pos = 0;
    while ((value & 1) == 0) {
        value >>= 1;
        ++pos;
    }
    return pos;
#endif
}

// Count the number of bit 1s in a uint32_t number (popcount)
inline int popcount(uint32_t value) {
#if defined(_MSC_VER)
    // MSVC 2010+ supports __popcnt
    // Requires compile option: /arch:SSE2 or higher
    return static_cast<int>(__popcnt(value));
#elif defined(__GNUC__) || defined(__clang__)
    // GCC/Clang: use __builtin_popcount
    return static_cast<int>(__builtin_popcount(value));
#else
    // Generic implementation
    int count = 0;
    while (value) {
        count += value & 1;
        value >>= 1;
    }
    return count;
#endif
}

// ==================== uint64_t versions ====================

// Get the position of the highest bit 1 in a uint64_t number (counting from 0)
// If value is 0, return -1
inline int highest_bit_position(uint64_t value) {
    if (value == 0) {
        return -1;
    }

#if defined(_MSC_VER)
    // Windows/MSVC: use _BitScanReverse64
    unsigned long index;
    _BitScanReverse64(&index, value);
    return static_cast<int>(index);
#elif defined(__GNUC__) || defined(__clang__)
    // Linux/GCC/Clang: use __builtin_clzll
    // __builtin_clzll returns the count of leading zeros, so highest bit position = 63 - leading_zeros
    return 63 - __builtin_clzll(value);
#else
    // Generic implementation (binary search)
    int pos = -1;
    if (value & 0xFFFFFFFF00000000ULL) { value >>= 32; pos = 32; }
    if (value & 0x00000000FFFF0000ULL) { value >>= 16; pos += 16; }
    if (value & 0x000000000000FF00ULL) { value >>= 8;  pos += 8; }
    if (value & 0x00000000000000F0ULL) { value >>= 4;  pos += 4; }
    if (value & 0x000000000000000CULL) { value >>= 2;  pos += 2; }
    if (value & 0x0000000000000002ULL) {              pos += 1; }
    return pos;
#endif
}

// Get the position of the lowest bit 1 in a uint64_t number (counting from 0)
// If value is 0, return -1
inline int lowest_bit_position(uint64_t value) {
    if (value == 0) {
        return -1;
    }

#if defined(_MSC_VER)
    // Windows/MSVC: use _BitScanForward64
    unsigned long index;
    _BitScanForward64(&index, value);
    return static_cast<int>(index);
#elif defined(__GNUC__) || defined(__clang__)
    // Linux/GCC/Clang: use __builtin_ctzll
    // __builtin_ctzll returns the count of trailing zeros
    return static_cast<int>(__builtin_ctzll(value));
#else
    // Generic implementation
    int pos = 0;
    while ((value & 1) == 0) {
        value >>= 1;
        ++pos;
    }
    return pos;
#endif
}

// Count the number of bit 1s in a uint64_t number (popcount)
inline int popcount(uint64_t value) {
#if defined(_MSC_VER)
    // MSVC 2010+ supports __popcnt64
    // Requires compile option: /arch:SSE2 or higher
    return static_cast<int>(__popcnt64(value));
#elif defined(__GNUC__) || defined(__clang__)
    // GCC/Clang: use __builtin_popcountll
    return static_cast<int>(__builtin_popcountll(value));
#else
    // Generic implementation
    int count = 0;
    while (value) {
        count += value & 1;
        value >>= 1;
    }
    return count;
#endif
}

#endif // BIT_UTILS_HPP
