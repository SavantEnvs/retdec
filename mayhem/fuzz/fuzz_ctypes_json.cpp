// libFuzzer harness: retdec::ctypesparser::JSONCTypesParser::parse -- the JSON C-type database
// (functions, structs, unions, enums, typedefs) the decompiler loads signatures from.
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>

#include "retdec/ctypesparser/json_ctypes_parser.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
	std::istringstream in(std::string(reinterpret_cast<const char *>(data), size));
	try {
		retdec::ctypesparser::JSONCTypesParser p(32);
		(void)p.parse(in, {{"int", 32}, {"char", 8}, {"long", 64}});
	} catch (...) {
	}
	return 0;
}
