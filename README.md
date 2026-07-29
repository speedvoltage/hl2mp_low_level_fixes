# HL2MP - Low-Level Fixes

Native Metamod:Source fixes for Half-Life 2: Deathmatch issues that can't be safely handled from SourcePawn.

## Fixes

- **`trigger_weapon_dissolve` crash** — the stock `CTriggerWeaponDissolve::DissolveThink()` doesn't prune its own weapon list as tracked weapons get destroyed elsewhere, so it can dereference a stale, dead entry. This plugin validates each entry against the game's own live entity list immediately before the stock function runs, and drops anything no longer valid. The stock function itself is untouched otherwise.

## Requirements

- Half-Life 2: Deathmatch dedicated server (32-bit)
- Metamod:Source 2.0
- SourceMod not required

## Installation

Copy into your HL2DM game directory:

```text
addons/metamod/hl2mp_lowlevel_fixes.vdf
addons/hl2mp_lowlevel_fixes/bin/hl2mp_lowlevel_fixes.so   (or .dll on Windows)
```

Restart the server, then confirm with:

```text
meta list
```

## Building

**Linux:**
```bash
export MMSOURCE_ROOT=/path/to/metamod-source
export HL2SDK_ROOT=/path/to/hl2sdk-hl2dm
./build_linux_x86.sh
```

**Windows:** see `docs/BUILD_WINDOWS.md`.

Both produce output under `dist/<platform>/`, matching the installation layout above. Funchook and Capstone are vendored and linked statically.

## License

GPLv2 — see `LICENSE`. Third-party components listed in `THIRD_PARTY_NOTICES.md`.