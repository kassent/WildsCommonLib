# WildsCommonLib

Shared native code used by REClassInjector and UserDataExtension for Monster Hunter Wilds.

- `include/`, `src/`: shared engine types, VM calls, reference handling, hooks, trampolines, configuration, logging, and bundled dependencies.
- `test/`: offline managed-call and allocation contract tests; no game process is required.
- `common_lib`: CMake target consumed by the plugin repositories through their pinned `common/` submodule.

## Build

Windows x64, Visual Studio 2022 with C++/Windows SDK, CMake 3.21+, and vcpkg are required. Set `VCPKG_ROOT` to your vcpkg checkout.

```powershell
cmake -S . -B build -A x64 `
  "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-windows-static
cmake --build build --config Release --target check
```

Consumers use `add_subdirectory(common)` and link `common_lib`. Each parent repository pins a specific commit; common changes do not silently alter existing releases.

## Scope

This is a source snapshot with independent history, not an extraction of the private development history. It contains no game binaries, TDB dumps, resources, deployment paths, credentials, or unrelated plugins. Runtime support remains Windows x64 / Monster Hunter Wilds TDB81; the offline tests do not establish compatibility with other games.

Bundled code retains its existing copyright/license notices (including Lua, sol2, Zydis/Zycore, and the REFramework API). No new license has been selected for the original project code by this extraction.
