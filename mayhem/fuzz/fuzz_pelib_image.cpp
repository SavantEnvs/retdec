// libFuzzer harness: PeLib alone (no fileformat layer) -- PeFileT::loadPeHeaders(ByteBuffer) through
// ImageLoader::Load, then every directory reader and relocateImage.
// ImageLoader::Save is not called: it is an output writer that retdec only uses from the unpacker
// plugins (upx, mpress; IoFlagNewFile), and its default-flags path allocates a buffer sized by a
// section header's raw size, so it turned most inputs into multi-GiB allocations.
// Like upstream's PeFormat (fileformat/file_format/pe/pe_format.cpp: `new PeFileT(fileStream)` then
// `loadPeHeaders(bytes)`), PeFileT gets a stream over the same input: the security and debug
// directory readers read from that stream and bound their sizes by its length.
#include <cstddef>
#include <cstdint>
#include <sstream>

#include "retdec/pelib/PeFile.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
	try {
		PeLib::ByteBuffer buf(data, data + size);
		std::istringstream in(std::string(reinterpret_cast<const char *>(data), size));
		PeLib::PeFileT pe(in);
		if (pe.loadPeHeaders(buf) != PeLib::ERROR_NONE) return 0;
		pe.readExportDirectory();
		pe.readImportDirectory();
		pe.readBoundImportDirectory();
		pe.readResourceDirectory();
		pe.readRelocationsDirectory();
		pe.readComHeaderDirectory();
		pe.readIatDirectory();
		pe.readDebugDirectory();
		pe.readTlsDirectory();
		pe.readDelayImportDirectory();
		pe.readSecurityDirectory();
		pe.readLoadConfigDirectory();
		pe.readCoffSymbolTable(buf);
		pe.imageLoader().relocateImage(0x10000000);
	} catch (...) {
	}
	return 0;
}
