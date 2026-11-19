// libFuzzer harness: retdec::fileformat::ElfFormat over an in-memory file (the parser retdec-fileinfo,
// the loader and the decompiler use for this format), then every table it exposes.
#include <cstddef>
#include <cstdint>
#include <exception>

#include "fuzz_llvm_abort.h"
#include "retdec/fileformat/file_format/elf/elf_format.h"
#include "fuzz_common.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
	try {
		retdec::fileformat::ElfFormat f(data, size);
		fuzz::exercise(f);
	} catch (...) {
		// retdec reports malformed input with exceptions (e.g. LoaderError); not a defect.
	}
	return 0;
}
