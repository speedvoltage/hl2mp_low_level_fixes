#!/usr/bin/env python3
import argparse
import hashlib
import struct
from pathlib import Path

LINUX_PATTERN = bytes.fromhex("55 89 E5 57 56 53 83 EC 7C 8B 5D 08 8B 83 00 00 00 00 85 C0 89 45 9C 0F 8E 00 00 00 00 C7 45 90 00 00 00 00 8D 7D B0 83 F8 01")
LINUX_MASK = "xxxxxxxxxxxxxx????xxxxxxx????xxxxxxxxxxxxx"
WINDOWS_PATTERN = bytes.fromhex("55 8B EC 83 EC 40 53 56 57 8B F9 33 F6 89 75 FC 8B 97 00 00 00 00 89 55 F8 85 D2 7E 00 8D 49 00 8B 87 00 00 00 00 8B 0C B0 85 C9 74 00 BA FF 1F 00 00")
WINDOWS_MASK = "xxxxxxxxxxxxxxxxxx????xxxxxx?xxxxx????xxxxxx?xxxxx"


def matches(data, offset, pattern, mask):
    return all(mask[i] != "x" or data[offset + i] == pattern[i] for i in range(len(pattern)))


def scan(data, pattern, mask):
    return [i for i in range(0, len(data) - len(pattern) + 1) if matches(data, i, pattern, mask)]


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def verify_linux(path):
    data = path.read_bytes()
    hits = scan(data, LINUX_PATTERN, LINUX_MASK)
    print(f"Linux: {path}")
    print(f"  SHA-256: {sha256(data)}")
    print(f"  Signature matches: {len(hits)}")
    if len(hits) != 1:
        return False
    hit = hits[0]
    count_offset = struct.unpack_from("<I", data, hit + 14)[0]
    data_offset = count_offset - 12
    found = any(
        data[hit + i:hit + i + 2] == b"\x8b\x83" and
        struct.unpack_from("<I", data, hit + i + 2)[0] == data_offset
        for i in range(0, 315)
    )
    if not found:
        print("  Vector data instruction validation failed")
        return False
    print(f"  File offset: 0x{hit:X}")
    print(f"  m_pWeapons data: 0x{data_offset:X}")
    print(f"  m_pWeapons count: 0x{count_offset:X}")
    return data_offset + 12 == count_offset


def pe_rva(data, file_offset):
    if data[:2] != b"MZ":
        return None
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe:pe + 4] != b"PE\0\0":
        return None
    sections = struct.unpack_from("<H", data, pe + 6)[0]
    optional_size = struct.unpack_from("<H", data, pe + 20)[0]
    section = pe + 24 + optional_size
    for _ in range(sections):
        virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from("<IIII", data, section + 8)
        size = max(virtual_size, raw_size)
        if raw_offset <= file_offset < raw_offset + size:
            return virtual_address + file_offset - raw_offset
        section += 40
    return None


def verify_windows(path):
    data = path.read_bytes()
    hits = scan(data, WINDOWS_PATTERN, WINDOWS_MASK)
    print(f"Windows: {path}")
    print(f"  SHA-256: {sha256(data)}")
    print(f"  Signature matches: {len(hits)}")
    if len(hits) != 1:
        return False
    hit = hits[0]
    count_offset = struct.unpack_from("<I", data, hit + 18)[0]
    data_offset = count_offset - 12
    found = any(
        data[hit + i:hit + i + 2] == b"\x8b\x87" and
        struct.unpack_from("<I", data, hit + i + 2)[0] == data_offset
        for i in range(0, 315)
    )
    if not found:
        print("  Vector data instruction validation failed")
        return False
    rva = pe_rva(data, hit)
    print(f"  File offset: 0x{hit:X}")
    if rva is not None:
        print(f"  RVA: 0x{rva:X}")
    print(f"  m_pWeapons data: 0x{data_offset:X}")
    print(f"  m_pWeapons count: 0x{count_offset:X}")
    return data_offset + 12 == count_offset


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--linux", type=Path)
    parser.add_argument("--windows", type=Path)
    args = parser.parse_args()
    if not args.linux and not args.windows:
        parser.error("provide --linux and/or --windows")
    okay = True
    if args.linux:
        okay = verify_linux(args.linux) and okay
    if args.windows:
        okay = verify_windows(args.windows) and okay
    raise SystemExit(0 if okay else 1)


if __name__ == "__main__":
    main()
