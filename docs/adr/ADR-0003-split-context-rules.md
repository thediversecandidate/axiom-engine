# ADR-0003: Split the context rules; conservative token fallback

**Status:** accepted (2026-09-27)

## Context
The first real pre-flight run (`session_manifest.py`) refused an ordinary core session: the always-loaded documents measured 3,735 tokens against their 3,000 budget. `CONTEXT_RULES.md` alone was about 2,300 tokens. The planning-time bytes/4 estimate had hidden this. The pinned tokenizer is not available yet, so counts use a fallback.

## Decision
1. `CONTEXT_RULES.md` keeps only what every session needs: modules, budget, loading rules, state, Definition of Done.
2. Tool internals, repo-map and checker details, the git workflow and the Doom-fork policy move to `docs/CONTEXT_TOOLING.md`, loaded for build/CI/infra and Doom-fork tasks (counted in the sections slot).
3. Until the local model's tokenizer and chat template are pinned in `scripts/tokenizer/`, all counts use ceil(bytes/3), labelled "fallback". It over-estimates typical English and code, so budgets stay safe.

## Consequences
The protocol slot fits with margin under the fallback counter. Budgets must be re-measured once the real tokenizer is pinned (STATE.md open item).
