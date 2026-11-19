#!/usr/bin/env python3
"""Seed generator for the pdb_parser target (not used by the build).

retdec's PDBFile::load_pdb_v700 rejects any file whose length is not exactly
page_size * NumPages, so a seed truncated by the fuzzer's max input length never gets past the
MSF header. LLVM's test PDBs (1-4 KiB pages, 35-400 KiB) are repacked here into small MSF 7.00
files with 512-byte pages holding only the streams retdec reads (PDB info, TPI, DBI, symbol
records, section headers, module symbol streams):

  repack_pdb.py compact IN.pdb OUT.pdb   every module, section headers kept
  repack_pdb.py tiny IN.pdb OUT.pdb N    4096 bytes: N-th module only, each stream cut at a
                                         record boundary to one page

Inputs: /opt/toolchains/deps/llvm-<rev>/src/test/{tools/llvm-pdbdump,DebugInfo/PDB}/Inputs/*.pdb
(the pinned LLVM source in the image). The committed testsuite is the libFuzzer -merge=1 minimised
union of both forms.
"""
import struct
import sys

MAGIC = b"Microsoft C/C++ MSF 7.00\r\n\x1aDS\0\0\0"
def msf_read(path):
    d = open(path,'rb').read()
    assert d[:32] == MAGIC
    bs, fpm, nb, dirsz, _, bma = struct.unpack_from('<6I', d, 32)
    ndb = (dirsz + bs - 1)//bs
    dblocks = struct.unpack_from('<%dI'%ndb, d, bma*bs)
    dr = b''.join(d[b*bs:(b+1)*bs] for b in dblocks)[:dirsz]
    n = struct.unpack_from('<I', dr, 0)[0]
    sizes = struct.unpack_from('<%dI'%n, dr, 4)
    off = 4+4*n; streams=[]
    for s in sizes:
        if s == 0xFFFFFFFF or s == 0: streams.append((s,b'')); continue
        k=(s+bs-1)//bs; bl=struct.unpack_from('<%dI'%k, dr, off); off+=4*k
        streams.append((s, b''.join(d[b*bs:(b+1)*bs] for b in bl)[:s]))
    return bs, streams
def msf_write(path, streams, bs=512, fpm=True):
    base = 3 if fpm else 2  # first directory block
    # block 0 superblock, 1 FPM, 2 block map, then directory, then streams (all contiguous)
    nblk = lambda s: (s+bs-1)//bs
    sizes=[s for s,_ in streams]
    # directory size depends on stream block lists
    nsb = sum(nblk(len(b)) for _,b in streams)
    dirsz = 4 + 4*len(streams) + 4*nsb
    ndb = nblk(dirsz); assert ndb*4 <= bs
    cur = base + ndb; lists=[]
    for s,b in streams:
        k = nblk(len(b)); lists.append(list(range(cur,cur+k))); cur+=k
    total = cur
    dr = struct.pack('<I', len(streams)) + struct.pack('<%dI'%len(streams), *sizes)
    for l in lists: dr += struct.pack('<%dI'%len(l), *l)
    out = bytearray(total*bs)
    out[:32]=MAGIC; struct.pack_into('<6I', out, 32, bs, 1, total, len(dr), 0, base - 1)
    struct.pack_into('<%dI'%ndb, out, (base-1)*bs, *range(base,base+ndb))
    out[base*bs:base*bs+len(dr)] = dr
    for (s,b),l in zip(streams,lists):
        if l: out[l[0]*bs:l[0]*bs+len(b)] = b
    open(path,'wb').write(out)

def compact(src, dst):
    bs, st = msf_read(src)
    dbi = bytearray(st[3][1])
    gs, ps, sr = (struct.unpack_from('<H', dbi, o)[0] for o in (12, 16, 20))
    cbmod, cbsc, cbsm, cbfi, cbts, _, cbdbg, cbec = struct.unpack_from('<8I', dbi, 24)
    mods = bytearray(dbi[64:64 + cbmod]); dbg = bytearray(dbi[len(dbi) - cbdbg:])
    out = [(0, b''), st[1], st[2], None, st[sr]]
    sec = struct.unpack_from('<H', dbg, 10)[0]
    out.append(st[sec] if sec != 0xffff else (0, b""))
    off = 0
    while off + 64 <= len(mods):
        sn = struct.unpack_from('<H', mods, off + 34)[0]
        p = off + 64
        for _ in range(2): p = mods.index(b'\0', p) + 1
        end = (p + 3) & ~3
        if sn != 0xffff:
            struct.pack_into('<H', mods, off + 34, len(out)); out.append(st[sn]); off = end
        else:
            del mods[off:end]
    dbg = bytearray(b'\xff\xff' * (len(dbg) // 2)); struct.pack_into("<H", dbg, 10, 5)
    hdr = bytearray(dbi[:64])
    for o in (12, 16, 20): struct.pack_into('<H', hdr, o, 4)
    struct.pack_into('<8I', hdr, 24, len(mods), 0, 0, 0, 0, 0, len(dbg), 0)
    d = bytes(hdr + mods + dbg); out[3] = (len(d), d)
    msf_write(dst, out)

def tiny(src, dst, modpick):
    P = 512
    bs, st = msf_read(src)
    dbi = bytearray(st[3][1])
    sr = struct.unpack_from('<H', dbi, 20)[0]
    cbmod, *_r = struct.unpack_from('<8I', dbi, 24); cbdbg = _r[5]
    mods = bytearray(dbi[64:64 + cbmod]); dbg = dbi[len(dbi) - cbdbg:]
    tpi = bytearray(st[2][1])
    hs = struct.unpack_from('<I', tpi, 4)[0]
    tb, te = struct.unpack_from('<2I', tpi, 8)
    t2, n = cut_records(tpi, hs, P)
    t2 = bytearray(t2); struct.pack_into('<2I', t2, 12, tb + n, len(t2) - hs)
    sym, _ = cut_records(st[sr][1], 0, P)
    # pick the modpick-th module with a symbol stream
    off = 0; chosen = None; k = 0
    while off + 64 <= len(mods):
        sn = struct.unpack_from('<H', mods, off + 34)[0]
        p = off + 64
        for _ in range(2): p = mods.index(b'\0', p) + 1
        end = (p + 3) & ~3
        if sn != 0xffff:
            if k == modpick: chosen = (bytearray(mods[off:end]), sn)
            k += 1
        off = end
    if chosen is None: raise SystemExit(1)
    m, sn = chosen
    ms = st[sn][1]
    body, _ = cut_records(ms, 4, P)
    struct.pack_into('<H', m, 34, 5); struct.pack_into('<3I', m, 36, len(body), 0, 0)
    dbgn = bytearray(b'\xff\xff' * (len(dbg) // 2)); struct.pack_into('<H', dbgn, 10, 0)
    hdr = bytearray(dbi[:64])
    for o in (12, 16, 20): struct.pack_into('<H', hdr, o, 4)
    struct.pack_into('<8I', hdr, 24, len(m), 0, 0, 0, 0, 0, len(dbgn), 0)
    d = bytes(hdr + m + dbgn)
    if len(d) > P or len(st[1][1]) > P: raise SystemExit(1)
    out = [(0, b''), st[1], (len(t2), bytes(t2)), (len(d), d), (len(sym), bytes(sym)), (len(body), bytes(body))]
    msf_write(dst, out, P, fpm=False)

def cut_records(data, start, limit, step_extra=2):
    off = start; last = start; n = 0
    while off + 2 <= len(data):
        l = struct.unpack_from('<H', data, off)[0]
        if l == 0 or off + 2 + l > limit: break
        off += 2 + l; last = off; n += 1
    return data[:last], n

if __name__ == '__main__':
    if sys.argv[1] == 'compact':
        compact(sys.argv[2], sys.argv[3])
    else:
        tiny(sys.argv[2], sys.argv[3], int(sys.argv[4]))
