#include "fls/reader/segment.hpp"
#include "fls/footer/segment_descriptor.hpp"
#include "fls/cor/lyt/buf.hpp"
#include <iostream>

int main() {
    fastlanes::Segment seg;
    seg.MakeBlockBased();
    auto* arr = seg.GetFixedSizeArray<int32_t>(3);
    arr[0] = 0;
    arr[1] = 7;
    arr[2] = 42;

    fastlanes::Buf external;
    fastlanes::n_t offset = 0;
    uint8_t helper[1024]{};
    auto descriptor = seg.Dump(external, offset, helper);

    auto view = fastlanes::make_segment_view(external.Span(), *descriptor, 0, nullptr);
    auto ptr = reinterpret_cast<const int32_t*>(view.data_span.data());
    std::cout << ptr[0] << "," << ptr[1] << "," << ptr[2] << std::endl;
    return 0;
}
