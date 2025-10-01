// ────────────────────────────────────────────────────────
// |                      FastLanes                       |
// ────────────────────────────────────────────────────────
// src/io/file.cpp
// ────────────────────────────────────────────────────────
#include "fls/io/file.hpp"
#include "fls/common/alias.hpp"
#include "fls/common/assert.hpp"
#include "fls/cor/lyt/buf.hpp"
#include "fls/std/filesystem.hpp"
#include "fls/std/string.hpp"
#include <cstdint>    // for int64_t
#include <filesystem> // for std::filesystem::file_size, std::filesystem::exists
#include <fstream>    // for std::ifstream, std::ofstream
#include <ios>        // for std::ios, std::streamoff, std::streamsize
#include <memory>     // for std::make_unique
#include <sstream>
#include <stdexcept> // for std::runtime_error

// namespace fastlanes {

// File::File(const path& path) // NOLINT
//     : m_path(path) {
// }
//
// File::~File() {
//
// 	if (m_of_stream != nullptr) {
// 		FileSystem::close(*m_of_stream);
// 	}
//
// 	if (m_if_stream != nullptr) {
// 		FileSystem::close(*m_if_stream);
// 	}
// }
//
// void File::Write(const Buf& buf) {
// 	if (m_of_stream == nullptr) {
// 		m_of_stream = make_unique<std::ofstream>(FileSystem::open_w(m_path));
// 	}
// 	//
// 	m_of_stream->write(reinterpret_cast<char*>(buf.data()), static_cast<int64_t>(buf.Size()));
// }
//
// void File::Read(Buf& buf) {
// 	if (m_if_stream == nullptr) {
// 		m_if_stream = make_unique<std::ifstream>(FileSystem::open_r_binary(m_path));
// 	}
//
// 	auto file_size = fs::file_size(m_path);
// 	FLS_ASSERT_LE(file_size, buf.Capacity())
//
// 	m_if_stream->read(reinterpret_cast<char*>(buf.mutable_data()), static_cast<int64_t>(file_size));
// }
//
// void File::ReadRange(Buf& buf, const n_t offset, const n_t size) {
// 	if (m_if_stream == nullptr) {
// 		m_if_stream = make_unique<std::ifstream>(FileSystem::open_r_binary(m_path));
// 	}
//
// 	[[maybe_unused]] auto file_size = fs::file_size(m_path);
// 	FLS_ASSERT_LE(offset + size, file_size);
// 	FLS_ASSERT_LE(size, buf.Capacity());
//
// 	m_if_stream->seekg(static_cast<std::streamoff>(offset), std::ios::beg);
// 	m_if_stream->read(reinterpret_cast<char*>(buf.mutable_data()), static_cast<std::streamsize>(size));
// }
//
// n_t File::Size() const {
// 	if (!exists(m_path)) {
// 		throw std::runtime_error("File does not exist");
// 	}
// 	return static_cast<n_t>(std::filesystem::file_size(m_path));
// }
//
// void File::Append(const Buf& buf) {
// 	if (m_of_stream == nullptr) {
// 		// Open file in append mode
// 		m_of_stream = std::make_unique<std::ofstream>(m_path, std::ios::binary | std::ios::app);
// 	}
// 	m_of_stream->write(reinterpret_cast<char*>(buf.data()), static_cast<int64_t>(buf.Size()));
// }
//
// void File::Append(const char* pointer, n_t size) {
// 	if (m_of_stream == nullptr) {
// 		// Open file in append mode
// 		m_of_stream = std::make_unique<std::ofstream>(m_path, std::ios::binary | std::ios::app);
// 	}
// 	m_of_stream->write(pointer, static_cast<int64_t>(size));
// }
//
// /*--------------------------------------------------------------------------------------------------------------------*\
//  * STATIC
// \*--------------------------------------------------------------------------------------------------------------------*/
// string File::read(const path& file_path) {
// 	std::ifstream     json_stream = FileSystem::open_r(file_path);
// 	std::stringstream buffer;
// 	buffer << json_stream.rdbuf();
// 	return buffer.str();
// }
//
// void File::write(const path& dir_path, const string& dump) {
// 	auto file = FileSystem::open_w(dir_path);
//
// 	file << dump;
//
// 	FileSystem::close(file);
// }
//
// void File::append(const path& dir_path, const string& dump) {
// 	auto file = FileSystem::opend_app(dir_path);
//
// 	file << dump;
//
// 	FileSystem::close(file);
// }
// } // namespace fastlanes

// src/io/file.cpp
#include "fls/io/file.hpp"
#include "fls/common/assert.hpp"
#include "fls/cor/lyt/buf.hpp"
#include "fls/std/filesystem.hpp"
#include "fls/std/string.hpp"
#include <cstdint>
#include <stdexcept>
#include <system_error>

// POSIX
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace fastlanes {

File::File(const path& p)
    : m_path(p) {
}

File::~File() {
	if (m_of_stream) {
		FileSystem::close(*m_of_stream);
	}
	if (fd_ >= 0) {
		::close(fd_);
		fd_ = -1;
	}
}

static inline n_t stat_size(const path& p) {
	struct stat st {};
	if (::stat(p.c_str(), &st) != 0) {
		throw std::system_error(errno, std::generic_category(), "stat failed");
	}
	return static_cast<n_t>(st.st_size);
}

void File::ensure_fd_open_for_read() {
	if (fd_ >= 0)
		return;
	fd_ = ::open(m_path.c_str(), O_RDONLY);
	if (fd_ < 0) {
		throw std::system_error(errno, std::generic_category(), "open(O_RDONLY) failed");
	}
#if defined(__APPLE__)
	// Strided/random reads benefit from disabling sequential readahead
	int one = 0;
	(void)fcntl(fd_, F_RDAHEAD, one); // set to 0 to disable; keep as-is if you prefer kernel defaults
#endif
	file_size_cached_ = stat_size(m_path);
}

void File::Write(const Buf& buf) {
	if (!m_of_stream) {
		m_of_stream = std::make_unique<std::ofstream>(FileSystem::open_w(m_path));
	}
	m_of_stream->write(reinterpret_cast<const char*>(buf.data()), static_cast<std::streamsize>(buf.Size()));
}

void File::Read(Buf& buf) {
	ensure_fd_open_for_read();
	FLS_ASSERT_LE(file_size_cached_, buf.Capacity());
	// issue one pread (loop for short/EINTR)
	n_t      to_read = file_size_cached_;
	uint8_t* dst     = buf.mutable_data();
	n_t      done    = 0;
	while (done < to_read) {
		ssize_t n = ::pread(fd_, dst + done, static_cast<size_t>(to_read - done), static_cast<off_t>(done));
		if (n < 0 && errno == EINTR)
			continue;
		if (n <= 0)
			throw std::runtime_error("pread short/failed in Read()");
		done += static_cast<n_t>(n);
	}
}
void File::Append(const Buf& buf) {
	if (!m_of_stream) {
		// Open in append mode, binary
		m_of_stream = std::make_unique<std::ofstream>(m_path, std::ios::binary | std::ios::app);
		if (!*m_of_stream) {
			throw std::runtime_error("failed to open file for append");
		}
	}
	m_of_stream->write(reinterpret_cast<const char*>(buf.data()), static_cast<std::streamsize>(buf.Size()));
	if (!*m_of_stream) {
		throw std::runtime_error("append write failed");
	}
}

void File::Append(const char* pointer, n_t size) {
	if (!m_of_stream) {
		m_of_stream = std::make_unique<std::ofstream>(m_path, std::ios::binary | std::ios::app);
		if (!*m_of_stream) {
			throw std::runtime_error("failed to open file for append");
		}
	}
	m_of_stream->write(pointer, static_cast<std::streamsize>(size));
	if (!*m_of_stream) {
		throw std::runtime_error("append write failed");
	}
}

void File::ReadRange(Buf& buf, const n_t offset, const n_t size) {
	// int one = 1, zero = 0;
	// fcntl(fd_, F_NOCACHE, one);   // don’t use page cache
	// fcntl(fd_, F_RDAHEAD, zero);
	// ensure_fd_open_for_read();
	// FLS_ASSERT_LE(offset + size, file_size_cached_);
	// FLS_ASSERT_LE(size, buf.Capacity());
	//
	// n_t      done = 0;
	// uint8_t* dst  = buf.mutable_data();
	// while (done < size) {
	// 	ssize_t n = ::pread(fd_, dst + done, static_cast<size_t>(size - done), static_cast<off_t>(offset + done));
	// 	if (n < 0 && errno == EINTR)
	// 		continue;
	// 	if (n <= 0)
	// 		throw std::runtime_error("pread short/failed in ReadRange()");
	// 	done += static_cast<n_t>(n);
	// }
	ensure_fd_open_for_read();
	FLS_ASSERT_LE(offset + size, file_size_cached_);
	FLS_ASSERT_LE(size, buf.Capacity());

	n_t      done = 0;
	uint8_t* dst  = buf.mutable_data();

	while (done < size) {
		uint32_t inflight = IoTracer::get().on_submit();
		auto     t0       = std::chrono::steady_clock::now();
		ssize_t  n  = ::pread(fd_, dst + done, static_cast<size_t>(size - done), static_cast<off_t>(offset + done));
		auto     t1 = std::chrono::steady_clock::now();
		IoTracer::get().on_complete();

		if (n < 0 && errno == EINTR)
			continue;
		if (n <= 0)
			throw std::runtime_error("pread short/failed in ReadRange()");

		auto     dur = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
		uint64_t ns  = static_cast<uint64_t>(dur);
		IoTracer::get().record((uint64_t)(offset + done), (uint32_t)n, ns, inflight);

		done += static_cast<n_t>(n);
	}
}

n_t File::Size() const {
	if (!exists(m_path))
		throw std::runtime_error("File does not exist");
	return static_cast<n_t>(std::filesystem::file_size(m_path));
}

/*--------------------------------------------------------------------------------------------------------------------*\
 * STATIC
\*--------------------------------------------------------------------------------------------------------------------*/
string File::read(const path& file_path) {
	std::ifstream     json_stream = FileSystem::open_r(file_path);
	std::stringstream buffer;
	buffer << json_stream.rdbuf();
	return buffer.str();
}

void File::write(const path& dir_path, const string& dump) {
	auto file = FileSystem::open_w(dir_path);

	file << dump;

	FileSystem::close(file);
}

void File::append(const path& dir_path, const string& dump) {
	auto file = FileSystem::opend_app(dir_path);

	file << dump;

	FileSystem::close(file);
}

} // namespace fastlanes