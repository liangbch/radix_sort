#include <cctype>
#include <algorithm>
#include <iostream>
#include <chrono>
//#include <format>
#include <random>

#include "radix_sort.hpp"

template<typename p1, typename p2, typename Op>
void test_pair(size_t test_size, size_t test_round, Op op, std::string sort_name)
{
	using namespace std;
	uniform_int_distribution<p1> rnd1;
	uniform_int_distribution<p2> rnd2;
	int used_time = 0;
	vector<pair<p1, p2>> ar;
	ar.resize(test_size + 1);
	for (size_t i = 0; i < test_round; ++i)
	{
		{
			minstd_rand e(114514);
			for (size_t i = 0; i < test_size; ++i)
				ar[i] = { rnd1(e),rnd2(e) };
		}
		auto beg = chrono::high_resolution_clock::now();
		op(ar.begin(), ar.end());
		used_time += (chrono::high_resolution_clock::now() - beg) / 1ms;
	}
	cout << "|" << sort_name << "|" << used_time / test_round << "ms" << "|\n";
}

template<typename T, typename Op>
void test(size_t test_size, size_t test_round, Op op, std::string sort_name)
{
	using namespace std;
	uniform_int_distribution<T> rnd;
	int used_time = 0;
	vector<T> ar;
	ar.resize(test_size + 1);
	for (size_t i = 0; i < test_round; ++i)
	{
		{
			minstd_rand e(114514);
			for (size_t i = 0; i < test_size; ++i)
				ar[i] = rnd(e);
		}
		auto beg = chrono::high_resolution_clock::now();
		op(ar.begin(), ar.end());
		used_time += (chrono::high_resolution_clock::now() - beg) / 1ms;
	}
	cout << "|" << sort_name << "|" << used_time / test_round << "ms" << "|\n";
}

template<typename p1, typename p2>
struct one_key
{
	static constexpr std::size_t radix_size = sizeof(p1) / sizeof(unsigned char);
	template <size_t index>
	static unsigned char get(const std::pair<p1, p2>& obj) noexcept
	{
		static_assert(index < radix_size);
		return radix_trait<p1>::template get<index>(obj.first);
	}
};

int main()
{
	using namespace std;
	int test_round ;
	for (auto test_size : { 1e6,1e7,1e8 })
	{
		if (test_size >= 1e9)
			test_round=1;
		else if (test_size >= 1e8)
			test_round=3;
		else
			test_round=5;

		cout << "\nRun sort on int vector with " << test_size << " elements: " << endl;
			
		test<int>(test_size, test_round, [](const auto& a, const auto& b) {sort(a, b); }, "std::sort");
		test<int>(test_size, test_round, [](const auto& a, const auto& b) {stable_sort(a, b); }, "std::stable_sort");
		test<int>(test_size, test_round, [](const auto& a, const auto& b) {radix_sort(a, b); }, "radix_sort");

		test<int>(test_size, test_round, [](const auto& a, const auto& b) {sort(execution::par, a, b); }, "std::sort(par)");
		test<int>(test_size, test_round, [](const auto& a, const auto& b) {stable_sort(execution::par, a, b); }, "std::stable_sort(par)");
		test<int>(test_size, test_round, [](const auto& a, const auto& b) {radix_sort(a, b, execution::par); }, "radix_sort(par)");

		cout << "\nRun sort on size_t vector with " << test_size << " elements: " << endl;
		test<size_t>(test_size, test_round, [](const auto& a, const auto& b) {sort(a, b); }, "std::sort");
		test<size_t>(test_size, test_round, [](const auto& a, const auto& b) {stable_sort(a, b); }, "std::stable_sort");
		test<size_t>(test_size, test_round, [](const auto& a, const auto& b) {radix_sort(a, b); }, "radix_sort");

		test<size_t>(test_size, test_round, [](const auto& a, const auto& b) {sort(execution::par, a, b); }, "std::sort(par)");
		test<size_t>(test_size, test_round, [](const auto& a, const auto& b) {stable_sort(execution::par, a, b); }, "std::stable_sort(par)");
		test<size_t>(test_size, test_round, [](const auto& a, const auto& b) {radix_sort(a, b, execution::par); }, "radix_sort(par)");

		cout << "\nRun sort on pair<int,uint> vector with " << test_size << " elements: " << endl;

		test_pair<int, unsigned int>(test_size, test_round, [](const auto& a, const auto& b) {sort(a, b); }, "std::sort");
		test_pair<int, unsigned int>(test_size, test_round, [](const auto& a, const auto& b) {stable_sort(a, b); }, "std::stable_sort");
		test_pair<int, unsigned int>(test_size, test_round, [](const auto& a, const auto& b) {radix_sort(a, b); }, "radix_sort");

		test_pair<int, unsigned int>(test_size, test_round, [](const auto& a, const auto& b) {sort(execution::par, a, b); }, "std::sort(par)");
		test_pair<int, unsigned int>(test_size, test_round, [](const auto& a, const auto& b) {stable_sort(execution::par, a, b); }, "std::stable_sort(par)");
		test_pair<int, unsigned int>(test_size, test_round, [](const auto& a, const auto& b) {radix_sort(a, b, execution::par); }, "radix_sort(par)");

		cout << "\nRun sort on pair<size_t,size_t> vector with " << test_size << " elements: " << endl;

		test_pair<size_t, size_t>(test_size, test_round, [](const auto& a, const auto& b) {sort(a, b, [](const auto& a, const auto& b) {return a.first < b.first; }); }, "std::sort");
		test_pair<size_t, size_t>(test_size, test_round, [](const auto& a, const auto& b) {stable_sort(a, b, [](const auto& a, const auto& b) { return a.first < b.first; }); }, "std::stable_sort");
		test_pair<size_t, size_t>(test_size, test_round, [](const auto& a, const auto& b) {radix_sort<one_key<size_t, size_t>>(a, b); }, "radix_sort");

		test_pair<size_t, size_t>(test_size, test_round, [](const auto& a, const auto& b) {sort(execution::par, a, b, [](const auto& a, const auto& b) {return a.first < b.first; }); }, "std::sort(par)");
		test_pair<size_t, size_t>(test_size, test_round, [](const auto& a, const auto& b) {stable_sort(execution::par, a, b, [](const auto& a, const auto& b) { return a.first < b.first; }); }, "std::stable_sort(par)");
		test_pair<size_t, size_t>(test_size, test_round, [](const auto& a, const auto& b) {radix_sort<one_key<size_t, size_t>>(a, b, execution::par); }, "radix_sort(par)");
	
}


	return 0;
}
