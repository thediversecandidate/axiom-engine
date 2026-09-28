# ADR-0002: CI package pinning via Ubuntu snapshots; local-only compiler override

**Status:** accepted (2026-09-27)

## Context
CONVENTIONS §1 requires every apt package in the CI image to be pinned. Writing exact version strings for each package requires querying the Ubuntu 26.04 archive, which the authoring environment could not reach. Separately, the authoring environment has only Clang 18, while Axiom pins Clang 21.

## Decision
1. **Snapshot pinning.** `ci/Dockerfile` installs `ci/packages.txt` with `apt-get --snapshot 20260927T000000Z`. The snapshot freezes the entire archive at that moment, so every package resolves to one exact version. The resolved versions (plus the pip freeze of `scripts/requirements.txt`) are written to `/opt/axiom-ci/packages.lock` and uploaded as a CI artifact on every run. Ubuntu 23.10 and later support snapshots natively. Because the snapshot service is HTTPS-only and the base image has no CA certificates, `ca-certificates` is first bootstrapped from the regular archive; the snapshot step then installs every listed package, `ca-certificates` included. The lock file records the final versions.
2. **Image identity.** The workflow tags the image with a hash of `ci/Dockerfile`, `ci/packages.txt` and `scripts/requirements.txt`, and runs all jobs by digest.
3. **Local-only compiler override.** `-DAXIOM_CLANG_VERSION=<major> -DAXIOM_ALLOW_UNPINNED_COMPILER=ON` lets a machine without Clang 21 build for development. CI never sets it, and only CI results with Clang 21 satisfy the Definition of Done.

## Consequences
* Follow-up (open in STATE.md): pin the `ubuntu:26.04` base image by digest once the first CI run records it.
* Updating any package means changing the snapshot ID, which requires a new ADR.
