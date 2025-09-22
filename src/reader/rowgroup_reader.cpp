// ────────────────────────────────────────────────────────
// |                      FastLanes                       |
// ────────────────────────────────────────────────────────
// src/reader/rowgroup_reader.cpp
// ────────────────────────────────────────────────────────
#include "fls/reader/rowgroup_reader.hpp" //
#include "fls/common/alias.hpp"
#include "fls/connection.hpp"                     // for Connector (ptr only)
#include "fls/cor/lyt/buf.hpp"                    // for Buf
#include "fls/csv/csv.hpp"                        // for CSV
#include "fls/encoder/materializer.hpp"           //
#include "fls/expression/decoding_operator.hpp"   //
#include "fls/expression/encoding_operator.hpp"   //
#include "fls/expression/expression_executor.hpp" //
#include "fls/expression/interpreter.hpp"         // for Interpreter
#include "fls/expression/physical_expression.hpp" // for PhysicalExpr
#include "fls/footer/rowgroup_descriptor.hpp"     // for Footer, ColumnMeta...
#include "fls/io/file.hpp"                        // for File
#include "fls/io/io.hpp"                          // for IO, io
#include "fls/reader/column_view.hpp"
#include "fls/reader/rowgroup_view.hpp"
#include "fls/std/filesystem.hpp"
#include "fls/table/chunk.hpp" // for Chunk
#include <memory>              // for make_unique, uniqu...
#include <numeric>

namespace fastlanes {

RowgroupReader::RowgroupReader(const path&                file_path,
                               const RowgroupDescriptorT& rowgroup_descriptor,
                               Connection&                connection,
                               const std::vector<idx_t>&  column_ids)
    : m_connection(connection)
    , m_rowgroup_descriptor(rowgroup_descriptor)
    , m_column_ids(column_ids) {

	// read file
	{
		// allocate buffer
		io                                              io = make_unique<File>(file_path); // todo[IO]
		std::unordered_map<idx_t, ColumnBufferReference> column_map;
		column_map.reserve(m_column_ids.size());
		m_column_bufs.reserve(m_column_ids.size());

		for (const auto& col_idx : m_column_ids) {
			auto& column_descriptor = m_rowgroup_descriptor.m_column_descriptors[col_idx];
			auto  column_size       = column_descriptor->total_size;
			auto  column_offset     = m_rowgroup_descriptor.m_offset + column_descriptor->column_offset;

			auto buffer = std::make_shared<Buf>(column_size); // todo[memory_pool]
			IO::range_read(io, *buffer, column_offset, column_size);
			m_column_bufs.push_back(buffer);
			column_map.emplace(col_idx,
			                   ColumnBufferReference {buffer->Span(), std::shared_ptr<void>(buffer, buffer.get())});
		}

		m_rowgroup_view = make_unique<RowgroupView>(column_map, m_rowgroup_descriptor);
	}

	// init level 1 expression
	{
		m_expressions.reserve(m_column_ids.size());
		for (const auto& col_idx : m_column_ids) {
			auto& column_descriptor = m_rowgroup_descriptor.m_column_descriptors[col_idx];
			auto& column_view       = (*m_rowgroup_view)[col_idx];

			InterpreterState state;
			auto             physical_expr = make_decoding_expression(*column_descriptor, column_view, *this, state);
			ExprExecutor::CountOperator(*physical_expr);
			m_expressions.emplace_back(physical_expr);
		}
	}
}

RowgroupReader::RowgroupReader(const path&                file_path,
                               const RowgroupDescriptorT& rowgroup_descriptor,
                               Connection&                connection)
    : m_connection(connection)
    , m_rowgroup_descriptor(rowgroup_descriptor) {

	m_column_ids.resize(m_rowgroup_descriptor.m_column_descriptors.size());
	std::iota(m_column_ids.begin(), m_column_ids.end(), idx_t {0});

	// read file
	{
		// allocate buffer
		io                                              io     = make_unique<File>(file_path); // todo[IO]
		uint64_t                                        offset = m_rowgroup_descriptor.m_offset;
		std::unordered_map<idx_t, ColumnBufferReference> column_map;
		column_map.reserve(m_column_ids.size());

		for (const auto& col_idx : m_column_ids) {
			auto& column_descriptor = m_rowgroup_descriptor.m_column_descriptors[col_idx];
			auto  column_size       = column_descriptor->total_size;

			auto buffer = std::make_shared<Buf>(column_size); // todo[memory_pool]
			IO::range_read(io, *buffer, offset, column_size);
			offset += column_size;
			m_column_bufs.push_back(buffer);
			column_map.emplace(col_idx,
			                   ColumnBufferReference {buffer->Span(), std::shared_ptr<void>(buffer, buffer.get())});
		}

		m_rowgroup_view = make_unique<RowgroupView>(column_map, m_rowgroup_descriptor);
	}

	// init level 1 expression[
	{
		m_expressions.reserve(m_column_ids.size());
		for (const auto& col_idx : m_column_ids) {
			auto& column_descriptor = m_rowgroup_descriptor.m_column_descriptors[col_idx];
			auto& column_view       = (*m_rowgroup_view)[col_idx];

			InterpreterState state;
			auto             physical_expr = make_decoding_expression(*column_descriptor, column_view, *this, state);
			ExprExecutor::CountOperator(*physical_expr);
			m_expressions.emplace_back(physical_expr);
		}
	}
}

vector<sp<PhysicalExpr>>& RowgroupReader::get_chunk(const n_t vec_idx) {
	for (idx_t i {0}; i < m_column_ids.size(); i++) {
		auto& physical_expr = *m_expressions[i];
		ExprExecutor::smart_execute(physical_expr, vec_idx);
	}
	return m_expressions;
}

void RowgroupReader::reset() {
}

const RowgroupDescriptorT& RowgroupReader::get_descriptor() const {
	return m_rowgroup_descriptor;
}

up<Rowgroup> RowgroupReader::materialize() {
	auto               rowgroup_up = std::make_unique<Rowgroup>(m_rowgroup_descriptor, m_connection);
	const Materializer materializer {*rowgroup_up};

	for (n_t vec_idx {0}; vec_idx < m_rowgroup_descriptor.m_n_vec; vec_idx++) {
		auto& expressions = get_chunk(vec_idx);
		materializer.Materialize(expressions, vec_idx);
	};

	// materializer.rowgroup.Cast();
	materializer.rowgroup.Finalize();
	materializer.rowgroup.GetStatistics();

	return rowgroup_up;
}

void RowgroupReader::to_csv(const path& dir_path) {
	const auto& materialized_rowgroup = materialize();
	CSV::to_csv(dir_path, *materialized_rowgroup, materialized_rowgroup->m_descriptor);
}

} // namespace fastlanes
