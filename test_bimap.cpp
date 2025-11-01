#include "fls/common/bimap_frequency.hpp"
#include <iostream>

int main() {
    fastlanes::BiMapFrequency<int32_t> bmf;
    bmf.insert(0, 5);
    bmf.insert(1, 0);
    std::cout << "key(0)=" << bmf.get_key(0) << " key(5)=" << bmf.get_key(5) << "\n";
    return 0;
}
