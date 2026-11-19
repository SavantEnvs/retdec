// libFuzzer harness: retdec::ar_extractor::ArchiveWrapper -- ar archive parsing, plain and JSON
// member listings, and extraction of every member into a scratch directory.
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <string>

#include "fuzz_llvm_abort.h"
#include "retdec/ar-extractor/archive_wrapper.h"
#include "fuzz_tempfile.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
	fuzz::TempFile tf(data, size, ".a");
	std::string outDir = tf.path + ".d";
	try {
		bool ok = false;
		std::string err, res;
		retdec::ar_extractor::ArchiveWrapper ar(tf.path, ok, err);
		if (ok) {
			ar.getPlainTextList(res, err, true, true);
			ar.getJsonList(res, err, false, true);
			(void)ar.isThinArchive();
			(void)ar.isEmptyArchive();
			std::filesystem::create_directory(outDir);
			const std::size_t n = ar.getNumberOfObjects();
			for (std::size_t i = 0; i < n && i < 16; ++i)
				ar.extractByIndex(i, err, outDir + "/m" + std::to_string(i));
		}
	} catch (...) {
	}
	std::error_code ec;
	std::filesystem::remove_all(outDir, ec);
	return 0;
}
