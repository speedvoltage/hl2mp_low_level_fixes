# Building the Windows x86 DLL with Visual Studio 2022

## 1. Install the required Visual Studio components

Open Visual Studio Installer and ensure the following are installed:

```text
Desktop development with C++
MSVC v143 C++ x86/x64 build tools
Windows 10 SDK or Windows 11 SDK
C++ CMake tools for Windows
```

The project uses MASM through Funchook, so the MSVC x86 tools must be present.

## 2. Prepare the dependency trees

You need:

```text
Metamod:Source 2.0 source tree
HL2DM SDK tree
```

`MMSOURCE_ROOT` must contain:

```text
core\ISmmPlugin.h
```

`HL2SDK_ROOT` may point either to:

```text
Valve Source SDK 2013 ...\src
```

or to the root of AlliedModders' `hl2sdk` HL2DM branch containing:

```text
public\eiface.h
```

Funchook and Capstone are already included in this source package. Do not download them separately.

## 3. Open a Visual Studio developer prompt

Use:

```text
Developer Command Prompt for VS 2022
```

An x64-hosted developer prompt is fine because CMake is explicitly configured with `-A Win32`.

## 4. Set the paths

Example:

```bat
set MMSOURCE_ROOT=C:\Development\metamod-source
set HL2SDK_ROOT=C:\Development\source-sdk-2013\src
```

Do not include an extra pair of quotes in the environment-variable value.

## 5. Build

From the project directory:

```bat
build_windows_x86.bat
```

The script runs the equivalent of:

```bat
cmake -S . -B build\windows-x86 -A Win32 -DMMSOURCE_ROOT="%MMSOURCE_ROOT%" -DHL2SDK_ROOT="%HL2SDK_ROOT%"
cmake --build build\windows-x86 --config Release
cmake --install build\windows-x86 --config Release --prefix dist\windows-x86
```

## 6. Result

The installable files will be:

```text
dist\windows-x86\addons\hl2mp_lowlevel_fixes.vdf
dist\windows-x86\addons\hl2mp_lowlevel_fixes\bin\hl2mp_lowlevel_fixes.dll
```

A PDB should also be produced under the Visual Studio build directory. Keep it for crash analysis; it does not need to be installed on the server.

## 7. Install and verify

Copy the `addons` directory into the HL2DM game directory and restart the server.

Run:

```text
meta list
meta info <plugin-id>
```

The Windows startup line should report:

```text
m_pWeapons offset 0x4E0
```

## Troubleshooting

### CMake chooses x64

Delete `build\windows-x86` and rerun the provided script. The project intentionally refuses a 64-bit configuration.

### MASM is missing

Modify the Visual Studio installation and add the MSVC x86/x64 build tools. Funchook's x86 trampoline uses `ml.exe`.

### `ISmmPlugin.h` is missing

`MMSOURCE_ROOT` is pointing at the runtime package rather than the Metamod:Source source tree.

### `eiface.h` is missing

Point `HL2SDK_ROOT` at the SDK root containing `public\eiface.h`, or at Valve's `src` directory.
