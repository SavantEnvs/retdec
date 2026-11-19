// Shared helpers for the retdec libFuzzer harnesses (mayhem/fuzz/*.cpp).
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <vector>

#include "retdec/fileformat/file_format/file_format.h"
#include "fuzz_tempfile.h"

namespace fuzz {

// Walks what a loaded FileFormat exposes, as retdec-fileinfo does: hashes, strings, sections,
// segments, symbol/relocation/dynamic tables and the per-format directories.
inline void exercise(retdec::fileformat::FileFormat &f) {
	using namespace retdec::fileformat;
	if (!f.isInValidState()) return;
	f.loadImpHash();
	f.loadExpHash();
	f.loadResourceIconHash();
	(void)f.getImportTable();
	(void)f.getExportTable();
	(void)f.getResourceTable();
	(void)f.getRichHeader();
	(void)f.getPdbInfo();
	(void)f.getCertificateTable();
	(void)f.getTlsInfo();
	(void)f.getOverlaySize();
	double ent = 0;
	(void)f.getOverlayEntropy(ent);
	std::vector<std::uint8_t> bytes;
	std::string s;
	(void)f.getEpBytes(bytes, 64);
	(void)f.getHexEpBytes(s, 64);
	(void)f.getHexBytesFromEnd(s, 64);
	(void)f.getEpSection();
	(void)f.getEpSegment();
	for (std::size_t i = 0; i < f.getNumberOfSections() && i < 256; ++i) {
		if (const Section *sec = f.getSection(i)) {
			(void)f.getBytes(bytes, sec->getOffset(), 64);
			std::uint64_t a = 0;
			(void)f.getAddressFromOffset(a, sec->getOffset());
			(void)f.getOffsetFromAddress(a, sec->getAddress());
		}
	}
	for (std::size_t i = 0; i < f.getNumberOfSegments() && i < 256; ++i) {
		if (const Segment *seg = f.getSegment(i)) {
			(void)f.getBytes(bytes, seg->getOffset(), 64);
		}
	}
	for (std::size_t i = 0; i < f.getNumberOfSymbolTables(); ++i) (void)f.getSymbolTable(i);
	for (std::size_t i = 0; i < f.getNumberOfRelocationTables(); ++i) (void)f.getRelocationTable(i);
}

} // namespace fuzz
