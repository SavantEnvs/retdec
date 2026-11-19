// libFuzzer harness: retdec::macho_extractor::BreakMachOUniversal -- fat (universal) Mach-O header,
// architecture listings (plain + JSON, with archive member names) and best-architecture extraction.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <sstream>
#include <string>

#include "fuzz_llvm_abort.h"
#include "retdec/macho-extractor/break_fat.h"
#include "fuzz_tempfile.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
	fuzz::TempFile tf(data, size);
	std::string out = tf.path + ".out";
	try {
		retdec::macho_extractor::BreakMachOUniversal fat(tf.path);
		if (fat.isValid()) {
			std::ostringstream ss;
			fat.listArchitectures(ss, true);
			fat.listArchitecturesJson(ss, true);
			(void)fat.isStaticLibrary();
			fat.extractBestArchive(out);
		}
	} catch (...) {
	}
	std::remove(out.c_str());
	return 0;
}
