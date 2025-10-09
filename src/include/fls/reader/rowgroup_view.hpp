// ────────────────────────────────────────────────────────
// |                      FastLanes                       |
// ────────────────────────────────────────────────────────
// src/include/fls/reader/rowgroup_view.hpp
// ────────────────────────────────────────────────────────
#ifndef FLS_READER_ROWGROUP_VIEW_HPP
#define FLS_READER_ROWGROUP_VIEW_HPP

#include "fls/std/span.hpp"
#include "fls/std/vector.hpp"
#include <cstddef>
#include <memory>
#include <optional>

namespace fastlanes {
/*--------------------------------------------------------------------------------------------------------------------*/
struct RowgroupDescriptorT;
class ColumnView;

struct ColumnBufferReference {
	span<std::byte>       data;
	std::shared_ptr<void> owner;
};
/*--------------------------------------------------------------------------------------------------------------------*/

class RowgroupView {
public:
	explicit RowgroupView(const std::vector<std::optional<ColumnBufferReference>>& cols_by_id,
	                      const RowgroupDescriptorT&                               footer);

public:
	ColumnView&       operator[](n_t col_idx);
	const ColumnView& operator[](n_t col_idx) const;

public:
	vector<up<ColumnView>> columns;
};

} // namespace fastlanes

#endif // FLS_READER_ROWGROUP_VIEW_HPP
