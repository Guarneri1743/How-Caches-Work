#include <iostream>
#include <chrono>
#include <thread>
#include <string>

using namespace std;

#define KB << 10
#define MB << 20
#define CACHE_LINE_SIZE 64

class Stopwatch
{
public:
    std::chrono::steady_clock::time_point start_time;

public:
    Stopwatch()
    {
        start();
    }
    void start()
    {
        start_time = std::chrono::steady_clock::now();
    }
    long long elapsed()
    {
        auto end = std::chrono::steady_clock::now();
        return  std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_time).count();
    }
};

string get_size_str(int length)
{
    if (length < 1 KB)
    {
        return std::to_string(length) + " B";
    }
    else if (length >= 1 KB && length < 1 MB)
    {
        return std::to_string((float)length / 1024.0f) + " KB";
    }
    else if (length >= 1 MB)
    {
        return std::to_string((float)length / (1024.0f * 1024.0f)) + " MB";
    }
    return std::to_string(length);
}

/// <summary>
/// element access does not affect performance a lot, on the contrary, memory access does.
/// </summary>
void cacheline()
{
    cout << "cacheline: " << endl;
    for (int step = 0; step <= 10; step++)
    {
        int size = 64 MB;
        int* buffer = new int[size];
        std::fill(buffer, buffer + size, 1);
        Stopwatch sw;
        for (int i = 0; i < size; i += (1 << step))
        {
            buffer[i]++;
        }
        auto elapsed = sw.elapsed();
        cout << "elapsed: " << elapsed << " ns" << ", step: " << ((1 << step) * sizeof(int)) << " bytes" << endl;
        delete[] buffer;
    }
    cout << endl;
}

int get_nice_step(int size)
{
    if (size >= 1 KB && size < 32 KB)
    {
        return 1 KB;
    }
    else if (size >= 32 KB && size < 256 KB)
    {
        return 16 KB;
    }
    else if (size >= 256 KB && size < 1 MB)
    {
        return 128 KB;
    }
    else if (size >= 1 MB && size < 12 MB)
    {
        return 1 MB;
    }
    else if (size >= 12 MB)
    {
        return 4 MB;
    }
    return 8 MB;
}

/// <summary>
/// multi-level caches
/// use cpu-z to get the cache size of your cpu
/// 3 dramatic increases of cost will be observed at L1 size, L2 size and L3 size respectively.
/// </summary>
void multi_level_caches_foreach(int buffer_size)
{
    int array_size = buffer_size / sizeof(int);
    int access_count = 64 * 1024 * 1024;
    int mod = array_size - 1;

    int* buffer = new int[array_size];

    Stopwatch sw;
    for (int i = 0; i < access_count; ++i)
    {
        buffer[(i << 2) & mod]++;
    }
    auto elapsed = sw.elapsed();
    cout << "elapsed: " << elapsed << " ns" << ", buffer size: " << get_size_str(array_size * sizeof(int)) << endl;
    delete[] buffer;
}

void multi_level_caches()
{
    cout << "multi-level caches: " << endl;
    for (int i = 0; i <= 16; ++i)
    {
        multi_level_caches_foreach((1 << i) * 1024);
    }
    cout << endl;
}

/// <summary>
/// mordern cpu has the ability to access multiple memory localtions or execute multiple instructions simultaneously (only if there are no dependencies among the memory locations/instructions).
/// </summary>
void instruction_level_parallelism()
{
    cout << "instruction level parallelism: " << endl;
    int count = 1 MB;
    int* buffer1 = new int[2]{ 0 };
    Stopwatch sw1;
    for (int i = 0; i < count; ++i)
    {
        buffer1[0]++;
        buffer1[0]++;
    }
    auto elapsed1 = sw1.elapsed();
    cout << "case1: " << elapsed1 << " ns" << endl;
    delete[] buffer1;
    int* buffer2 = new int[2]{ 0 };
    Stopwatch sw2;
    for (int i = 0; i < count; ++i)
    {
        buffer2[0]++;
        buffer2[1]++;
    }
    auto elapsed2 = sw2.elapsed();
    cout << "case2: " << elapsed2 << " ns" << endl;
    delete[] buffer2;
    int* buffer3 = new int[2]{ 0 };
    Stopwatch sw3;
    for (int i = 0; i < count; ++i)
    {
        buffer3[0] += 2;
    }
    auto elapsed3 = sw3.elapsed();
    cout << "case3: " << elapsed3 << " ns" << endl;
    delete[] buffer3;
    cout << endl;
}

/// <summary>
/// cache size = N-way * M-set * cacheline size
/// thus, M = cache size / (N * cacheline size)
/// 
/// memory->cache mapping:
/// cache set index = memory block address % M
/// 
/// it's easy to see that accessing memory with POT step is more likely to thrash the same cache set.
/// that's why the steps like 256, 512, 2048, 4096, 8192 are much more costly than other steps.
/// </summary>
void cache_associativity(int length, int step)
{
    char* buffer = new char[length];
    std::fill(buffer, buffer + length, 0);
    const int count = 64 MB;
    Stopwatch sw;
    for (int i = 0, p = 0; i < count; i++)
    {
        buffer[p]++;
        p += step;
        if (p >= length) p = 0;
    }
    auto elapsed = sw.elapsed();
    cout << "elapsed: " << elapsed << " ns, " << get_size_str(length) << ", step : " << step << endl;
    delete[] buffer;
}

void cache_associativity()
{
    cout << "cache associativity: " << endl;
    cache_associativity(128 * 1024 * 1024, 1);
    cache_associativity(128 * 1024 * 1024, 31);
    cache_associativity(128 * 1024 * 1024, 32);
    cache_associativity(128 * 1024 * 1024, 63);
    cache_associativity(128 * 1024 * 1024, 64);
    cache_associativity(128 * 1024 * 1024, 256);
    cache_associativity(128 * 1024 * 1024, 257);
    cache_associativity(128 * 1024 * 1024, 512);
    cache_associativity(128 * 1024 * 1024, 576);
    cache_associativity(128 * 1024 * 1024, 4096);
    cache_associativity(128 * 1024 * 1024, 4097);
    cache_associativity(128 * 1024 * 1024, 8192);
    cache_associativity(128 * 1024 * 1024, 8193);
    cout << endl;
}

void write_buffer(int* buffer, int pos)
{
    for (int i = 0; i < 1 MB; i++)
    {
        buffer[pos]++;
    }
}

/// <summary>
/// false sharing
/// hence every core has its own L1/L2 cache, cache coherence becomes a problem in multi-thread environment.
/// the entire cacheline will be invalidated or updated while multiple threads are writing to the same position.
/// </summary>
void false_sharing(int pos1, int pos2, int pos3, int pos4)
{
    int* buffer = new int[1024];
    thread t1(write_buffer, buffer, pos1);
    thread t2(write_buffer, buffer, pos2);
    thread t3(write_buffer, buffer, pos3);
    thread t4(write_buffer, buffer, pos4);
    Stopwatch sw;
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    auto elapsed = sw.elapsed();
    cout << "steps: " << pos1 << ", " << pos2 << ", " << pos3 << ", " << pos4 << ": " << "elapsed " << elapsed << " ns" << endl;
    delete[] buffer;
}

void false_sharing()
{
    cout << "false sharing: " << endl;
    false_sharing(0, 1, 2, 3);
    false_sharing(16, 32, 48, 64);
}

int main()
{
    cacheline();
    multi_level_caches();
    instruction_level_parallelism();
    cache_associativity();
    false_sharing();
    return 0;
}
