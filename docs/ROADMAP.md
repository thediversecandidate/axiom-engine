# Axiom Engine — Execution Roadmap

Two tracks run in parallel: the **engine track** (Stages 0–13, repository `axiom-engine`, MIT) and the **Doom 3 track** (D1–D4, repository `axiom-doom3`, GPL-3.0). Only the current stage is loaded per session.

**Releases** (each must pass all its tests before the next begins; nothing is dropped):
* **Release 1:** Stages 0–8 (playable Axiom Arena, one learned NPC, director) + D1 (learned Doom 3 enemies).
* **Release 2:** Stages 9–10 (ray tracing, path tracing, neural denoising, film camera) + D2 (Doom 3 director and story graph) + D3 (AI host, responsive NPCs, squad, voiced lead).
* **Release 3:** Stages 11–13 (solver corrections, photoreal characters, motion and sound) + D4 (Axiom renderer in Doom 3, Classic/Remastered toggle).

**Threshold rule:** wherever a stage says "frozen threshold", the metric, held-out data and threshold are fixed and recorded in an ADR from an independent baseline BEFORE the candidate is evaluated (CONVENTIONS §6).

## Stage 0: Ground Truth & Tooling
* **Deliverables:** module skeleton (`core math physics renderer ai app`), toolchain file, presets, pinned CI image, pinned `SYSTEM` FetchContent with single-header implementation libraries, `AXIOM_ASSERT`, `core::Error`/`Result`, `CLAUDE.md`, `STATE.md`, ADR-0001 (all decisions to date), checkers + `session_manifest.py` + `run_quiet.py` + `expect_failure.py` + fixtures, `.clang-format`, empty `lsan.supp`, GitHub Actions on `ubuntu-26.04` inside the CI image.
* **Acceptance (CI, all three presets):**
  * A Catch2 smoke test per module; `__cpp_exceptions` undefined in engine code; `_LIBCPP_VERSION` defined in engine and tests.
  * Under `asan-ubsan`: the UB fixture fails with the UBSan report, the leak fixture fails with the LeakSanitizer report, and both clean controls pass.
  * One symbol each from the VMA and cgltf implementation libraries links and runs.
  * All checker self-tests pass.
  * First configure runs inside the CI image with an empty FetchContent cache and succeeds.
  * `run_quiet.py` self-tests: success, exit 7 (returned as 7), signal termination and timeout (non-zero), and a failure whose error appears beyond the output cap (still non-zero; full log intact), and a grandchild that inherits stdout and ignores SIGTERM (no descendant left, non-zero returned by the deadline).
  * Launcher tests: complete-file load admitted and refused cases, diagnostic truncation, serialization overhead counted, checkpoint-only transition.
  * `vulkaninfo --summary` with `VK_DRIVER_FILES` set to the lavapipe ICD reports `llvmpipe`, and the validation layer is listed.

## Stage 1: Vertical Slice (Triangle)
* **Deliverables:** instance/device with explicit feature enables and debug messenger; offscreen path (no SDL window, surface or swapchain); SDL3 window + swapchain path for local presentation.
* **Frozen setup:** 256×256 `R8G8B8A8_SRGB`, no MSAA, clear (0,0,0,1), vertex colors pure red/green/blue, camera in `tests/golden/triangle.json`.
* **Acceptance (CI, offscreen, lavapipe identity asserted via `VkPhysicalDeviceDriverProperties`):**
  * At fixed integer landmark pixels (listed in the JSON), expected color = perspective-correct linear interpolation at the pixel center, sRGB-encoded, quantized to 8 bits; stored bytes match within 2. Alpha is linear.
  * Foreground pixel count within ±1% of the analytic triangle area.
  * Culling: CCW triangle facing the camera is drawn; the reversed-winding copy produces zero foreground pixels; draws ordinary → mirrored (negative determinant) → ordinary in one command buffer all cull correctly.
  * Full-image comparison to the reviewed reference: ≤ 0.1% (65 px) may differ by > 2/255 in any channel. Zero validation errors including teardown; actual/reference/diff images uploaded on failure.
* **Acceptance (local RTX 4090):** swapchain path, zero validation errors.

## Stage 2: Math
* **Deliverables:** `Vector3`, `Vector4`, `Matrix4`, `Quaternion`, `Transform`; AVX2 paths with scalar references.
* **Known answers:** +π/2 about +Z maps +X → +Y; +π/2 about +X maps +Y → +Z; (Z then X) on +X gives +Z, (X then Z) gives +Y; translate-then-rotate and rotate-then-translate match hand-computed matrices; glTF `(x,y,z,w)` for +π/2 about Y maps +Z → +X.
* **Accumulation:** chain 10,000 rotations of 2π/10,000 about a fixed axis with renormalization; after steps 1, 2,500, 5,000, 7,500 and 10,000 the orientation matches the independently computed rotation (angle k·2π/10,000) within 1e-3 rad. Repeat with steps summing to 1 rad (non-periodic). |q| within 1e-6 of 1 throughout.
* **Properties:** RᵀR = I, det R = +1; q and −q rotate identically; quaternion and matrix rotation agree on 10,000 seeded random cases; SIMD equals scalar; no non-finite output for valid input; `tryNormalize` is empty for zero-length and non-finite input.

## Stage 3: App & Mesh Pipeline
* **Deliverables:** `app` module, camera (CONVENTIONS §4), `cgltf` loader (quaternion reorder, negative-determinant winding, required-extension rejection, MikkTSpace tangents), stb_image decoding, UBO/SSBO reflection tests and GPU readback test, PBR shader per §4.
* **Assets:** Khronos Avocado (CC0) plus CC0 tangent fixtures, committed with SHA-256 in `tests/assets/manifest.txt`: a missing-tangent normal-mapped mesh, a mesh whose shared indices span mirrored UV orientation, and a reviewed UV-seam mesh.
* **Acceptance (CI, Stage 1 setup and tolerances; scenes in `tests/golden/*.json`, ambient A = 0 unless stated):**
  * Material fixtures with analytically computed expected pixels: metallic 0 and 1, roughness 0.5, light/view at normal, 45° and 80° (grazing) incidence.
  * BRDF boundaries: `nl = 0`, `nv = 0`, exactly grazing, and opposed light/view vectors all produce finite output (zero direct light where required); roughness at the 0.045 clamp is finite.
  * Tangent fixtures: generated vertices are split where tangent or handedness differs (vertex count checked), the mirrored-UV and seam meshes match their references, and the missing-tangent mesh shows its normal-map detail. A 16-bit-index source mesh whose tangent splitting produces indices above 65,535 renders correctly with `uint32` indices. A normal-mapped mesh under world scale (−1,1,1) and under non-uniform scale lights correctly (compared against its analytic expectation); a tiny uniform scale (1e-5) is accepted, and singular and ill-conditioned transforms (e.g. diag(1e8, 1, 1e-8)) are rejected.
  * Depth: near-plane fragment depth 1.0 ± 1e-6; depth strictly decreases with distance; overlapping quads occlude correctly.
  * Avocado matches its reference, with foreground coverage within ±2% and textured landmarks matching. Zero validation errors.
* **Gate:** ADR on adopting an orchestrator + workers, using STATE.md session metrics.

## Stage 4: Physics
* **Math:** Euler–Lagrange with non-conservative forces and constraints, d/dt(∂L/∂q̇ⱼ) − ∂L/∂qⱼ = Qⱼ + Σₖ λₖ ∂aₖ/∂qⱼ, L = T − V; contacts as inequalities (λₖ ≥ 0) → LCP; Sequential Impulses (projected Gauss-Seidel).
* **Deliverables:** SoA rigid bodies, broadphase (dynamic BVH or uniform grid), SAT + Sutherland–Hodgman manifolds, persistent contacts with warm starting; position-correction method chosen by ADR; **reference mode** (Δt/8, 100 iterations) with trajectory export.
* **Reference qualification (before any trajectory is used for training):** reference mode matches analytic solutions for a block sliding with friction on an incline and a ball bouncing with restitution, within tolerances frozen in an ADR, and converges as Δt is halved.
* **Acceptance:** box stack (1 m, 1 kg, g = −9.81, friction 0.5, restitution 0, sleeping off, slop < 1 mm), aligned and touching, Δt = 1/60 s, 10,000 steps: top-box horizontal drift < 1 mm; max penetration over all contacts and steps < 1 mm; starts at 10 iterations, raised if needed and recorded in an ADR. Two runs from the same seed give identical state hashes at every step.

## Stage 5: Memory, Threads & Performance
* **Deliverables:** block/arena allocators, job system (≤ 23 workers + main thread on the 7960X), thread-safe allocators.
* **Acceptance:** 10,000 spheres (r = 0.5 m), seeded grid spawn in a 50 m box; after 300 settling steps, p99 step time (broadphase included) over 1,000 steps ≤ 8 ms, `release`, 7960X. State hashes identical for 1, 8 and 23 workers.

## Stage 6: Simulation Interface & `ai` Module
* **Deliverables:** replay recording/playback (CONVENTIONS §5), headless mode, C API `axiom_env_*` with a Python wrapper implementing the Gymnasium environment interface, `ai` module with ONNX Runtime (CPU), model manifest and evaluation-gate tooling.
* **Acceptance:** a 10,000-step replay with injected model events reproduces the state hash at every step; state-hash mutation tests change each state category (CONVENTIONS §5) and each changes the hash; a model result applied one tick late is detected as a divergence; a recurrent-policy fixture whose action stays constant while its hidden state changes replays with identical hashes; two inferences applied on the same tick replay in `(applied_tick, request_id)` order, and a duplicate or missing record is reported as a replay error; headless runs the Stage 5 scene ≥ 10× real time; a toy ONNX policy loads by manifest pin, rejects a hash mismatch, and falls back to the scripted policy on injected inference failure.

## Stage 7: Learned Physics I — Parameter Fitting
* **Deliverables:** fit friction and restitution from reference-mode trajectories (differentiable or gradient-free optimization, chosen by ADR).
* **Acceptance:** only qualified reference trajectories are used. Fixtures exercise each fitted parameter independently (sliding for friction, bouncing for restitution). On held-out complete initial-condition scenarios, fitted values are within 2% (absolute 0.01 for parameters whose true value is 0), and held-out trajectory error is lower than with default parameters.

## Stage 8: Axiom Arena — Learned NPCs & Director
* **Deliverables:** Axiom Arena (Kenney, Quaternius, Poly Haven CC0 assets); NPC policy trained by imitation then reinforcement learning (pipeline from D1); director + story graph per CREATIVE_DIRECTION §7.
* **Acceptance (gates per CONVENTIONS §6):** cloned policy reaches ≥ 90% macro-averaged recall across actions on held-out episodes, confusion matrix published; opponents and evaluation seeds frozen before training; the promoted policy's paired win-rate improvement over the scripted NPC has a 95% lower confidence bound above 10 percentage points over 1,000 seeded matches; p99 observation-to-action ≤ 0.5 ms with 16 simultaneous NPCs plus active dialogue; CREATIVE_DIRECTION §8 tests marked "Arena, Stage 8" pass.

## Stage 9: Ray-Traced Lighting
* **Deliverables:** acceleration structures, ray-traced shadows, ReSTIR direct lighting; rasterized PBR kept as fallback and reference.
* **Acceptance:** CI on lavapipe (ray tracing supported since Mesa 24.1) at 256×256; shadow masks match the analytic expectation in fixture scenes; ReSTIR output error against a ≥ 4,096-sample reference is below a frozen threshold on held-out scenes (threshold derived from the non-ReSTIR baseline).

## Stage 10: Path Tracing, Neural Denoising & Film Camera
* **Deliverables:** ReSTIR GI / path tracing; neural denoiser and radiance cache trained on the engine's own converged renders; film camera (exposure, filmic tone mapping by ADR, depth of field, motion blur, grain, color grading); volumetric fog; Amazon Lumberyard Bistro (CC-BY 4.0) test scene.
* **Acceptance:** on held-out scenes, the neural denoiser beats the non-neural baseline in error against the converged reference at equal frame time. Performance manifest (CONVENTIONS §6): Bistro, 4K output, stated internal resolution, rays and bounces, upscaler, frame generation off and on reported separately, fixed camera path; frame rate at least the Cyberpunk 2077 RT Overdrive rate measured on the same 4090 with a matching manifest (recorded in an ADR; no Cyberpunk content used); image quality reported alongside.

## Stage 11: Learned Physics II — Solver Corrections
* **Acceptance:** reference-error bounds frozen before evaluation; on held-out scenarios the learned correction reduces position error against qualified reference mode by ≥ 30% with ≤ 20% added step cost; no penetration beyond tolerance and no energy gain in closed systems; fallback verified.

## Stage 12: Photoreal Characters
* **Deliverables:** skin subsurface scattering, hair, eyes; facial animation driven by dialogue audio.
* **Acceptance:** perceptual image-difference metric against converged references and lip-sync error each below frozen thresholds on held-out scenes and dialogue.

## Stage 13: Motion & Sound Realism
* **Deliverables:** animation system with motion matching and physically driven hit reactions (CC0 or licensed motion capture); spatial audio with HRTF, geometry-based sound propagation and occlusion, material-aware impacts.
* **Acceptance:** zero foot-slide above 1 cm per step in locomotion tests; audio occlusion matches geometry in fixture scenes; the blind realism panel (CREATIVE_DIRECTION §0) run on Axiom Arena, with results recorded and the ≥ 40% target tracked.

## Doom 3 Track (`axiom-doom3`, GPL-3.0, fork of RBDOOM-3-BFG v1.6.0; requires the player's own Doom 3 BFG Edition data)
* **D1 — Learned enemies (alongside Stages 0–3, Release 1):** record monster and player state; clone one monster type's scripted AI by imitation (≥ 90% macro-averaged recall on held-out episodes, confusion matrix published), then improve with reinforcement learning; promote only if, on frozen seeded encounter scenarios, the paired improvement in the frozen metrics (damage dealt/taken, time to engage, navigation failures) has a 95% lower confidence bound above zero; ONNX Runtime CPU with CONVENTIONS §6 thread settings; p99 ≤ 0.5 ms per decision with the scenario's maximum monster count; scripted fallback.
* **D2 — Director & story graph (Release 2):** reveal budgets, pacing and fallback delivery per CREATIVE_DIRECTION; §8 tests marked "Doom, D2" pass.
* **D3 — AI host, responsive NPCs, squad, voiced lead (Release 2, after Stage 6):** dialogue system per CREATIVE_DIRECTION §5.
* **D4 — Axiom renderer in Doom 3 (Release 3):** an adapter in `axiom-doom3` feeds the fork's world, materials and entities to Axiom's renderer; all gameplay (entities, scripts, triggers, combat, saves) stays in the fork. Axiom source keeps its MIT license; the combined program is distributed under GPLv3 without any proprietary game assets. Classic/Remastered toggle; side-by-side against stock RBDOOM-3-BFG on a fixed camera path with a performance manifest; locally upscaled textures kept out of every repository. A native reimplementation of Doom 3 gameplay in Axiom would need its own separately scoped roadmap.
