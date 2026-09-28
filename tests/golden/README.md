# Golden scenes and reference images

* `triangle.json`: the frozen Stage 1 scene (camera, vertices, landmarks, tolerances).
* `triangle.reference.pam`: the reviewed reference image; `triangle.reference.png` is the same pixels for viewing.

## Rules
* Tests never create or overwrite a reference. A missing or malformed reference fails the test.
* Every test run writes `<build>/test_output/<name>.actual.pam`; a failing comparison also writes `<name>.diff.pam`
  (red = pixel beyond tolerance). CI converts them to PNG and uploads them as the `test-images-<preset>` artifact.

## Creating or replacing a reference (by a person, in its own commit)
1. Run the test; it writes `build/debug/test_output/<name>.actual.pam`.
2. `python3 scripts/pam_to_png.py build/debug/test_output` and look at the PNG.
3. Check it against the scene's analytic tests (landmarks, coverage, culling) passing on the same build.
4. Copy the `.pam` and `.png` here and add a row below. A replacement also needs an ADR stating why the image changed.

## Review log
| Image | SHA-256 (pam) | Rendered on | Reviewed | Checks |
| --- | --- | --- | --- | --- |
| triangle.reference.pam | `130b421f505fd09c…` | lavapipe, Mesa 25.2.8-0ubuntu0.24.04.2, debug/release/asan-ubsan byte-identical | 2026-09-28, Claude (visual + analytic tests); Derrick approved visually 2026-09-28 | upright; blue apex, red bottom-left, green bottom-right; black background; landmarks ±2, coverage ±1%, culling pass |
