// libFuzzer harness: retdec demanglers (Itanium/GCC, Microsoft, Borland) -- the first byte picks the
// demangler, the rest is the mangled name; both demangleToString and the ctypes conversion run.
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "fuzz_llvm_abort.h"
#include "retdec/ctypes/context.h"
#include "retdec/ctypes/module.h"
#include "retdec/demangler/demangler.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
	if (size < 1) return -1;
	std::string mangled(reinterpret_cast<const char *>(data + 1), size - 1);
	std::unique_ptr<retdec::demangler::Demangler> d;
	switch (data[0] % 3) {
		case 0: d = std::make_unique<retdec::demangler::ItaniumDemangler>(); break;
		case 1: d = std::make_unique<retdec::demangler::MicrosoftDemangler>(); break;
		default: d = std::make_unique<retdec::demangler::BorlandDemangler>(); break;
	}
	try {
		(void)d->demangleToString(mangled);
		auto module = std::make_unique<retdec::ctypes::Module>(std::make_shared<retdec::ctypes::Context>());
		retdec::ctypesparser::CTypesParser::TypeWidths widths{{"int", 32}, {"long", 64}, {"char", 8}};
		retdec::ctypesparser::CTypesParser::TypeSignedness sign;
		(void)d->demangleFunctionToCtypes(mangled, module, widths, sign, 32);
	} catch (...) {
	}
	return 0;
}
