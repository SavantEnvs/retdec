// libFuzzer harness: retdec::loader::createImage(FileFormat) -- maps the parsed file into an image
// (segment layout, overlap checks, ELF/PE/Mach-O/COFF/IHEX loaders) and reads from each segment.
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "fuzz_llvm_abort.h"
#include "retdec/fileformat/format_factory.h"
#include "retdec/loader/image_factory.h"
#include "retdec/loader/loader/image.h"
#include "retdec/loader/loader/segment.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
	try {
		std::shared_ptr<retdec::fileformat::FileFormat> ff =
			retdec::fileformat::createFileFormat(data, size);
		if (!ff) return 0;
		auto img = retdec::loader::createImage(ff);
		if (!img) return 0;
		(void)img->getEpSegment();
		std::vector<std::uint8_t> bytes;
		std::uint64_t v = 0;
		for (const auto &seg : img->getSegments()) {
			std::uint64_t a = seg->getAddress();
			(void)img->getXBytes(a, 16, bytes);
			(void)img->getXByte(a, 4, v);
			(void)img->isPointer(a);
			(void)img->getRawSegmentData(a);
			(void)img->getRawSegmentData(seg->getEndAddress() ? seg->getEndAddress() - 1 : 0);
		}
	} catch (...) {
	}
	return 0;
}
