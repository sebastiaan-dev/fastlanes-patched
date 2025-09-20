// ────────────────────────────────────────────────────────
// |                      FastLanes                       |
// ────────────────────────────────────────────────────────
// src/include/fls/reader/rowgroup_view.hpp
// ────────────────────────────────────────────────────────
#ifndef FLS_READER_ROWGROUP_VIEW_HPP
#define FLS_READER_ROWGROUP_VIEW_HPP

#include "fls/std/span.hpp"
#include "fls/std/vector.hpp"
#include <unordered_map>

namespace fastlanes {
/*--------------------------------------------------------------------------------------------------------------------*/
struct RowgroupDescriptorT;
class ColumnView;
/*--------------------------------------------------------------------------------------------------------------------*/

class RowgroupView {
public:
	explicit RowgroupView(std::unordered_map<idx_t, std::span<std::byte>> map, const RowgroupDescriptorT& footer);

public:
	ColumnView&       operator[](n_t col_idx);
	const ColumnView& operator[](n_t col_idx) const;

public:
	vector<up<ColumnView>>           columns;
	std::unordered_map<n_t, idx_t> col_to_pos;
};

} // namespace fastlanes

#endif // FLS_READER_ROWGROUP_VIEW_HPP
