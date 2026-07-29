# Binary audit

## Stock Linux x86 `server_srv.so`

```text
SHA-256: 4b4d8c167d3dbab47a1cccd2b9b6cd2eb01646ca774c36dd9e1684e589e301f6
GNU Build ID: 68524484e01a8694ef6fc7a814795eaf18d4d650
CTriggerWeaponDissolve::DissolveThink: 0x0088EEA0
m_pWeapons data offset: 0x4F8
m_pWeapons count offset: 0x504
```

Signature:

```text
55 89 E5 57 56 53 83 EC 7C 8B 5D 08 8B 83 ?? ?? ?? ?? 85 C0 89 45 9C 0F 8E ?? ?? ?? ?? C7 45 90 00 00 00 00 8D 7D B0 83 F8 01
```

The pattern resolves exactly once in the supplied binary.

## Stock Windows x86 `server.dll`

```text
SHA-256: 4e6f93c09a0088fda516992ea70ea170c0f3ffb455db586ebc9244878aa8052f
Preferred image base: 0x10000000
Function VA: 0x102F7B00
Function RVA: 0x002F7B00
File offset: 0x002F6F00
m_pWeapons data offset: 0x4E0
m_pWeapons count offset: 0x4EC
```

Signature:

```text
55 8B EC 83 EC 40 53 56 57 8B F9 33 F6 89 75 FC 8B 97 ?? ?? ?? ?? 89 55 F8 85 D2 7E ?? 8D 49 00 8B 87 ?? ?? ?? ?? 8B 0C B0 85 C9 74 ?? BA FF 1F 00 00
```

The pattern resolves exactly once in the supplied binary.

## Offset validation

The plugin does not blindly hardcode `m_pWeapons`.

After resolving the function signature, it reads and validates both machine-code member displacements:

```text
vector data pointer
vector element count
```

The count must be exactly 12 bytes after the vector data field, matching the x86 `CUtlVector` layout. A mismatch prevents the detour from being installed.

## Linux plugin binary

The release binary is:

```text
ELF 32-bit LSB shared object
Machine: Intel 80386
Exported Metamod entry point: CreateInterface
```

Funchook and Capstone are linked statically. The plugin has no runtime dependency on separate Funchook or Capstone shared libraries.

Release binary hashes:

```text
hl2mp_lowlevel_fixes.so
SHA-256: 9226e27b89822c28ea517a21d24464a76d68dc236f58a2979f51c5bcfd0bc6c1

hl2mp_lowlevel_fixes.so.debug
SHA-256: f9e980bb43045e3c25951142be1db042770ee51428ccad81be3b339d8db6fd27
```

The release binary exports only `CreateInterface` as a defined global dynamic symbol. A GNU debug link points to the separate symbol file.
