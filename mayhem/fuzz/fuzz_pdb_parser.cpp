// libFuzzer harness: retdec::pdbparser::PDBFile::load_pdb_file + initialize -- MSF streams, type,
// symbol, module and global-variable records of a PDB (the path-only API gets a temp file).
#include <cstddef>
#include <cstdint>

#include "retdec/pdbparser/pdb_file.h"
#include "fuzz_tempfile.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
	fuzz::TempFile tf(data, size, ".pdb");
	try {
		retdec::pdbparser::PDBFile pdb;
		if (pdb.load_pdb_file(tf.path.c_str()) != retdec::pdbparser::PDB_STATE_OK) return 0;
		pdb.initialize();
		(void)pdb.get_types_container();
		(void)pdb.get_functions();
		(void)pdb.get_global_variables();
	} catch (...) {
	}
	return 0;
}
