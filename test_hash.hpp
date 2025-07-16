#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

#include "CRC32.hpp"

inline std::mutex m;

template <typename T>
void Calculation(T begin, T end, uint threadNumber) {
    {
        std::lock_guard lck(m);
        std::cout << "Поток №" << threadNumber
                  << ", begin: " << static_cast<size_t>(begin)
                  << ", end: " << static_cast<size_t>(end) << std::endl;
    }
    std::vector<char> result(sizeof(T));
    for (T i = begin; i < end; ++i) {
        std::copy_n(reinterpret_cast<const char *>(&i), sizeof(T),
                    result.begin());
        crc32(result.data(), result.size());
    }
    {
        std::lock_guard lck(m);
        std::cout << "Поток №" << threadNumber << " завершился" << std::endl;
    }
}

template <typename T>
uint TestHash() {
    const T maxVal = std::numeric_limits<T>::max();
    uint maxTotalThreads = std::thread::hardware_concurrency() + 2;
    uint totalThreads{1};
    std::cout << "maxVal: " << static_cast<size_t>(maxVal) << std::endl;
    std::map<double, uint> timeTest;
    for (; totalThreads <= maxTotalThreads; ++totalThreads) {
        std::cout << "Всего потоков: " << totalThreads << std::endl;
        T t = maxVal / totalThreads;
        std::vector<std::thread> threads;
        threads.reserve(totalThreads);
        std::chrono::time_point startTest =
            std::chrono::high_resolution_clock::now();
        for (uint i = 0; i < totalThreads; ++i) {
            size_t begin = (t + 1) * i;
            size_t end = begin + t > maxVal ? maxVal : begin + t;
            threads.emplace_back(std::thread{Calculation<T>, begin, end, i});
        }
        for (auto &t : threads) {
            t.join();
        }
        std::chrono::time_point endTest =
            std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsedTest = endTest - startTest;
        std::cout << elapsedTest.count() << " сек" << std::endl;
        timeTest.emplace(elapsedTest.count(), totalThreads);
    }
    std::cout << timeTest.cbegin()->second << " потока за минимальное время "
              << timeTest.cbegin()->first << " сек" << std::endl;
    return timeTest.cbegin()->second;
}