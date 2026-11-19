# Stand-in "LLVM source tree" for upstream's deps/llvm ExternalProject (passed as LLVM_LOCAL_DIR).
# avast's LLVM fork is built once in mayhem/Dockerfile, with upstream's own options, into
# /opt/toolchains/deps/llvm-<rev>; this project's install step links that prefix into the
# ExternalProject's install dir, so the retdec build uses those libraries instead of rebuilding LLVM.
cmake_minimum_required(VERSION 3.13)
project(llvm-prebuilt NONE)
set(LLVM_PREBUILT_DIR /opt/toolchains/deps/llvm-a776c2a976ef64d9cd84d7ee71d0e4a04aa117a1)
if(NOT EXISTS ${LLVM_PREBUILT_DIR}/lib/libLLVMSupport.a)
	message(FATAL_ERROR "prebuilt LLVM not found in ${LLVM_PREBUILT_DIR} (built by mayhem/Dockerfile)")
endif()
install(CODE "
	file(MAKE_DIRECTORY \"\${CMAKE_INSTALL_PREFIX}\")
	foreach(d include lib)
		# ninja pre-creates lib/ for the ExternalProject's BUILD_BYPRODUCTS: replace that empty dir.
		set(dst \"\${CMAKE_INSTALL_PREFIX}/\${d}\")
		if(IS_DIRECTORY \"\${dst}\" AND NOT IS_SYMLINK \"\${dst}\")
			file(REMOVE_RECURSE \"\${dst}\")
		endif()
		execute_process(COMMAND ln -sfn ${LLVM_PREBUILT_DIR}/\${d} \"\${CMAKE_INSTALL_PREFIX}/\${d}\" COMMAND_ERROR_IS_FATAL ANY)
	endforeach()
")
