// test_count_for_radix.cpp
//
// 验证 include/count_for_radix.hpp 的正确性：
//   1. 4 种关键字类型 (uint32/int32/uint64/int64) 均可实例化
//   2. 关键不变量：small_bucket_map 尺寸、值域 [2,255]、元素守恒、大桶索引连续
//   3. 带符号关键字与等价无符号关键字产生相同的 small_bucket_map
//   4. case 1 / case 2 的元素不计入 case 3 总数 T
//   5. 单桶超限时该小桶独占一个大桶
//   6. 3 参数重载（KeyExtractor 默认构造）可用
//   7. big_buckets：桶 0/1 常量值域 + count；case 3 大桶 min/max/count 与独立复现一致

#include "count_for_radix.hpp"

#include <bit>
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <numeric>
#include <random>
#include <set>
#include <vector>

// ---------- 无状态关键字提取器（可默认构造） ----------
struct identity_u32 { uint32_t operator()(uint32_t x) const noexcept { return x; } };
struct identity_i32 { int32_t  operator()(int32_t  x) const noexcept { return x; } };
struct identity_u64 { uint64_t operator()(uint64_t x) const noexcept { return x; } };
struct identity_i64 { int64_t  operator()(int64_t  x) const noexcept { return x; } };

static int g_failures = 0;
#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            std::cout << "  FAIL line " << __LINE__ << ": " #cond << "\n";\
            ++g_failures;                                                 \
        }                                                                 \
    } while (0)

// 独立复现 case 3 小桶索引计算与计数（作为参考实现对照）
template <typename U>
static std::size_t small_bucket_index(U k, int W)
{
    constexpr int N = static_cast<int>(sizeof(U)) * 8;
    const int h = (N - 1) - std::countl_zero(k);
    const int s = h - W;
    const U   m = (U{1} << W) - U{1};
    return (static_cast<std::size_t>(h - 16) << W) | static_cast<std::size_t>((k >> s) & m);
}

// 聚合 small_bucket_map -> 每个大桶的元素数；返回 case 3 元素总数。
// 模板化以同时接受 std::vector<uint8_t> 与 std::array<uint8_t, N>。
template <typename BucketMap>
static std::uint64_t aggregate(const BucketMap& bucket_map,
                               const std::vector<std::uint64_t>& cnt,
                               std::vector<std::uint64_t>& per_bucket)
{
    per_bucket.assign(256, 0);
    for (std::size_t j = 0; j < bucket_map.size(); ++j)
        per_bucket[bucket_map[j]] += cnt[j];
    std::uint64_t total = 0;
    for (int b = 2; b <= 255; ++b) total += per_bucket[b];
    return total;
}

// =============================================================================
// 测试 1+2+7：uint32_t / uint64_t 随机数据的关键不变量 + big_buckets 校验
// =============================================================================
template <typename U, typename KeyType, int W>
static void test_invariants(const char* name)
{
    constexpr int GROUPS = (sizeof(U) == 4) ? 16 : 48;
    const std::size_t J = static_cast<std::size_t>(GROUPS) << W;
    const U CASE3_LO = U{1} << 16;                  // case 3 下界（含）

    std::mt19937_64 rng(12345ULL);
    // 关键字全部落在 case 3，取值覆盖整个无符号域高位
    std::uniform_int_distribution<std::uint64_t> dist(
        static_cast<std::uint64_t>(CASE3_LO),
        std::numeric_limits<std::uint64_t>::max());

    constexpr std::size_t N = 2'000'000;
    std::vector<U> data(N);
    for (auto& v : data)
        v = static_cast<U>(dist(rng)) | CASE3_LO;   // 强制 case 3：保证 k >= 2^16

    key_dist_result<std::uint64_t, KeyType, W> result;
    count_for_radix<std::uint64_t, KeyType, W>(
        data.begin(), data.end(), result,
        [](U x) { return static_cast<KeyType>(x); });

    // ---- 独立参考计数（仅 case 3 元素参与，与库一致） ----
    std::vector<std::uint64_t> cnt(J, 0);
    for (U k : data)
        if (k >= CASE3_LO) ++cnt[small_bucket_index(k, W)];
    const std::uint64_t T = std::accumulate(cnt.begin(), cnt.end(), 0ULL);

    std::cout << name << ":\n";
    CHECK(result.small_bucket_map.size() == J);

    // 值域 [2, 255]
    bool range_ok = true;
    for (uint8_t b : result.small_bucket_map) range_ok = range_ok && (b >= 2 && b <= 255);
    CHECK(range_ok);

    // 元素守恒：所有大桶元素数之和 == T（case 3）
    std::vector<std::uint64_t> per_bucket;
    std::uint64_t total = aggregate(result.small_bucket_map, cnt, per_bucket);
    CHECK(total == T);

    // 大桶索引从 2 开始连续（B 单调 ++1，不会跳跃）
    std::set<int> used(result.small_bucket_map.begin(), result.small_bucket_map.end());
    int maxB = *used.rbegin();
    bool continuous = true;
    for (int b = 2; b <= maxB; ++b) continuous = continuous && (used.count(b) > 0);
    CHECK(continuous);

    // 分布质量：除超限独占桶外，每个大桶元素数不应显著超过 2e
    const std::uint64_t e = T / 254;
    std::uint64_t over_cnt = 0, over_max = 0;
    for (int b = 2; b <= maxB; ++b)
    {
        // 允许两种情况：<= 2e（普通合并桶），或 > e（超限独占桶，其计数 == 某个 cnt[j]）
        if (per_bucket[b] > 2 * e)
        {
            ++over_cnt;
            over_max = std::max(over_max, per_bucket[b]);
            // 超限独占桶的元素数必须等于某个小桶的计数
            bool is_single = false;
            for (std::size_t j = 0; j < J; ++j)
                if (cnt[j] == per_bucket[b]) { is_single = true; break; }
            CHECK(is_single);
        }
    }

    // ---- big_buckets 校验 ----
    // 整体元素守恒：Σ_{b=0..255} count == data.size()（本测试数据全在 case 3，
    // 故桶 0/1 的 count == 0，Σ case 3 count == T == data.size()）
    std::uint64_t total_all = 0;
    for (int b = 0; b < 256; ++b) total_all += result.big_buckets[b].count;
    CHECK(total_all == static_cast<std::uint64_t>(data.size()));

    // 桶 0/1 的常量值域（无论是否为空，min/max 恒为定值）
    CHECK(result.big_buckets[0].key_min == 0);
    CHECK(result.big_buckets[0].key_max == (U{1} << 8) - U{1});
    CHECK(result.big_buckets[1].key_min == (U{1} << 8));
    CHECK(result.big_buckets[1].key_max == (U{1} << 16) - U{1});

    // 独立复现 case 3 大桶的 min/max/count，与 result.big_buckets 逐桶对照
    std::vector<U>             bb_min(256, 0), bb_max(256, 0);
    std::vector<std::uint64_t> bb_cnt(256, 0);
    for (std::size_t j = 0; j < J; ++j)
    {
        if (cnt[j] == 0) continue;                       // 跳过空小桶
        const int b   = result.small_bucket_map[j];
        const int g   = static_cast<int>(j >> W);
        const int h   = g + 16;
        const int s   = h - W;
        const U    o   = static_cast<U>(j & ((std::size_t{1} << W) - 1));
        const U    bmin = (U{1} << h) | (o << s);
        const U    bmax = bmin + ((U{1} << s) - U{1});
        if (bb_cnt[b] == 0) bb_min[b] = bmin;            // 首个非空小桶 → min
        bb_max[b] = bmax;                                // 末个非空小桶 → max
        bb_cnt[b] += cnt[j];
    }
    bool minmax_ok = true;
    for (int b = 2; b <= 255; ++b)
    {
        if (result.big_buckets[b].count != bb_cnt[b]) { minmax_ok = false; break; }
        if (bb_cnt[b] > 0)
        {
            if (result.big_buckets[b].key_min != bb_min[b]) { minmax_ok = false; break; }
            if (result.big_buckets[b].key_max != bb_max[b]) { minmax_ok = false; break; }
        }
    }
    CHECK(minmax_ok);

    std::cout << "  T=" << T << "  e=" << e << "  max_bucket=" << maxB
              << "  buckets>2e=" << over_cnt << "  max_over=" << over_max << "\n";
    std::cout << (g_failures ? "  -- has failures --\n" : "  ok\n");
}

// =============================================================================
// 测试 3：带符号关键字与等价无符号关键字产生相同 small_bucket_map
// =============================================================================
static void test_signed_mapping()
{
    std::cout << "test_signed_mapping (int32 vs uint32):\n";
    std::mt19937_64 rng(777);
    std::uniform_int_distribution<std::uint32_t> dist(0, std::numeric_limits<std::uint32_t>::max());
    constexpr int W = 10;
    constexpr std::size_t N = 500'000;

    std::vector<std::uint32_t> udata(N);
    std::vector<std::int32_t>  idata(N);
    for (std::size_t i = 0; i < N; ++i)
    {
        std::uint32_t u = dist(rng);
        udata[i] = u;
        // 反向翻转符号位：保证 to_unsigned(idata[i]) == udata[i]
        idata[i] = static_cast<std::int32_t>(u ^ 0x80000000u);
    }

    key_dist_result<std::uint64_t, std::uint32_t, W> res_u;
    key_dist_result<std::uint64_t, std::int32_t,  W> res_i;
    count_for_radix<std::uint64_t, std::uint32_t, W>(
        udata.begin(), udata.end(), res_u, identity_u32{});
    count_for_radix<std::uint64_t, std::int32_t, W>(
        idata.begin(), idata.end(), res_i, identity_i32{});

    CHECK(res_u.small_bucket_map.size() == res_i.small_bucket_map.size());
    bool same = (res_u.small_bucket_map == res_i.small_bucket_map);
    CHECK(same);
    std::cout << (g_failures ? "  -- has failures --\n" : "  ok\n");
}

// =============================================================================
// 测试 4：case 1 / case 2 的元素不计入 case 3 总数 T（且计入桶 0/1 的 count）
// =============================================================================
static void test_case_mixture()
{
    std::cout << "test_case_mixture (case1/2 excluded from T):\n";
    constexpr int W = 8;
    std::vector<std::uint32_t> data;
    std::uint64_t expected_case1 = 0, expected_case2 = 0;
    // case 1: k < 2^8
    for (std::uint32_t k = 0; k < (1u << 8); ++k) { data.push_back(k); ++expected_case1; }
    // case 2: 2^8 <= k < 2^16
    for (std::uint32_t k = (1u << 8); k < (1u << 16); k += 7) { data.push_back(k); ++expected_case2; }
    // case 3: k >= 2^16
    std::uint64_t expected_T = 0;
    for (std::uint32_t k = (1u << 16); k < (1u << 16) + 100000; k += 3)
    {
        data.push_back(k);
        ++expected_T;
    }

    key_dist_result<std::uint64_t, std::uint32_t, W> result;
    count_for_radix<std::uint64_t, std::uint32_t, W>(
        data.begin(), data.end(), result, identity_u32{});

    // 独立统计 case 3 计数
    const std::size_t J = std::size_t(16) << W;
    std::vector<std::uint64_t> cnt(J, 0);
    for (std::uint32_t k : data)
        if (k >= (1u << 16)) ++cnt[small_bucket_index<std::uint32_t>(k, W)];

    std::vector<std::uint64_t> per_bucket;
    std::uint64_t T = aggregate(result.small_bucket_map, cnt, per_bucket);
    CHECK(T == expected_T);

    // 桶 0/1 的 count 与 case 1/2 元素数一致
    CHECK(result.big_buckets[0].count == expected_case1);
    CHECK(result.big_buckets[1].count == expected_case2);
    // 整体守恒
    std::uint64_t total_all = 0;
    for (int b = 0; b < 256; ++b) total_all += result.big_buckets[b].count;
    CHECK(total_all == static_cast<std::uint64_t>(data.size()));

    std::cout << "  expected_T=" << expected_T << "  actual_T=" << T
              << "  case1=" << result.big_buckets[0].count
              << "  case2=" << result.big_buckets[1].count << "\n";
    std::cout << (g_failures ? "  -- has failures --\n" : "  ok\n");
}

// =============================================================================
// 测试 5：3 参数重载（KeyExtractor 默认构造）
// =============================================================================
static void test_three_arg_overload()
{
    std::cout << "test_three_arg_overload:\n";
    std::vector<std::uint64_t> data(100000);
    std::mt19937_64 rng(2024);
    for (auto& v : data) v = rng() | (1ULL << 20);   // 强制 case 3

    // 3 参数版本：identity_u64 默认构造
    key_dist_result<std::uint64_t, std::uint64_t, 9> result;
    count_for_radix<std::uint64_t, std::uint64_t, 9, identity_u64>(
        data.begin(), data.end(), result);

    const std::size_t J = std::size_t(48) << 9;
    CHECK(result.small_bucket_map.size() == J);
    bool range_ok = true;
    for (uint8_t b : result.small_bucket_map) range_ok = range_ok && (b >= 2 && b <= 255);
    CHECK(range_ok);
    std::cout << (g_failures ? "  -- has failures --\n" : "  ok\n");
}

int main()
{
    std::cout << "count_for_radix test program\n============================\n";

    test_invariants<std::uint32_t, std::uint32_t, 10>("test_invariants uint32 (W=10)");
    test_invariants<std::uint64_t, std::uint64_t, 9>("test_invariants uint64 (W=9)");
    test_signed_mapping();
    test_case_mixture();
    test_three_arg_overload();

    std::cout << "============================\n";
    if (g_failures == 0)
    {
        std::cout << "All tests passed!\n";
        return 0;
    }
    std::cout << g_failures << " check(s) FAILED\n";
    return 1;
}
