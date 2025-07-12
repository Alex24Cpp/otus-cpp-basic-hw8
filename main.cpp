#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <limits>
#include <thread>
#include <vector>

#include "CRC32.hpp"
#include "IO.hpp"

/// @brief Переписывает последние 4 байта значением value
void replaceLastFourBytes(std::vector<char> &data, uint32_t value) {
    std::copy_n(reinterpret_cast<const char *>(&value), 4, data.end() - 4);
}

void SearchCRC32(std::vector<char> &result, const uint32_t originalCrc32,
                 bool &success) {
    const size_t maxVal = std::numeric_limits<uint32_t>::max();
    size_t i = 0;
    for (; i < maxVal; ++i) {
        // Заменяем последние четыре байта на значение i
        replaceLastFourBytes(result, uint32_t(i));
        // Вычисляем CRC32 текущего вектора result
        auto currentCrc32 = crc32(result.data(), result.size());

        if (currentCrc32 == originalCrc32) {
            success = true;
            std::cout << "Success! 4 bytes found: " << std::hex
                      << std::uppercase << std::setw(8) << std::setfill('0')
                      << i << "\n";
            return;
        }
        // Отображаем прогресс
        if (i % 1000 == 0) {
            std::cout << "progress: "
                      << static_cast<double>(i) / static_cast<double>(maxVal)
                      << std::endl;
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

    std::vector<char> result(original.size() + injection.size() + 4);
    auto it = std::copy(original.begin(), original.end(), result.begin());
    std::copy(injection.begin(), injection.end(), it);

    bool success{false};
    std::thread do1{SearchCRC32, std::ref(result), originalCrc32,
                    std::ref(success)};
    do1.join();
    if (!success) {
        throw std::logic_error("Can't hack");
    }
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
