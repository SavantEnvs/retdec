// libFuzzer harness: retdec::fileformat::createFileFormat(data, size, isRaw) -- format detection and
// the format it picks (PE/ELF/COFF/Mach-O/Intel HEX, or raw data). The first byte selects isRaw.
#include <cstddef>
#include <cstdint>

#include "fuzz_llvm_abort.h"
#include "retdec/fileformat/format_factory.h"
#include "fuzz_common.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
	if (size < 1) return -1;
	const bool isRaw = data[0] & 1;
	try {
		auto f = retdec::fileformat::createFileFormat(data + 1, size - 1, isRaw);
		if (f) fuzz::exercise(*f);
	} catch (...) {
	}
	return 0;
}
