#pragma once

// =============================================================================
// count_for_radix.hpp
//
// 基数排序辅助函数：统计整数数组关键字 k 的取值分布，将关键字空间划分为
// 若干「大桶」（large bucket），并输出「小桶 -> 大桶」映射表 bucket_map。
//
// 设计依据：notes/count_for_radix.md
//
// 流程概要：
//   1. 带符号关键字先翻转最高符号位，单调映射到无符号域 [0, 2^N - 1]。
//   2. 按 h = floor(log2(k)) 粗分：k < 2^8      -> 大桶 0 (case 1)
//                                   2^8 <= k < 2^16 -> 大桶 1 (case 2)
//                                   k >= 2^16       -> 大桶 2..255 (case 3)
//   3. case 3 再按「(h-16) 分组 + 最高位之下 W 比特」细分为若干小桶 j，
//      采用 lo_delta(热,uint8_t) + counter(冷,CountType) 双层计数降低 cache 失效。
//   4. 贪心合并：将 case 3 的小桶合并为 254 个大桶 (2..255)，每桶元素数尽量接近
//      期望值 e = T / 254（T 为 case 3 元素总数）。
//
// 符号对照（设计文档符号 -> 代码变量）：
//   h(最高位) -> hi_bit； 合并组号 -> group； N(位宽) -> bits
//   W(细分位宽) -> fanout_bits； j(小桶号) -> j； e(期望) -> target； T(case3总数) -> case3_total
// =============================================================================

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace radix_detail {

// =============================================================================
// radix_key_info : 按 KeyType 偏特化的关键字描述信息
//   unsigned_type : 去符号后的无符号关键字类型
//   bits          : 关键字位宽（32 / 64）
//   case3_groups  : case 3 的分组数（h ∈ [16, bits-1] 的组数）
//   default_W     : 推荐细分位宽（依据 32 KB L1 data cache，见 notes 3.2 节）
//   to_unsigned   : 带符号 -> 无符号的单调映射（等价翻转最高符号位）
// =============================================================================
template <typename KeyType>
struct radix_key_info;  // 主模板不予定义：仅支持下列 4 种偏特化

// -------------------- uint32_t --------------------
template <>
struct radix_key_info<uint32_t>
{
    using unsigned_type = uint32_t;
    static constexpr int bits         = 32;
    static constexpr int case3_groups = 16;  // h ∈ [16, 31]
    static constexpr int default_W    = 10;  // 16 × 2^10 = 16 K ≈ 16 KB（留有余量）

    static constexpr unsigned_type to_unsigned(uint32_t key) noexcept { return key; }
};

// -------------------- int32_t --------------------
template <>
struct radix_key_info<int32_t>
{
    using unsigned_type = uint32_t;
    static constexpr int bits         = 32;
    static constexpr int case3_groups = 16;
    static constexpr int default_W    = 10;

    // 加 0x80000000（等价翻转最高符号位），把 [INT_MIN, INT_MAX] 单调映射到 [0, 2^32-1]
    static constexpr unsigned_type to_unsigned(int32_t key) noexcept
    {
        return static_cast<unsigned_type>(key) ^ (unsigned_type{1} << (bits - 1));
    }
};

// -------------------- uint64_t --------------------
template <>
struct radix_key_info<uint64_t>
{
    using unsigned_type = uint64_t;
    static constexpr int bits         = 64;
    static constexpr int case3_groups = 48;  // h ∈ [16, 63]
    static constexpr int default_W    = 9;   // 48 × 2^9  = 24 K ≈ 24 KB

    static constexpr unsigned_type to_unsigned(uint64_t key) noexcept { return key; }
};

// -------------------- int64_t --------------------
template <>
struct radix_key_info<int64_t>
{
    using unsigned_type = uint64_t;
    static constexpr int bits         = 64;
    static constexpr int case3_groups = 48;
    static constexpr int default_W    = 9;

    static constexpr unsigned_type to_unsigned(int64_t key) noexcept
    {
        return static_cast<unsigned_type>(key) ^ (unsigned_type{1} << (bits - 1));
    }
};


// =============================================================================
// highest_bit_position : 求整数最高置位比特的位置（从 0 起计）。
// 为了性能考虑，这里假定 value 总是大于 0；仅在 value > 0 的情况下使用。
// 模板主声明仅作接口约束，仅支持下列两种显式特化，其余类型不予定义。
// =============================================================================
template <typename T>
uint32_t highest_bit_position(T value) = delete;  // 仅 uint32_t/uint64_t 提供特化，其余类型编译期报错

// -------------------- 特化：uint32_t --------------------
template <>
inline uint32_t highest_bit_position<uint32_t>(uint32_t value) {
    assert(value != 0);   // 调用方需保证 value > 0（见函数头注释）
#if defined(_MSC_VER)
    // Windows/MSVC: use _BitScanReverse
    unsigned long index;
    _BitScanReverse(&index, value);
    return static_cast<uint32_t>(index);
#elif defined(__GNUC__) || defined(__clang__)
    // Linux/GCC/Clang: use __builtin_clz
    // __builtin_clz returns the count of leading zeros, so highest bit position = 31 - leading_zeros
    return 31 - __builtin_clz(value);
#else
    // Generic implementation (binary search)
    int pos = 0;
    if (value & 0xFFFF0000U) { value >>= 16; pos = 16; }
    if (value & 0x0000FF00U) { value >>= 8;  pos += 8; }
    if (value & 0x000000F0U) { value >>= 4;  pos += 4; }
    if (value & 0x0000000CU) { value >>= 2;  pos += 2; }
    if (value & 0x00000002U) {              pos += 1; }
    return static_cast<uint32_t>(pos);
#endif
}

// -------------------- 特化：uint64_t --------------------
template <>
inline uint32_t highest_bit_position<uint64_t>(uint64_t value) {
    assert(value != 0);   // 调用方需保证 value > 0（见函数头注释）
#if defined(_MSC_VER)
    // Windows/MSVC: use _BitScanReverse64
    unsigned long index;
    _BitScanReverse64(&index, value);
    return static_cast<uint32_t>(index);
#elif defined(__GNUC__) || defined(__clang__)
    // Linux/GCC/Clang: use __builtin_clzll
    // __builtin_clzll returns the count of leading zeros, so highest bit position = 63 - leading_zeros
    return 63 - __builtin_clzll(value);
#else
    // Generic implementation (binary search)
    int pos = 0;
    if (value & 0xFFFFFFFF00000000ULL) { value >>= 32; pos = 32; }
    if (value & 0x00000000FFFF0000ULL) { value >>= 16; pos += 16; }
    if (value & 0x000000000000FF00ULL) { value >>= 8;  pos += 8; }
    if (value & 0x00000000000000F0ULL) { value >>= 4;  pos += 4; }
    if (value & 0x000000000000000CULL) { value >>= 2;  pos += 2; }
    if (value & 0x0000000000000002ULL) {              pos += 1; }
    return static_cast<uint32_t>(pos);
#endif
}


}  // namespace radix_detail

#define USE_2WAY_LOOPUNROLL_IN_COUNT_FOR_RADIX

// =============================================================================
// key_dist_result : count_for_radix 的统计结果。
//   small_bucket_map[j] : case 3 小桶序号 j ∈ [0, small_bucket_count) -> 大桶序号 ∈ [2, 255]。
//   big_buckets[b]      : 大桶 b 的值域边界与元素数。
//                         桶 0 = case 1 (k < 2^8)，桶 1 = case 2 (2^8 ≤ k < 2^16)，
//                         桶 2..255 = case 3。
//   key_min/key_max     : 无符号映射域 U 下的值域边界；空桶 (count == 0) 时不定义。
// =============================================================================
template <
    typename CountType,
    typename KeyType,
    int      fanout_bits = radix_detail::radix_key_info<KeyType>::default_W
>
struct key_dist_result
{
    using info          = radix_detail::radix_key_info<KeyType>;
    using unsigned_type = typename info::unsigned_type;

    static constexpr int small_bucket_count = info::case3_groups << fanout_bits;  // case 3 小桶数
    static constexpr int big_bucket_count = 256;                   // 大桶总数（0..255）

    // 单个大桶的信息
    struct big_bucket_info
    {
        unsigned_type key_min;   // 该大桶覆盖的关键字值域下界（无符号域）
        unsigned_type key_max;   // 该大桶覆盖的关键字值域上界（无符号域）
        CountType     count;     // 该大桶中的元素数（== 0 表示空桶，此时 min/max 不定义）
    };

    std::array<uint8_t, small_bucket_count>    small_bucket_map{};  // 小桶 j -> 大桶序号
    std::array<big_bucket_info, big_bucket_count> big_buckets{};
};

// =============================================================================
// count_for_radix（带 extractor 实例的重载：核心实现）
//
// 模板参数：
//   CountType    —— 计数类型（元素总数 < 2^32 用 uint32_t，否则 uint64_t）
//   KeyType      —— 关键字类型（uint32_t/int32_t/uint64_t/int64_t，偏特化）
//   fanout_bits  —— 细分位宽（默认取 radix_key_info<KeyType>::default_W）
//   KeyExtractor —— 关键字提取器（可调用对象：给定元素返回关键字 k）
//   Iter         —— 随机访问迭代器类型（由参数推导）
//
// 函数参数：
//   begin, end   —— 待统计区间 [begin, end)
//   result       —— 输出：
//     result.small_bucket_map[j] —— case 3 小桶 j 所属的大桶索引 ∈ [2, 255]；
//                                   长度 == case 3 小桶总数 (case3_groups << fanout_bits)。
//     result.big_buckets[b]      —— 大桶 b 的 {key_min, key_max, count}：
//       桶 0/1   (case 1/2) ：key_min/key_max 为常量值域，count 为扫描统计值。
//       桶 2..255 (case 3)  ：key_min/key_max 为归属该大桶的首/尾「非空小桶」值域
//                             边界，count 为归属该大桶所有小桶计数之和（空桶不计）。
//   extractor    —— 关键字提取器实例
// =============================================================================
template <
    typename CountType,
    typename KeyType,
    int      fanout_bits = radix_detail::radix_key_info<KeyType>::default_W,
    typename KeyExtractor,
    typename Iter
>
void count_for_radix(Iter begin, Iter end,
                     key_dist_result<CountType, KeyType, fanout_bits>& result,
                     KeyExtractor extractor)
{
    using info   = radix_detail::radix_key_info<KeyType>;
    using utype = typename info::unsigned_type;
    constexpr int bits   = info::bits;
    constexpr int GROUPS = info::case3_groups;

    static_assert(fanout_bits > 0 && fanout_bits < bits, "fanout_bits must satisfy 0 < fanout_bits < bits");
    static_assert(fanout_bits <= 16, "fanout_bits must be <= 16: case3 min h_bit is 16, shift = h_bit - fanout_bits must be >= 0");

    constexpr int POW2_8  = (1u << 8);    // 2^8  = 256
    constexpr int POW2_16 = (1u << 16);   // 2^16 = 65536

    // case 3 小桶总数：组数 × 2^fanout_bits（== key_dist_result::small_bucket_count）；constexpr 以便用作 array 尺寸
    constexpr std::size_t small_bucket_count = static_cast<std::size_t>(GROUPS) << fanout_bits;

    // 双层计数：
    //   counter  —— 冷数据，完整记录每个小桶的累计计数；体积大（可达数百 KB），
    //               故堆分配（std::vector）以规避栈溢出风险。
    //   lo_delta —— 热数据，记录每个小桶「最低字节的累计变化量」（0~255，满 256 进位到 counter）；
    //               体积小（设计上 ≤ L1）可常驻 L1，且尺寸编译期已知，用 std::array 省去堆分配。
    std::vector<CountType> counter(small_bucket_count, 0);
    std::array<uint8_t, small_bucket_count> lo_delta{};

    // case 1 / case 2 的元素数（大桶 0 / 1 的 count）
    CountType case1_count = 0;   // k < 2^8（含 k == 0）
    CountType case2_count = 0;   // 2^8 <= k < 2^16

    // ---------- 1. 扫描统计 ----------
#ifndef  USE_2WAY_LOOPUNROLL_IN_COUNT_FOR_RADIX

    for (auto it = begin; it != end; ++it)
    {
        const utype key = info::to_unsigned(extractor(*it));

        if (key < POW2_16)                              // case 1 (k<2^8) 或 case 2 (2^8<=k<2^16)
        {
            case1_count += (key < POW2_8);              // 无分支累加：k < 2^8（含 k == 0）
            case2_count += (key >= POW2_8);             //                2^8 <= k < 2^16
            continue;
        }

        const uint32_t hi_bit = radix_detail::highest_bit_position<utype>(key);  // 此时 key >= 2^16，hi_bit ∈ [16, bits-1]

        // case 3：取紧邻最高位之下的 fanout_bits 个比特作为组内偏移
        const int shift = hi_bit - fanout_bits;
        const uint32_t mask = (utype{1} << fanout_bits) - utype{1};
        const uint32_t j = ((hi_bit - 16) << fanout_bits) | ((key >> shift) & mask);

        // 绝大多数访问只更新热数据 lo_delta；每 256 次才更新一次冷数据 counter，
        // 从而把 counter 的 cache 失效摊薄到每 256 次一次。
        uint8_t old = lo_delta[j];
        lo_delta[j] = old + 1; // when old==255, lo_delta[j] will be 0
        if (old == 255)
            counter[j] += 256;
    }
#else
    auto process_one = [&](utype key) {
      if (key < POW2_16) {                       // case 1 / case 2
          case1_count += (key < POW2_8);
          case2_count += (key >= POW2_8);
          return;
      }
      const uint32_t hi_bit = radix_detail::highest_bit_position<utype>(key);  // key >= 2^16
      const int shift = hi_bit - fanout_bits;
      const uint32_t mask = (utype{1} << fanout_bits) - utype{1};
      const uint32_t j = ((hi_bit - 16) << fanout_bits) | ((key >> shift) & mask);
      uint8_t old = lo_delta[j];
      lo_delta[j] = old + 1;
      if (old == 255)
         counter[j] += 256;
    };

    const auto mid = begin + (std::distance(begin, end) & ~std::ptrdiff_t{1});  // 偶数对齐
    auto it=begin;
    for (; it != mid; it += 2) {
        utype key1 = info::to_unsigned(extractor(*it));
        utype key2 = info::to_unsigned(extractor(*(it+1)));
        if ( key1 < POW2_16 && key2 < POW2_16)
        {
            case1_count += (key1 < POW2_8);                // 无分支累加：k < 2^8（含 k == 0）
            case2_count += (key1 >= POW2_8);               //                2^8 <= k < 2^16
            case1_count += (key2 < POW2_8);                // 无分支累加：k < 2^8（含 k == 0）
            case2_count += (key2 >= POW2_8);
        }
        else if ( key1 >= POW2_16 && key2 >= POW2_16)
        {
            const utype mask = (utype{1} << fanout_bits) - utype{1};
            const uint32_t hi_bit1 = radix_detail::highest_bit_position<utype>(key1);  // 此时 key >= 2^16，hi_bit ∈ [16, bits-1]
            const uint32_t hi_bit2 = radix_detail::highest_bit_position<utype>(key2);  // 此时 key >= 2^16，hi_bit ∈ [16, bits-1]

            // case 3：取紧邻最高位之下的 fanout_bits 个比特作为组内偏移
            const int shift1 = hi_bit1 - fanout_bits;
            const int shift2 = hi_bit2 - fanout_bits;
            const uint32_t j1 = ((hi_bit1 - 16) << fanout_bits) | ((key1 >> shift1) & mask);
            const uint32_t j2 = ((hi_bit2 - 16) << fanout_bits) | ((key2 >> shift2) & mask);

            if ( j1 == j2 )
            {
                uint8_t old = lo_delta[j1];
                // if old==254, lo_delta[j1] will be 0
                // if old==255, lo_delta[j1] will be 1
                lo_delta[j1] = old + 2;
                if ( old >= 254 )
                    counter[j1] += 256;
            }
            else
            {
                uint8_t old1 = lo_delta[j1];
                uint8_t old2 = lo_delta[j2];
                lo_delta[j1] = old1 + 1;
                lo_delta[j2] = old2 + 1;
                if ( old1 == 255)
                    counter[j1] += 256;
                if ( old2 == 255)
                    counter[j2] += 256;
            }
        }
        else
        {
            if (key1 > key2)
                std::swap(key1, key2);
            case1_count += (key1 < POW2_8);      // 无分支累加：k < 2^8（含 k == 0）
            case2_count += (key1 >= POW2_8);
            process_one(key2);
        }
    }
    if (it != end)
    {
        const utype key = info::to_unsigned(extractor(*it));
        process_one(key);
    }

#endif

    // ---------- 2. 合并 lo_delta -> counter，得到每个小桶的最终计数 ----------
    for (std::size_t j = 0; j < small_bucket_count; ++j)
        counter[j] += lo_delta[j];

    // 桶 0 = case 1：值域 [0, 2^8)
    result.big_buckets[0].key_min = utype{0};
    result.big_buckets[0].key_max = POW2_8 - utype{1};
    result.big_buckets[0].count   = case1_count;

    // 桶 1 = case 2：值域 [2^8, 2^16)
    result.big_buckets[1].key_min = POW2_8;
    result.big_buckets[1].key_max = POW2_16 - utype{1};
    result.big_buckets[1].count   = case2_count;

    // ---------- 3. case 3 元素总数 case3_total ----------
    CountType case3_total = 0;
    for (std::size_t j = 0; j < small_bucket_count; ++j)
        case3_total += counter[j];
    // 期望值 target = case3_total / 254 ； 平均每个大桶元素的个数
    const double target = (double)case3_total / 254.00;

    // ---------- 4. 贪心合并：case 3 小桶 -> 大桶 [2, 255]
    // 写入 result.small_bucket_map[] 和 result.big_buckets[] ----------
    // 初始化  result.big_buckets[cur_bucket].count
    for (int cur_bucket = 2; cur_bucket <= 255; cur_bucket++)
        result.big_buckets[cur_bucket].count = 0;

    int         cur_bucket = 2;     // 当前大桶索引
    CountType   total      = 0;     // 已 确定的 大桶的元素个数的累加和
    CountType   acc        = 0;     // 当前大桶累计元素数

    //当前，预期的多个大桶元素个数的累加和,total+acc ~= expected_acc
    CountType expected_acc = (CountType)(target * (cur_bucket - 1));
    bool found_non_empty = false;

    for ( std::size_t group = 0 ; group < GROUPS; group++)
    {
        const utype group_base = utype{1} << (group + 16);
        const int shift = group + 16 - fanout_bits;

        for ( std::size_t i = 0; i < (1<<fanout_bits); ++i)
        {
            std::size_t j = (group << fanout_bits) + i;  // j: 小桶索引（group 为组号）
            const utype key_min = group_base + (static_cast<utype>(i) << shift);

            result.small_bucket_map[j] = static_cast<uint8_t>(cur_bucket);  // 设置小桶对应的大桶
            acc += counter[j];

            // find the first non-empty small bucket
            if ( !found_non_empty && counter[j] > 0 )
            {
                found_non_empty = true;
                result.big_buckets[cur_bucket].key_min = key_min;
            }

            if ( counter[j] > 0)
            {
                result.big_buckets[cur_bucket].key_max = group_base + (static_cast<utype>(i + 1) << shift) - 1;
                result.big_buckets[cur_bucket].count = acc;
            }

            if (cur_bucket < 255 )
            {
                // 更新求下一个大桶数据用到的变量
                if (acc + total >= expected_acc )
                {
                    total += acc;
                    cur_bucket++;
                    acc = 0;
                    expected_acc = (CountType)(target * (cur_bucket - 1));
                    found_non_empty = false;
                }
            }
        }
    }
}

// =============================================================================
// count_for_radix（便利重载：KeyExtractor 默认构造，适用于无状态提取器）
// =============================================================================
template <
    typename CountType,
    typename KeyType,
    int      fanout_bits = radix_detail::radix_key_info<KeyType>::default_W,
    typename KeyExtractor,
    typename Iter
>
void count_for_radix(Iter begin, Iter end,
                     key_dist_result<CountType, KeyType, fanout_bits>& result)
{
    count_for_radix<CountType, KeyType, fanout_bits, KeyExtractor, Iter>(
        begin, end, result, KeyExtractor{});
}
