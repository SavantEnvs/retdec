// Leak detection off for every fuzz/standalone binary (SPEC §6.2 item 15); ASan + UBSan stay on.
extern "C" int __lsan_is_turned_off() { return 1; }
