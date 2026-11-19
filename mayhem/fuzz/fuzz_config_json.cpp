// libFuzzer harness: retdec::config::Config::fromJsonString (the decompiler's JSON config: functions,
// types, objects, vtables, classes, patterns via serdes) and re-serialization.
#include <cstddef>
#include <cstdint>
#include <string>

#include "retdec/config/config.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
	std::string json(reinterpret_cast<const char *>(data), size);
	try {
		auto c = retdec::config::Config::fromJsonString(json);
		(void)c.generateJsonString();
	} catch (...) {
	}
	return 0;
}
