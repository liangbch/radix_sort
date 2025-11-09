#include "radix_sort.hpp"
#include<algorithm>

struct mystruct
{
    double key;
    size_t value;
};

template <>
struct radix_trait<mystruct>
{
	static constexpr std::size_t radix_size = sizeof(double) / sizeof(unsigned char);
	template <size_t index>
	static unsigned char get(const mystruct& obj) noexcept
	{
		static_assert(index < radix_size);
		return ((unsigned char*)&(obj.key))[index];
	}
};


struct my_trait
{
	static constexpr std::size_t radix_size = sizeof(std::pair<int,int>) / sizeof(unsigned char);
	template <size_t index>
	static unsigned char get(const std::pair<int, int>& obj) noexcept
	{
		static_assert(index < radix_size);
		if constexpr (index < sizeof(int))
			return radix_trait<int>::template get<index>(obj.second);
		else
			return radix_trait<int>::template get<index - sizeof(int)>(obj.first);
	}
};


int main()
{
    {
        std::vector<int> ar={2,3,1};
        radix_sort(ar.begin(),ar.end());
        //support signed number
    }
    {
        std::vector<unsigned short> ar={2,3,1};
        radix_sort(ar.begin(),ar.end());
        //support unsigned number
    }
    {
        std::vector<size_t> ar={2,3,1};
        radix_sort<radix_trait_greater<unsigned short>>(ar.begin(),ar.end());
        //descending order
    }
    {
        std::vector<float> ar={1.0f,2.4f,-3.5f};
        radix_sort(ar.begin(),ar.end());
        //support floating point
    }
    {
        std::vector<std::pair<int,int>> ar={{2,3},{0,1},{5,4}};
        radix_sort(ar.begin(),ar.end());
        //support std::pair
    }
    {
        std::vector<mystruct> ar={{1.0,2},{-1.4,123},{-1.4,0}};
        radix_sort(ar.begin(),ar.end());
        //support user-defined struct
    }
    {
        std::vector<std::pair<int,int>> ar={{2,3},{0,1},{5,4}};
        radix_sort<my_trait>(ar.begin(),ar.end());
        //support user-defined sort-way
        //first key ascending order,second key descending order
    }
    {
        unsigned char buf[sizeof(int)*5];
        std::vector<int> ar={5,3,2,6,3};
        radix_sort(ar.begin(),ar.end(),buf);
        //support user-supplied buffer
    }
    {
        std::vector<int> ar{3,5,1,3,6};
        radix_sort(ar.begin(),ar.end(),std::execution::par);
        //multi-threads parallel sorting
    }
    return 0;
}
