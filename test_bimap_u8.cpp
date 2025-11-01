#include "fls/common/bimap_frequency.hpp"
#include <iostream>

int main() {
    fastlanes::BiMapFrequency<uint8_t> bmf;
    bmf.insert(0, static_cast<uint8_t>(1));
    bmf.insert(1, static_cast<uint8_t>(0));
    for (const auto& [value, idx] : bmf.value_to_key) {
        std::cout << "value=" << static_cast<int>(value) << " idx=" << idx << "\n";
    }
    return 0;
}
