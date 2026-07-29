# HL2MP - Low-Level Fixes

Native Metamod:Source fixes for Half-Life 2: Deathmatch problems that are not cleanly or safely handled in SourcePawn.

## Version 0.1.0

This first release contains one fix:

- Sanitizes stale `CHandle<CBaseCombatWeapon>` entries in `CTriggerWeaponDissolve::m_pWeapons` immediately before the stock `DissolveThink()` function runs.

The stock function resolves each handle to a weapon pointer and immediately passes that pointer into `GetConduitPoint()`. If the weapon was removed after being collected by the trigger, the handle resolves to `NULL` and the stock function dereferences it.

The plugin removes only invalid handles and then calls the untouched stock function. It does not replace the dissolve logic or alter its timing.

## Requirements

- Half-Life 2: Deathmatch dedicated server
- Metamod:Source 2.0
- 32-bit server process

SourceMod is not required.

## Linux installation

Copy these files into the HL2DM game directory:

```text
addons/hl2mp_lowlevel_fixes.vdf
addons/hl2mp_lowlevel_fixes/bin/hl2mp_lowlevel_fixes.so
```

Restart the server completely, then run:

```text
meta list
meta info <plugin-id>
```

Expected startup output includes:

```text
[HL2MP-LLF] Loaded. DissolveThink <address>; m_pWeapons offset 0x4F8.
```

## Windows build

Visual Studio 2022 is supported. See `docs/BUILD_WINDOWS.md`.

## Linux build

Required environment variables:

```bash
export MMSOURCE_ROOT=/path/to/metamod-source
export HL2SDK_ROOT=/path/to/hl2sdk-or-source-sdk/src
```

Build:

```bash
./build_linux_x86.sh
```

Output:

```text
dist/linux-x86/addons/hl2mp_lowlevel_fixes.vdf
dist/linux-x86/addons/hl2mp_lowlevel_fixes/bin/hl2mp_lowlevel_fixes.so
dist/linux-x86/symbols/linux-x86/hl2mp_lowlevel_fixes.so.debug
```

The Linux build requires a working 32-bit Clang/GCC toolchain. Funchook and Capstone are included and linked statically.

## Design

The plugin locates `CTriggerWeaponDissolve::DissolveThink()` with a platform-specific signature. It independently validates the machine-code offsets used to access the vector data and vector count. If the signature, offsets, detour, or vector layout do not validate, the plugin refuses to enable the fix or stops the unsafe call path.

## Supported binaries

The supplied signatures were validated against the stock anniversary x86 game DLLs listed in `docs/BINARY_AUDIT.md`.

## Unloading

The plugin supports normal Metamod unloading and removes its Funchook detour. A full server restart is still recommended when replacing the binary.

## Scope

Version 0.1.0 deliberately contains only the weapon-dissolve fix. The 64-bit Steam/dedicated shutdown correction is deferred.
