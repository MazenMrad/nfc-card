# Build and test instructions

## Build configuration

The supplied binaries use this configuration:

- Godot API: 4.6, official single-precision build.
- `godot-cpp` commit: `6cceaf6a5f8b0d78ac5d71c139fd7fabba43b918`.
- Compiler: LLVM-MinGW 22.1.8 for Windows x86-64 with UCRT.
- Python: 3.12.6.
- SCons: 4.11.1.
- Godot test version: 4.6.3 stable.

The build profile includes `RefCounted`, `OS`, and their required Godot classes.

## Prerequisites

Install these tools:

- Python 3.8 or a later version.
- SCons 4 or a later version.
- Git.
- A Windows x86-64 MinGW-compatible compiler.

The supplied binaries use the `MartinStorsjo.LLVM-MinGW.UCRT` winget package.

## Get `godot-cpp`

If the `native/godot-cpp` directory does not exist, do these steps:

```powershell
git clone https://github.com/godotengine/godot-cpp.git native/godot-cpp
git -C native/godot-cpp checkout 6cceaf6a5f8b0d78ac5d71c139fd7fabba43b918
```

## Build the DLL files

From the `native` directory, run these commands:

```powershell
python -m SCons platform=windows arch=x86_64 target=template_debug --jobs=4
python -m SCons platform=windows arch=x86_64 target=template_release --jobs=4
```

Alternatively, run this command:

```powershell
.\build.ps1
```

The build puts the DLL files in `addons/nfc_windows/bin`.

The Windows build uses static GCC and C++ run-time libraries. The DLL files use only Windows system DLL files.

The supplied DLL files do not have digital signatures.

## Run the native tests

From the `native` directory, run this command:

```powershell
.\run_tests.ps1
```

The tests use a simulated PC/SC transport. The tests use the production protocol and controller code.

## Run the Godot tests

From the project root, run these commands with Godot 4.6:

```powershell
godot --headless --path . --script res://tests/godot_smoke.gd --quit
godot --headless --path . --script res://tests/demo_smoke.gd --quit
```

The first test loads the GDExtension and checks its public class.

The second test loads the demo scene and creates its nodes.

These tests do not prove reader-driver or radio-frequency compatibility.
