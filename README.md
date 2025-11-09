# radix_sort
The source code is base on  poor-circle/radix_sort and
1. Add CMakeLists.txt for build in Linux
2. Fix some compile issue on gcc

A high performence Cross-platform (parallel) STL-like LSD radix sort algorithm by C++20, 2.5-14.3x faster than std::sort, support user-defined struct.

## build in Linux

```bash
mkdir build
cd build
cmake ..
make
```

## Usage

```cpp
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
    radix_sort<radix_trait_greater<size_t>>(ar.begin(),ar.end());
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
    std::vector<void*> ar={0x300000,0x0,0x60000000};
    radix_sort(ar.begin(),ar.end());
    //support T*
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
```

## benchmark

Running on Xeon(R) CPU E5-2670@ 2.60GHz and DDR3 1600 MHz, the results are as follows.
```

Run sort on int vector with 1e+06 elements:
|std::sort|70ms|
|std::stable_sort|81ms|
|radix_sort|12ms|
|std::sort(par)|7ms|
|std::stable_sort(par)|7ms|
|radix_sort(par)|4ms|

Run sort on size_t vector with 1e+06 elements:
|std::sort|68ms|
|std::stable_sort|83ms|
|radix_sort|32ms|
|std::sort(par)|7ms|
|std::stable_sort(par)|8ms|
|radix_sort(par)|12ms|

Run sort on pair<int,uint> vector with 1e+06 elements:
|std::sort|87ms|
|std::stable_sort|92ms|
|radix_sort|34ms|
|std::sort(par)|8ms|
|std::stable_sort(par)|9ms|
|radix_sort(par)|11ms|

Run sort on pair<size_t,size_t> vector with 1e+06 elements:
|std::sort|75ms|
|std::stable_sort|93ms|
|radix_sort|62ms|
|std::sort(par)|16ms|
|std::stable_sort(par)|15ms|
|radix_sort(par)|30ms|

Run sort on int vector with 1e+07 elements:
|std::sort|805ms|
|std::stable_sort|977ms|
|radix_sort|142ms|
|std::sort(par)|103ms|
|std::stable_sort(par)|110ms|
|radix_sort(par)|53ms|

Run sort on size_t vector with 1e+07 elements:
|std::sort|786ms|
|std::stable_sort|1032ms|
|radix_sort|389ms|
|std::sort(par)|157ms|
|std::stable_sort(par)|166ms|
|radix_sort(par)|182ms|

Run sort on pair<int,uint> vector with 1e+07 elements:
|std::sort|1003ms|
|std::stable_sort|1147ms|
|radix_sort|418ms|
|std::sort(par)|170ms|
|std::stable_sort(par)|166ms|
|radix_sort(par)|179ms|

Run sort on pair<size_t,size_t> vector with 1e+07 elements:
|std::sort|878ms|
|std::stable_sort|1172ms|
|radix_sort|694ms|
|std::sort(par)|293ms|
|std::stable_sort(par)|298ms|
|radix_sort(par)|352ms|

Run sort on int vector with 1e+08 elements:
|std::sort|9157ms|
|std::stable_sort|11361ms|
|radix_sort|1434ms|
|std::sort(par)|1232ms|
|std::stable_sort(par)|1246ms|
|radix_sort(par)|551ms|

Run sort on size_t vector with 1e+08 elements:
|std::sort|9023ms|
|std::stable_sort|11968ms|
|radix_sort|4044ms|
|std::sort(par)|1793ms|
|std::stable_sort(par)|1831ms|
|radix_sort(par)|1746ms|

Run sort on pair<int,uint> vector with 1e+08 elements:
|std::sort|11456ms|
|std::stable_sort|13263ms|
|radix_sort|4225ms|
|std::sort(par)|1897ms|
|std::stable_sort(par)|1928ms|
|radix_sort(par)|1764ms|

Run sort on pair<size_t,size_t> vector with 1e+08 elements:
|std::sort|10157ms|
|std::stable_sort|14203ms|
|radix_sort|7855ms|
|std::sort(par)|4659ms|
|std::stable_sort(par)|5066ms|
|radix_sort(par)|3541ms|

```
