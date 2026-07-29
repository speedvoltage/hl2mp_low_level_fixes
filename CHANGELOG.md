# Changelog

## 0.1.0

- Added a native Funchook detour for `CTriggerWeaponDissolve::DissolveThink()`.
- Removed stale weapon handles before the stock function dereferences them.
- Preserved the complete stock dissolve implementation and timing.
- Added independent Linux x86 and Windows x86 signatures.
- Added machine-code validation for the private vector data/count offsets.
- Added Linux x86 CMake build and Visual Studio 2022 Win32 build support.
- Statically linked Funchook 2.0.0 and Capstone 5.0.1.
