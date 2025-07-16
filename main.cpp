#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <limits>
#include <mutex>
#include <thread>
#include <vector>

#include "CRC32.hpp"
#include "IO.hpp"
#include "test_hash.hpp"

// inline std::mutex m;

/// @brief Переписывает последние 4 байта значением value
void replaceLastFourBytes(std::vector<char> &data, uint32_t value) {
    std::copy_n(reinterpret_cast<const char *>(&value), 4, data.end() - 4);
}

/**
 * @brief Функция вспомогательного потока подбора четырех байт
 * для получения нового вектор с тем же CRC32
 */
void SearchCRC32(std::vector<char> result, const uint32_t originalCrc32,
                 size_t &hackResults, bool &success, uint totalThreads,
                 uint threadNumber) {
    const size_t maxVal = std::numeric_limits<uint32_t>::max();
    size_t t = maxVal / totalThreads;
    size_t begin = (t + 1) * threadNumber;
    size_t end = begin + t;
    {
        std::lock_guard lck(m);
        std::cout << "Поток №" << threadNumber << " maxVal: " << maxVal
                  << ", begin: " << begin << ", end: " << end << std::endl;
    }
    for (size_t i = begin; i < end; ++i) {
        if (success) {
            {
                std::lock_guard lck(m);
                std::cout << "Поток №" << threadNumber
                          << " завершился после чужого успеха" << std::endl;
            }
            return;
        }
        // Заменяем последние четыре байта на значение i
        replaceLastFourBytes(result, uint32_t(i));
        // Вычисляем CRC32 текущего вектора result
        auto currentCrc32 = crc32(result.data(), result.size());

        if (currentCrc32 == originalCrc32) {
            success = true;
            hackResults = i;
            {
                std::lock_guard lck(m);
                std::cout << "Успех! найдено 4 байта: " << std::hex
                          << std::uppercase << std::setw(8) << std::setfill('0')
                          << i << ", поток №" << threadNumber << "\n";
            }
            return;
        }
        // Отображаем прогресс
        if (i % 100000000 == 0) {
            {
                std::lock_guard lck(m);
                std::cout << "Поток №" << threadNumber << " progress: "
                          << static_cast<double>(i - begin) /
                                 static_cast<double>(maxVal / totalThreads)
                          << std::endl;
            }
        }
    }
}

/**
 * @brief Формирует новый вектор с тем же CRC32, добавляя в конец оригинального
 * строку injection и дополнительные 4 байта
 * @details При формировании нового вектора последние 4 байта не несут полезной
 * нагрузки и подбираются таким образом, чтобы CRC32 нового и оригинального
 * вектора совпадали
 * @param original оригинальный вектор
 * @param injection произвольная строка, которая будет добавлена после данных
 * оригинального вектора
 * @return новый вектор
 */
std::vector<char> hack(const std::vector<char> &original,
                       const std::string &injection) {
    const uint32_t originalCrc32 = crc32(original.data(), original.size());

    uint totalThreads = TestHash<u_int16_t>();

    std::vector<char> result(original.size() + injection.size() + 4);
    auto it = std::copy(original.begin(), original.end(), result.begin());
    std::copy(injection.begin(), injection.end(), it);

    // uint totalThreads = std::thread::hardware_concurrency();
    std::cout << "Всего потоков: " << totalThreads << std::endl;

    size_t hackResults{0};
    std::vector<std::thread> threads;
    threads.reserve(totalThreads);
    bool success{false};

    for (uint i = 0; i < totalThreads; ++i) {
        threads.emplace_back(std::thread{SearchCRC32, result, originalCrc32,
                                         std::ref(hackResults),
                                         std::ref(success), totalThreads, i});
    }
    for (auto &t : threads) {
        t.join();
    }
    replaceLastFourBytes(result, uint32_t(hackResults));
    return result;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Call with two args: " << argv[0]
                  << " <input file> <output "
                     "file>\n";
        return 1;
    }
    std::chrono::time_point start = std::chrono::high_resolution_clock::now();
    try {
        const std::vector<char> data = readFromFile(argv[1]);
        const std::vector<char> badData = hack(data, "He-he-he");
        writeToFile(argv[2], badData);
    } catch (std::exception &ex) {
        std::chrono::time_point end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << elapsed.count() << " сек" << std::endl;
        std::cerr << ex.what() << '\n';
        return 2;
    }
    std::chrono::time_point end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << elapsed.count() << " сек" << std::endl;
    return 0;
}
