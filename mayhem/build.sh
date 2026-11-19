#!/usr/bin/env bash
# mayhem/build.sh — retdec fuzz harnesses + upstream unit tests. Runs as `mayhem` in /mayhem,
# offline: avast's LLVM fork and googletest come from /opt/toolchains/deps (mayhem/Dockerfile).
#
#  1. build-fuzz/  : upstream's tree (via mayhem/fuzz/CMakeLists.txt) with $SANITIZER_FLAGS,
#                    SanCov (-fsanitize=fuzzer-no-link) and $DEBUG_FLAGS on every retdec library,
#                    plus fuzz_<t> (libFuzzer) and fuzz_<t>-standalone per target -> /mayhem/fuzz_<t>.
#                    mayhem/fuzz/CMakeLists.txt compiles mayhem/lsan_off.cc (LSan off, SPEC §6.2
#                    item 15) with these flags and links it into every fuzz and standalone binary.
#  2. build-tests/ : upstream's tree with RETDEC_TESTS=ON and its normal flags -> the googletest
#                    binaries mayhem/test.sh runs.
# Both build dirs match upstream's `/build*/` .gitignore, so `git clean -ffdX` removes them.
set -euo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${STANDALONE_FUZZ_MAIN:=/opt/mayhem/StandaloneFuzzTargetMain.c}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE STANDALONE_FUZZ_MAIN MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"

COMPONENTS="fileformat,loader,pelib,pdbparser,demangler,ar-extractor,extractor,config,ctypesparser"
LLVM_SHIM="$SRC/mayhem/llvm-prebuilt"
GTEST_DIR=/opt/toolchains/deps/googletest-90a443f9c2437ca8a682a1ac625eba64e1d74a8a/src

# SanCov on the project code itself (not only the harness), so the fuzzer sees retdec's edges.
SANCOV=""
case "$SANITIZER_FLAGS" in *address*|*undefined*) SANCOV="-fsanitize=fuzzer-no-link" ;; esac
# Two UBSan checks are relaxed (only these; ASan and the rest of UBSan halt), because they fire on
# nearly every input before any parsing happens:
#  - pointer-overflow: rapidjson's FindMember on a non-object value (document.h:1200) and PeLib's
#    `fileData.data() + size` on an empty buffer (ImageLoader.cpp:1654) compute `nullptr + n`
#    without dereferencing it;
#  - alignment: utils/crc32.cpp (slicing-by-8, line 360) loads uint32_t from unaligned file bytes
#    while hashing every loaded file (fine on x86).
RELAX=""
case "$SANITIZER_FLAGS" in *undefined*) RELAX="-fno-sanitize=pointer-overflow,alignment" ;; esac

# 1) Sanitized fuzz build.
cmake -G Ninja -S mayhem/fuzz -B build-fuzz \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_C_FLAGS="$SANITIZER_FLAGS $RELAX $SANCOV $DEBUG_FLAGS" \
	-DCMAKE_CXX_FLAGS="$SANITIZER_FLAGS $RELAX $SANCOV $DEBUG_FLAGS" \
	-DRETDEC_ENABLE="$COMPONENTS" -DRETDEC_TESTS=OFF -DRETDEC_COMPILE_YARA=OFF \
	-DLLVM_LOCAL_DIR="$LLVM_SHIM"
cmake --build build-fuzz -j"$MAYHEM_JOBS"

for f in build-fuzz/fuzz_*; do
	[ -x "$f" ] && [ -f "$f" ] && install -m 0755 "$f" "$SRC/$(basename "$f")"
done

# 2) Upstream's googletest suite, normal flags (independent of the sanitized build).
cmake -G Ninja -S . -B build-tests \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_C_FLAGS="$COVERAGE_FLAGS" -DCMAKE_CXX_FLAGS="$COVERAGE_FLAGS" \
	-DRETDEC_ENABLE="$COMPONENTS" -DRETDEC_TESTS=ON -DRETDEC_COMPILE_YARA=OFF \
	-DLLVM_LOCAL_DIR="$LLVM_SHIM" -DGOOGLETEST_LOCAL_DIR="$GTEST_DIR"
cmake --build build-tests -j"$MAYHEM_JOBS" || {
	# upstream's ExternalProjects log to files; show them so a failure is diagnosable.
	tail -n 40 build-tests/external/src/*-stamp/*-build-*.log 2>/dev/null; exit 1; }
