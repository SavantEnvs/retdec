// Temp-file helper for the retdec APIs that only take a path (mayhem/fuzz/*.cpp).
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <vector>

namespace fuzz {

// Writes the input to a fresh file under $TMPDIR (default /tmp) for the retdec APIs that only take
// a path (PDBFile, ArchiveWrapper, BreakMachOUniversal). Removed by the destructor, so no state is
// kept between inputs.
struct TempFile {
	std::string path;
	explicit TempFile(const uint8_t *data, size_t size, const char *suffix = "") {
		const char *dir = getenv("TMPDIR");
		std::string tmpl = std::string(dir && *dir ? dir : "/tmp") + "/retdec-fuzz-XXXXXX" + suffix;
		std::vector<char> buf(tmpl.begin(), tmpl.end());
		buf.push_back('\0');
		int fd = mkstemps(buf.data(), static_cast<int>(std::char_traits<char>::length(suffix)));
		if (fd < 0) abort();
		path = buf.data();
		size_t off = 0;
		while (off < size) {
			ssize_t n = write(fd, data + off, size - off);
			if (n <= 0) abort();
			off += static_cast<size_t>(n);
		}
		close(fd);
	}
	~TempFile() { unlink(path.c_str()); }
};

} // namespace fuzz
