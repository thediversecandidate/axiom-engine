# ADR-0001: Foundations

**Status:** accepted (2026-09-27)

## Context
Axiom is built by one developer with LLM coding assistants, including a local model with a 32K-token window. The plan went through five external review rounds before implementation (revisions 1–6; see `docs/`).

## Decisions
* **Stack:** C++23, Clang 21 + libc++ 21, x86-64-v3, CMake + Ninja, Vulkan 1.3, SDL3, VMA, cgltf, Catch2, MIT license (CONVENTIONS §1).
* **Engine type:** hybrid. A deterministic core with learned modules that are trained offline and promoted through frozen evaluation gates; no learning during play in shipped builds.
* **First learned capability:** NPC behavior (imitation, then reinforcement learning), proven in a 3D arena.
* **Inference:** ONNX Runtime, CPU first; TensorRT only by ADR after profiling. **Training:** Python/PyTorch outside the engine, local first, cloud when needed.
* **Rendering:** rasterized PBR is the first acceptance path; hardware ray tracing and path tracing are added later, with the raster path kept as fallback and reference.
* **Learned physics:** parameter fitting first, solver corrections second.
* **Workflow:** one agent per session through Stage 3, with metrics in STATE.md; orchestration is decided by ADR at the end of Stage 3.
* **Real-game track:** Doom 3, via a GPL-3.0 fork of RBDOOM-3-BFG v1.6.0 (`axiom-doom3`) on the player's own data; no code moves into this MIT repository.
* **Releases:** R1 = Stages 0–8 + D1; R2 = Stages 9–10 + D2–D3; R3 = Stages 11–13 + D4.
* **Creative direction:** story delivered only during gameplay; voiced lead; licensed or consented voices only (CREATIVE_DIRECTION.md).

## Consequences
Every rule above is in CONVENTIONS, CONTEXT_RULES, ROADMAP or CREATIVE_DIRECTION. Changing any of them requires a new ADR.
