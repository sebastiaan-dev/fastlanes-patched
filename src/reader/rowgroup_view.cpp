// ────────────────────────────────────────────────────────
// |                      FastLanes                       |
// ────────────────────────────────────────────────────────
// src/reader/rowgroup_view.cpp
// ────────────────────────────────────────────────────────
#include "fls/reader/rowgroup_view.hpp"
#include "fls/common/alias.hpp"
#include "fls/common/assert.hpp"
#include "fls/footer/rowgroup_descriptor_generated.h"
#include "fls/reader/column_view.hpp"
#include "fls/std/span.hpp"
#include <cstddef> // for std::byte

namespace fastlanes {

RowgroupView::RowgroupView(std::unordered_map<idx_t, std::span<std::byte>> map, const RowgroupDescriptorT& footer) {
	col_to_pos.reserve(map.size());

	for (const auto& [id, data] : map) {
		const auto& column_descriptor = footer.m_column_descriptors.at(id);
		columns.emplace_back(
		    std::make_unique<ColumnView>(data, *column_descriptor, footer, column_descriptor->column_offset));
		col_to_pos.emplace(id, columns.size() - 1);
	}
}

ColumnView& RowgroupView::operator[](const n_t col_idx) {
	FLS_ASSERT_NOT_EMPTY_VEC(columns)

	const auto pos_idx = col_to_pos.at(col_idx);
	return *columns[pos_idx];
}

const ColumnView& RowgroupView::operator[](const n_t col_idx) const {
	FLS_ASSERT_NOT_EMPTY_VEC(columns)

	const auto pos_idx = col_to_pos.at(col_idx);
	return *columns[pos_idx];
}
} // namespace fastlanes
