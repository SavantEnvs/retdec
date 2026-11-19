// LLVM's report_fatal_error() exit(1)s by default; libFuzzer and Mayhem then see an uncategorized
// "fuzz target exited". LLVM's own switch (install_fatal_error_handler) turns it into an abort()
// so the fatal error is reported as a crash with its backtrace (docs/rules/c.md: exit -> abort).
#pragma once
#include <cstdio>
#include <cstdlib>
#include <string>

#include "llvm/Support/ErrorHandling.h"

// Weak, so a build without sanitizers (empty SANITIZER_FLAGS) still links.
extern "C" void __sanitizer_print_stack_trace(void) __attribute__((weak));

namespace fuzz {
inline void llvmFatalAbort(void *, const std::string &reason, bool) {
	std::fprintf(stderr, "LLVM ERROR: %s\n", reason.c_str());
	// The backtrace through report_fatal_error names the LLVM check that fired (for example which
	// MachOObjectFile validation), which the abort report alone does not always carry.
	if (__sanitizer_print_stack_trace) __sanitizer_print_stack_trace();
	std::abort();
}
} // namespace fuzz

extern "C" int LLVMFuzzerInitialize(int *, char ***) {
	llvm::install_fatal_error_handler(fuzz::llvmFatalAbort);
	return 0;
}
