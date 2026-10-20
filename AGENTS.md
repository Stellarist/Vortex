# Vortex Development Guide

This file applies to the entire repository. Before editing code, check for a more specific `AGENTS.md` in the target directory.

## Working Practices

- Follow the user's language for conversation. Keep this guide in English and follow the existing English naming conventions in code.
- Start with `git status --short`. Preserve existing modifications, deletions, and untracked files; do not overwrite or revert work unrelated to the task.
- Read the relevant implementation and its callers before making changes. Keep changes focused on the task, avoiding incidental refactoring, repository-wide formatting, or unnecessary dependencies.
- Treat the current source code, `CMakeLists.txt`, and `CMakePresets.json` as authoritative. Module layouts and frame flow in `README.md` may be outdated; do not restore removed architecture based on that documentation.
- When finished, briefly describe the changes, checks performed, and anything left unverified. Identify specific environmental limitations rather than reporting checks that were not run as passing.

## Project Structure

Vortex is a C++23 graphics engine project for Windows, using C++ named modules, SDL3, ImGui, Slang, and Vulkan and D3D12 rendering backends.

| Path | Responsibility |
| --- | --- |
| `Engine/Core/` | Types, math, geometry, timing, input, files, logging, and error handling |
| `Engine/Runtime/World/` | Worlds, actors, components, and assets |
| `Engine/Runtime/RHI/` | Backend-independent graphics interfaces, resource descriptions, and validation |
| `Engine/Runtime/Vulkan/` | Vulkan backend implementation |
| `Engine/Runtime/D3D12/` | D3D12 backend implementation |
| `Engine/Runtime/RDG/` | Render graph, resource access declarations, passes, and validation |
| `Engine/Runtime/Render/` | Renderers, render scenes, and concrete rendering passes |
| `Engine/Editor/` | Windows, ImGui, configuration, camera controls, and asset importing |
| `Engine/Shaders/` | Slang shaders and shared definitions |
| `Engine/main.cpp` | Startup, backend selection, main loop, and shutdown checks |
| `Scripts/` | Shader compilation and runtime resource copying |

`Assets/` contains local scene resources. Directories such as `Build/`, `Packages/`, `Logs/`, and `.cache/` contain build outputs, dependencies, or local data. Scope searches and edits to source directories where possible. Do not fix source problems by manually editing generated files or dependency copies, and do not commit large scene assets or compiled shaders.

## C++ Modules and Code Style

- In this project, `.hpp` files are usually module interface partitions rather than traditional headers. Preserve declarations such as `export module Runtime.RHI:Buffer;`; do not replace them with `#pragma once` and textual includes.
- `.cppm` files aggregate module interfaces; `.cpp` files implement modules using the corresponding `module ...;` declaration. When adding a partition, check the aggregate module's `import` / `export import` declarations and visibility.
- Use existing public modules for cross-module dependencies, such as `Core`, `Runtime.World`, `Runtime.RHI`, and `Runtime.Render`. Do not export internal implementation details merely for convenience.
- Place third-party textual includes in the global module fragment introduced by `module;`, following neighboring files. Preserve existing module usage such as `import std;` and `import vulkan;`.
- D3D12 native declarations use the `d3d12` module in `D3D12Native.hpp`, which exports the `D3D12Native.inl` header unit. Preserve this wrapper and its Windows macro cleanup; do not scatter Windows SDK includes across module interfaces.
- Follow the root `.clang-format`: tab indentation with width 4, function opening braces on a new line, and no automatic include sorting. Format only code touched by the task.
- Use the `Vortex` namespace, PascalCase type names, camelCase function names, and the variable naming conventions in neighboring code.
- Reuse existing `RHIRef`, `makeRHIRef`, RAII, `CHECK`, `ERROR`, and `LOG` mechanisms. Understand ownership and lifetimes before introducing new resource management patterns.
- `CHECK` and `ERROR` throw exceptions, including in Release builds. Do not treat them as debug-only assertions or allow exceptions to escape `noexcept` destructors or native API callbacks.

## Ownership and Module Boundaries

- `World` owns actors, and actors own components through `std::unique_ptr`. Use their registration and lifecycle APIs when adding, removing, or attaching objects; preserve component registries, active camera references, and `beginPlay` / `endPlay` ordering.
- Assets use `AssetHandle`; GPU resources use the intrusive `RHIRef`. These have different ownership semantics. Do not manually delete referenced resources or adopt a raw pointer that already has an owner.
- Asset setters call `touch()` to advance revisions. `RenderManager` uses asset revisions and a texture epoch to refresh GPU caches; changes to asset payloads must preserve invalidation.
- Keep scene extraction and asset synchronization in the render preparation path. The current renderer reads asset payloads on a single rendering thread; shared handles and atomic reference counts do not make payload mutation thread-safe.
- Preserve the separation between `Runtime.World`, `Runtime.RHI`, `Runtime.RDG`, and `Runtime.Render`. The renderer builds its passes through RDG; concrete pass implementations live under `Engine/Runtime/Render/Passes/`.

## Rendering Changes

- Preserve the boundary between RHI and backend implementations. Keep native Vulkan and D3D12 types out of generic interfaces and rendering logic.
- When changing RHI descriptions, resource states, bindings, commands, or pipeline semantics, inspect both backends, relevant validation functions, and callers. Unsupported capabilities should produce explicit errors.
- When changing RDG passes, check resource read/write declarations, state transitions, dependency ordering, and resource lifetimes together.
- Declare graph outputs and side effects so pass culling retains required work. Keep RDG references within their owning graph. Pass callbacks and borrowed spans must remain valid through graph recording; preserve the lifetime and member destruction order of `RenderFrame`.
- When changing shared CPU/GPU data or resource bindings, verify that C++ and Slang agree on layout, alignment, binding locations, and entry points.
- Keep Slang `vk::binding` attributes consistent with D3D register/space declarations and RHI binding layouts. Shader filenames and entry points must also match `RenderManager::shader` and the compiler script: Vulkan loads `<name>.spv`, while D3D12 loads `<name>.<entry>.dxil`.
- Coordinate conventions span `Core/Math.cpp`, camera projections, shader target conversions in `common.slang`, and viewport/front-face state. Check these together when changing projection, screen coordinates, culling, or depth reconstruction; avoid applying a backend correction twice.
- Preserve GPU synchronization, in-flight resource lifetimes, and shutdown waits. Do not hide problems by disabling validation or ignoring errors.

## Building and Running

Before building, inspect `CMakeLists.txt`, `CMakePresets.json`, and any applicable local presets to determine the required tools, available configurations, and output directories. Select a configure preset and a matching build preset appropriate to the task and environment. Do not assume preset names, compiler versions, generators, or output paths remain unchanged.

Configure successfully before building, and check the result of each step. Stop and investigate configuration or build failures rather than running an older executable. Build additional configurations only when relevant to the change.

Resolve dependency toolchains, environment variables, target triplets, and installation directories from the active configuration. Do not put machine-specific absolute paths in shared configuration.

- CMake recursively collects `.cpp`, `.hpp`, and `.cppm` files under `Engine/`. New modules still require correct declarations and imports.
- Check whether the selected generator uses one or multiple configurations and select the build configuration accordingly. Locate the executable from the active target's output settings; do not confuse the install prefix with the build output directory.
- Building `Vortex` invokes `Scripts/compile_shaders.py` to compile SPIR-V and DXIL for individual entry points. It also invokes `Scripts/copy_resources.py` to copy `config.toml`, `Assets/`, and `Scripts/` beside the executable.
- Both backends are compiled and linked into the same executable, and both shader formats are built regardless of the selected runtime API. Selecting D3D12 does not remove the Vulkan build dependencies. These custom targets run on each build and may copy substantial scene data.
- When invoking the shader script separately, use `Engine/Shaders/` as the working directory and specify the output directory with `--output-dir`. The script recursively searches for `.slang` files from its current working directory.
- The shader script discovers entry points from `[shader("...")]` annotations and skips files without them. A run that finds no shaders exits successfully, so check the reported compilation count as well as the exit code. Do not search from the repository root and accidentally compile generated or copied shader files.
- At runtime, the application reads `config.toml` beside the executable; `scene` is relative to the adjacent `Assets/` directory. Changes to the root configuration must be copied or rebuilt before they affect execution.
- Configuration currently accepts only `api` and `scene`. Unknown keys fail validation; scene paths must remain relative to `Assets/`. Update the loader and documentation together when adding options.
- `File::initialize(argv[0])` establishes runtime paths. Assets, shaders, and logs are resolved relative to the executable, not the shell's working directory. Each launch truncates `<executable-directory>/Logs/log.txt`; preserve relevant logs before starting another run.

## Validation

Match validation to the scope of the change. For documentation, review the content and diff. For C++ changes, build at least the relevant configuration. Shader and rendering interface changes also require shader compilation and runtime checks of affected backends.

The current CMake configuration does not register CTest tests or a dedicated automated smoke-test runner. Do not treat an empty test suite as successful validation, or turn an ad hoc three-frame run into a general acceptance criterion.

Before running the application:

1. Build the intended configuration successfully, and use the executable produced by that build. An existing executable may predate the current source; stop on build failure instead of testing a stale binary.
2. Check `Engine/main.cpp` for supported command-line arguments. The current working tree accepts `--rhi vulkan`, `--rhi d3d12`, and a positive `--frames` value. These options are part of the current entry implementation, not a separate test interface; keep this guidance aligned with source changes. Configuration is loaded before command-line overrides, so valid configuration and scene assets are still required.
3. Confirm that the executable's adjacent configuration, scene assets, shader outputs, and runtime dependencies exist. Running the application creates a window and requires a compatible GPU and windowing environment; it is not a headless test.
4. Run affected backends separately and record each process's exit code and full log before the next launch overwrites it. Check exit status explicitly using the active shell or process runner; do not assume a failed command automatically stops subsequent steps.
5. Use an external wall-clock timeout for automated runs and terminate only the process started by that check if it expires. `--frames` counts successfully rendered frames; it does not bound asset loading, GPU waits, or iterations where `beginFrame()` returns false. Select the frame count and timeout for the behavior being tested.

- Prefer Debug for graphics API validation: CMake sets `VDEBUG=1` only in Debug. D3D12 may continue with validation disabled if Windows Graphics Tools are unavailable; confirm validation was actually enabled rather than inferring it from a zero error count. Vulkan currently requires Vulkan 1.4 and requests validation layers in Debug.
- For changes to generic RHI, RDG, shared rendering logic, or shaders, check both backends whenever possible. For changes limited to one backend, check at least that backend. Keep runs sequential because they share the log path.
- Inspect logs through process termination, including teardown. The error-count checks in `main` occur before local renderer, UI, and context destruction, so exit code zero alone does not establish that shutdown was free of validation errors.
- A short run checks startup and basic frame submission. Inspect rendered output for visual changes; exercise resize/minimize/restore, multiple frames in flight, resource updates, or shutdown when the changed behavior requires them.
- If the toolchain, assets, or runtime environment are unavailable, provide reproducible validation commands and explain the limitation. Do not change the default scene or disable validation solely to make checks pass.
