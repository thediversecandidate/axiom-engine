# Axiom Engine — Conventions & Locked Decisions

Changing anything here requires an ADR in `docs/adr/`. Section loading rules are in CONTEXT_RULES §3.

## 1. Toolchain, Build & CI
* **Project / namespace:** Axiom, `axiom::` (`core math physics renderer ai app`). License MIT.
* **Compiler:** Clang 21 (`clang-21`/`clang++-21`), C++23. **Standard library:** libc++ 21.
* **Toolchain file** `cmake/toolchain-clang21.cmake` is loaded by every preset before `project()`: sets the compilers and applies `-stdlib=libc++ -march=x86-64-v3` to ALL C++ compilation and linking, FetchContent code included. A Stage 0 test asserts `_LIBCPP_VERSION` in engine and test code.
* **Forbidden:** `-ffast-math`, `-ffp-model=fast`.
* **Presets:** `debug`, `asan-ubsan`, `release`; CI builds and tests all three on every push.
* **Sanitizers (`asan-ubsan`):** `-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer` on all code; runtimes linked into every executable.
  * Detection fixtures (registered only in this preset) run through `scripts/expect_failure.py`, which passes only if the process exits non-zero AND its output matches the expected sanitizer report (e.g. `runtime error: signed integer overflow`, `LeakSanitizer: detected memory leaks`). Each fixture has a clean control that must exit 0.
  * `lsan.supp` starts empty. An entry is allowed only for an identified external defect, matched on a specific function (never a whole library), with a reproducer and a tracking link in its comment.
* **CI image:** all CI jobs run in `ci/Dockerfile` (Ubuntu 26.04), published to GHCR and referenced by digest. Every apt package is pinned through a dated Ubuntu snapshot (`apt --snapshot`), and the exact resolved versions are recorded in `/opt/axiom-ci/packages.lock` (ADR-0002). Changing the snapshot, base image or package list requires an ADR.
* **Local-only compiler override:** a machine without Clang 21 may configure with `-DAXIOM_CLANG_VERSION=<major> -DAXIOM_ALLOW_UNPINNED_COMPILER=ON` for development. CI never sets it, and results from it do not satisfy the Definition of Done.
* **Apt packages (`ci/packages.txt`):** `git clang-21 libc++-21-dev libc++abi-21-dev libclang-rt-21-dev clang-format-21 cmake ninja-build python3 libvulkan-dev vulkan-tools vulkan-validationlayers mesa-vulkan-drivers glslc spirv-cross`, plus SDL3 build dependencies.
* **FetchContent** (`find_package(Git REQUIRED)` runs first; `GIT_TAG` = full commit hash with the tag in a comment; `SYSTEM` on every declare): SDL3 `release-3.4.16`, VulkanMemoryAllocator `v3.4.0`, cgltf `v1.15`, Catch2 `v3.16.0`; Stage 3 adds stb (`stb_image.h`) and MikkTSpace (`3e895b49`); Stage 6 adds ONNX Runtime `v1.30.0` (MIT).
* **Single-header libraries:** each has an `INTERFACE` target (SYSTEM includes) plus exactly one implementation `.cpp` in its own static library (`vma_impl.cpp` → `VMA_IMPLEMENTATION`, `cgltf_impl.cpp` → `CGLTF_IMPLEMENTATION`, `stb_image_impl.cpp` → `STB_IMAGE_IMPLEMENTATION`). No other file defines these macros.
* **Python tools:** pinned in `scripts/requirements.txt`. Training code has its own `training/requirements.txt` and is never part of the engine build.
* **Diagnostics wrapper:** build and test commands run through `scripts/run_quiet.py`. It stores complete stdout/stderr in `build/logs/` before summarizing, then prints at most 60 lines and 1,000 tokens (first error plus summary, long lines truncated, full-log path appended). It starts each command in a new process group/session and returns the child's exit code unchanged; signals and timeouts return non-zero. On timeout it terminates the whole group, waits a bounded grace period, kills the group, reaps the child and drains the logs. Sanitizer matching reads the complete log, never the summary.
* **Formatting:** `clang-format-21`, LLVM base, 120 columns; CI fails on any diff.

## 2. C++ Rules
* **Exceptions/RTTI:** engine targets use `-fno-exceptions -fno-rtti` as PRIVATE options; tests use exceptions. No exception may cross an engine frame.
* **Errors:** `axiom::core::Error { ErrorCode code; std::string_view message; }` (static storage), `template<class T> using Result = std::expected<T, Error>;`. Never call `.value()`. **Exception:** `math` depends on nothing, so math APIs that can fail return `std::optional`.
* **Assertions:** `AXIOM_ASSERT(cond, msg)` in `debug` and `asan-ubsan`, compiled out in `release`; logs file/line/message, then `std::abort()`.
* **Warnings:** `-Wall -Wextra -Wpedantic -Werror` on engine and test targets only.
* **Naming:** types `PascalCase`; functions, variables, members `camelCase`; constants `kPascalCase`; macros `AXIOM_UPPER_SNAKE`; files `snake_case.hpp/.cpp`.
* **Contract comments:** public types/functions that own resources, have lifetime rules, are thread-sensitive, or can fail carry `/// @owns`, `/// @lifetime`, `/// @thread`, `/// @errors` lines; the repo map extracts them.
* **Threading:** single-threaded until Stage 5.

## 3. Math & Physics
* **Units:** SI (m, kg, s); radians. **Coordinates:** right-handed; +X right, +Y up, +Z toward the viewer; camera looks down −Z.
* **Matrices:** column-major `float data[16]`, column vectors `v' = M·v`, `M_total = P·V·W`.
* **Quaternions:** stored `(w, x, y, z)`; Hamilton product; active rotation `v' = q·(0,v)·q*`; `qB*qA` applies A then B. glTF `(x, y, z, w)` is reordered on load.
* **Precision:** `float` for spatial/render/physics state; `double` only for time accumulators.
* **Tolerances:** absolute `1e-5` for single operations with |component| ≤ 1e3 and angles in [−2π, 2π]; relative `1e-5` above magnitude 1; any other tolerance is stated in the test. Valid inputs never produce non-finite output.
* **Normalization:** `tryNormalize()` returns `std::optional`, empty for non-finite input or length < 1e-12; `normalized()` requires a valid input (asserted).
* **Integration:** semi-implicit Euler. **Time step:** fixed-step accumulator in Core, Δt = 1/60 s.
* **Randomness:** fixed, logged seeds everywhere.

## 4. Rendering
* **Viewport:** positive width/height, depth range [0, 1], exactly one Y inversion (in the projection). Never a negative viewport height.
* **Projection:** reversed-Z, infinite far. With `f = 1/tan(fovY/2)`, aspect `a`, near `n > 0`, stored columns `c0=(f/a,0,0,0) c1=(0,−f,0,0) c2=(0,0,0,−1) c3=(0,0,n,0)`, so `z_ndc = n/(−z_view)`.
* **Depth:** `D32_SFLOAT`, clear 0.0, compare `GREATER_OR_EQUAL`, test and write enabled.
* **Winding:** pipelines declare `VK_DYNAMIC_STATE_FRONT_FACE`, and every draw calls `vkCmdSetFrontFace`: `COUNTER_CLOCKWISE` normally, `CLOCKWISE` when the world transform's determinant is negative. `cullMode = BACK`.
* **Device features:** enable `dynamicRendering` and `synchronization2` explicitly.
* **Validation:** `VK_LAYER_KHRONOS_validation` + debug messenger in debug and test builds; any ERROR message, including at teardown, fails the test.
* **Indices:** all imported and regenerated indices are `uint32_t`, bound as `VK_INDEX_TYPE_UINT32`; vertex/index counts are checked for overflow.
* **Tangent frames under world transforms:** for a non-singular world linear transform M, normals transform by inverse-transpose(M), tangents by M, then the frame is orthogonalized and normalized; bitangent handedness = `tangent.w · sign(det M)`. Non-finite matrices and matrices with `rcond∞ = 1/(‖M‖∞·‖M⁻¹‖∞) < 1e-6` are rejected with an error; no absolute determinant cutoff is used.
* **CPU types:** `Vector3` 12 bytes; `Vector4`, `Matrix4`, `Quaternion` `alignas(16)`.
* **GPU structs:** written to match the GLSL block exactly under `std140` (UBO) / `std430` (SSBO); a scalar after a `vec3` packs at offset 12. Every shader is reflected with `spirv-cross --reflect`; a test compares member offsets, array strides, matrix strides and matrix order with the C++ struct. A GPU readback test checks an array of ≥ 2 records.
* **BRDF (Stage 3 onward):** glTF 2.0 Appendix B, pinned at KhronosGroup/glTF commit `daee89b1`. `α = roughness²`, roughness clamped to [0.045, 1].
  With `nl = N·L`, `nv = N·V`: if `nl ≤ 0` or `nv ≤ 0`, direct illumination is zero (checked before constructing H). Otherwise:
  `D = α² χ⁺(N·H) / (π ((N·H)²(α²−1)+1)²)`
  `Vis = 1 / (2 (nv√(α²+(1−α²)nl²) + nl√(α²+(1−α²)nv²)))` (height-correlated Smith G divided by 4·nl·nv; algebraically identical to the spec, without the 0/0)
  `specular = F·D·Vis`, Schlick Fresnel `F`, Lambert diffuse. Ambient = `A · baseColor · (1 − metallic) · occlusion`, with `A` from the scene file.
* **Output (Stages 1–3):** one directional light + ambient; no tone mapping; linear result clamped and written to an `_SRGB` target. Film camera and tone mapping arrive in Stage 10.
* **Textures:** base color and emissive are sRGB; normal, metallic-roughness (G = roughness, B = metallic) and occlusion (R) are linear.
* **glTF support:** triangles; `POSITION NORMAL TEXCOORD_0 TANGENT`; PNG/JPEG via stb_image. Any `extensionsRequired` entry returns an error. Missing `TANGENT` with a normal map: run MikkTSpace on the unindexed triangles, collect its output per face corner, split vertices wherever tangent or handedness differs, then rebuild indices from complete vertex attributes (never write results through the original index list). If `NORMAL` or `TEXCOORD_0` is also missing, return an unsupported-material error.

## 5. Determinism & Replays
* **Determinism:** same build + platform + seed + recorded inputs ⇒ identical simulation state hash at every step. Thread count must not change results: entity and contact ordering and all reductions use stable, thread-independent order.
* **State hash:** computed over a canonical serialization of ALL state that affects future simulation (transforms, velocities, RNG state, warm-start/contact caches, pending actions, timers, AI state), excluding padding and addresses. Mutation tests change each category and must change the hash.
* **Replays** record inputs, seeds, and for every model inference: request ID, the tick it was issued, the tick its result was applied, the accepted action or text, whether fallback was used, and every model-produced value kept as future-affecting state (e.g. recurrent hidden/cell tensors, stored with dtype, shape and exact bytes). Replay applies all of these at their recorded ticks without running inference. Records are totally ordered by `(issued_tick, request_id)` for issue and `(applied_tick, request_id)` for application; duplicate IDs, missing records or inconsistent issue/apply pairs are replay errors.
## 6. Learned Models & Evaluation
* **Models:** ONNX files stored as `models/<name>/<version>/` with SHA-256, training-data hash and evaluation results; the engine loads only versions pinned in `models/manifest.txt`.
* **Inference:** ONNX Runtime C API, CPU execution first; GPU execution providers only by ADR after profiling. NPC sessions use sequential execution, 1 intra-op thread and spinning disabled; any other thread allocation requires profiling and an ADR. Latency is measured as p99 observation-to-action under the stage's maximum simultaneous NPC count plus active dialogue.
* **Evaluation gates:** datasets split by complete episodes and scenarios (never adjacent frames); held-out scenes, opponents, seeds, metrics and thresholds are frozen and recorded before a candidate is evaluated, using an independent baseline. Classification-style gates use macro-averaged per-class recall with the confusion matrix published. Win-rate gates use the 95% lower confidence bound of the paired improvement.
* **Performance claims:** each benchmark has a manifest (internal and output resolution, rays and bounces, upscaler, frame generation, camera path, measurement interval) and reports image quality with timing. Hardware and human-panel gates run separately from hosted CI.
* **Promotion:** a new model version replaces the pinned one only after passing its evaluation gate; rollback = re-pin the previous version. No learning during play in shipped builds.
* **Fallback:** if inference fails or an output breaks an invariant, that step uses the analytic or scripted implementation and the event is logged.
## 7. Content & Voices
* **Voices:** only licensed or consented voices. Never clone or imitate a real actor, including the original Doom 3 cast; original characters speak only their original recorded lines.
* **Content boundary:** the Axiom repository contains only MIT, CC0 or CC-BY material. Anything derived from Doom 3 data (generated text, upscaled textures) stays in the user's local data, never in any repository.
