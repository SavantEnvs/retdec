# syntax=docker/dockerfile:1.7
#
# Commit image for retdec (avast/retdec): the fileformat/pelib/loader/pdbparser/demangler/
# ar-extractor/macho-extractor/config/ctypesparser libraries, ASan+UBSan instrumented, with one
# libFuzzer harness per input surface (mayhem/fuzz/*.cpp) and upstream's googletest suite.
#
# Build (from the repo root):
#   DOCKER_BUILDKIT=1 docker build -f mayhem/Dockerfile --build-arg COMMIT=$(git rev-parse HEAD) -t retdec .
FROM ghcr.io/savantenvs/base:20261023

ARG COMMIT
ARG REPO_URL
ARG SOURCE_DATE_EPOCH
ARG SANITIZER_FLAGS
ARG COVERAGE_FLAGS=
ARG MAYHEM_JOBS=

# --- External dependencies (SPEC §6.2 item 19) -------------------------------------------------
# Upstream's CMake downloads these with ExternalProject (cmake/deps.cmake). They are fetched here
# once, pinned by the same URL + SHA256 upstream pins, so build.sh runs offline.
USER root

# avast's LLVM fork (cmake/deps.cmake LLVM_URL). It is the slow part of the build and is not the
# fuzz target, so it is built ONCE here, uninstrumented, with upstream's own configuration
# (deps/llvm/CMakeLists.txt), and installed to $d. build.sh hands it to upstream's ExternalProject
# through mayhem/llvm-prebuilt (a source dir whose install step copies this prefix), so the
# retdec build links the exact same libraries without recompiling LLVM.
ARG LLVM_REV=a776c2a976ef64d9cd84d7ee71d0e4a04aa117a1
ARG LLVM_SHA256=b5879b30768135e5fce84ccd8be356d2c55c940ab32ceb22d278b228e88c4c60
RUN set -eux; d=/opt/toolchains/deps/llvm-${LLVM_REV}; mkdir -p "$d/src" /tmp/llvm-build; \
    curl -fsSL -o /tmp/llvm.zip https://github.com/avast/llvm/archive/${LLVM_REV}.zip; \
    echo "${LLVM_SHA256}  /tmp/llvm.zip" | sha256sum -c -; \
    cd /tmp && unzip -q llvm.zip && cp -a llvm-${LLVM_REV}/. "$d/src/" && rm -rf llvm-${LLVM_REV} llvm.zip; \
    cd /tmp/llvm-build; \
    cmake -G Ninja "$d/src" -DCMAKE_INSTALL_PREFIX="$d" -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_C_FLAGS="-gline-tables-only -gdwarf-3" -DCMAKE_CXX_FLAGS="-gline-tables-only -gdwarf-3 -include cstdint" \
      -DPYTHON_EXECUTABLE=/usr/bin/python3 -DLLVM_TARGETS_TO_BUILD=X86 -DLLVM_REQUIRES_RTTI=YES \
      -DLLVM_ENABLE_ASSERTIONS=NO -DLLVM_ENABLE_WARNINGS=NO -DLLVM_ENABLE_RTTI=ON -DLLVM_ENABLE_EH=ON \
      -DLLVM_INCLUDE_TOOLS=OFF -DLLVM_INCLUDE_UTILS=OFF -DLLVM_INCLUDE_RUNTIMES=OFF -DLLVM_INCLUDE_EXAMPLES=OFF \
      -DLLVM_INCLUDE_TESTS=OFF -DLLVM_INCLUDE_GO_TESTS=OFF -DLLVM_INCLUDE_BENCHMARKS=OFF -DLLVM_INCLUDE_DOCS=OFF \
      -DLLVM_BUILD_TOOLS=OFF -DLLVM_BUILD_UTILS=OFF -DLLVM_BUILD_RUNTIMES=OFF -DLLVM_BUILD_RUNTIME=OFF \
      -DLLVM_BUILD_EXAMPLES=OFF -DLLVM_BUILD_TESTS=OFF -DLLVM_BUILD_BENCHMARKS=OFF -DLLVM_BUILD_DOCS=OFF \
      -DLLVM_ENABLE_BINDINGS=OFF -DLLVM_ENABLE_TERMINFO=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON; \
    ninja -j"${MAYHEM_JOBS:-$(nproc)}" install; \
    cd / && rm -rf /tmp/llvm-build; chmod -R a+rX "$d"

# googletest (cmake/deps.cmake GOOGLETEST_URL): source only; upstream's ExternalProject builds it
# from here (GOOGLETEST_LOCAL_DIR) for the unit-test suite mayhem/test.sh runs. Its 2019 CMake builds
# itself with -Werror, which clang-19's -Wdeprecated-copy (from gmock headers) turns into a build
# failure; the one-line sed drops -Werror from googletest's own flags (warnings stay visible).
ARG GTEST_REV=90a443f9c2437ca8a682a1ac625eba64e1d74a8a
ARG GTEST_SHA256=6fb9a49ad77656c860cfdafbb3148a91f076a3a8bda9c6d8809075c832549dd4
RUN set -eux; d=/opt/toolchains/deps/googletest-${GTEST_REV}; mkdir -p "$d/src"; \
    curl -fsSL -o /tmp/gtest.zip https://github.com/google/googletest/archive/${GTEST_REV}.zip; \
    echo "${GTEST_SHA256}  /tmp/gtest.zip" | sha256sum -c -; \
    cd /tmp && unzip -q gtest.zip && cp -a googletest-${GTEST_REV}/. "$d/src/" && rm -rf googletest-${GTEST_REV} gtest.zip; \
    sed -i 's/ -Werror//' "$d/src/googletest/cmake/internal_utils.cmake"; \
    chmod -R a+rX "$d"

USER mayhem
WORKDIR /mayhem
COPY --chown=2000:2000 . /mayhem

RUN if [ -x /opt/mayhem/ccache/prefill ]; then /opt/mayhem/ccache/prefill bash mayhem/build.sh; \
    else bash mayhem/build.sh; fi

RUN if [ -f mayhem/test.sh ]; then bash mayhem/test.sh; else echo "no mayhem/test.sh"; fi

LABEL org.opencontainers.image.source="${REPO_URL}" \
      org.opencontainers.image.revision="${COMMIT}" \
      rlenv.tier="commit" \
      rlenv.source_date_epoch="${SOURCE_DATE_EPOCH}" \
      rlenv.user="mayhem"

USER mayhem
WORKDIR /mayhem
