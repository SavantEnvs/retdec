# Fuzz build of retdec: upstream's tree as a subproject (unchanged, its own options) plus one
# libFuzzer harness and one standalone reproducer per target, linked against retdec's static libs.
cmake_minimum_required(VERSION 3.13)
project(retdec_fuzz LANGUAGES C CXX)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. retdec)

set(FUZZ_TARGETS
	pe_format=retdec::fileformat
	elf_format=retdec::fileformat
	macho_format=retdec::fileformat
	coff_format=retdec::fileformat
	format_factory=retdec::fileformat
	loader_image=retdec::loader
	pelib_image=retdec::pelib,retdec::utils
	pdb_parser=retdec::pdbparser
	ar_extractor=retdec::ar-extractor
	macho_extractor=retdec::macho-extractor
	demangler=retdec::demangler
	config_json=retdec::config
	ctypes_json=retdec::ctypesparser
)

add_library(fuzz_lsan_off OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/../lsan_off.cc)
add_library(fuzz_standalone_main STATIC $ENV{STANDALONE_FUZZ_MAIN})
set_source_files_properties($ENV{STANDALONE_FUZZ_MAIN} PROPERTIES LANGUAGE C)

foreach(t ${FUZZ_TARGETS})
	string(REPLACE "=" ";" parts ${t})
	list(GET parts 0 name)
	list(GET parts 1 lib)
	string(REPLACE "," ";" lib "${lib}")
	add_executable(fuzz_${name} fuzz_${name}.cpp)
	target_link_libraries(fuzz_${name} PRIVATE ${lib} fuzz_lsan_off)
	target_link_options(fuzz_${name} PRIVATE $ENV{LIB_FUZZING_ENGINE})
	add_executable(fuzz_${name}-standalone fuzz_${name}.cpp)
	target_link_libraries(fuzz_${name}-standalone PRIVATE ${lib} fuzz_lsan_off fuzz_standalone_main)
endforeach()
