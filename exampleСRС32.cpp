#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

#include "CRC32.hpp"

int main() {
    const std::string original{"HelpMe"};
    const std::string injection{"He-he-he"};
    std::vector<char> result(original.size() + injection.size());
    auto it = std::copy(original.begin(), original.end(), result.begin());
    std::copy(injection.begin(), injection.end(), it);
    const uint32_t originalCrc32 = crc32(original.data(), original.size());
    std::cout << "Original Crc32: " << originalCrc32 << std::endl;
    const uint32_t resultWithoutInjectionCrc32 =
        crc32(result.data(), original.size());
    std::cout << "Result Without Injection Crc32: "
              << resultWithoutInjectionCrc32 << std::endl;
    const uint32_t resultWithInjectionCrc32 =
        crc32(result.data() + original.size(), injection.size(),
              resultWithoutInjectionCrc32);
    std::cout << "Result With Injection Crc32: " << resultWithInjectionCrc32
              << std::endl;
    const uint32_t resultFullCrc32 = crc32(result.data(), result.size());
    std::cout << "Result Full Crc32: " << resultFullCrc32 << std::endl;
}