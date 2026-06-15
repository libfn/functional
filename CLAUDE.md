# CLAUDE.md

Conventions for AI agents in this repo (you are the primary reader — keep this terse).

## Commits

- Trailer `Assisted-by: Claude:<exact session model id>` (Linux-kernel convention), e.g. `claude-opus-4-7`. No `Co-Authored-By:`.
- Offer commits; never commit without confirmation. Terse messages: imperative topic, body only if needed.
- Never `git push` or sign commits — the user signs (GPG) and pushes.

## Git state

- The user may switch branch between prompts and forget. Run read-only git (`status`, `log -1`, `branch`) freely, especially when starting work, to catch it; `git diff` can be large, use judiciously.

## Code

- Default to no comment; assume the reader reads the surrounding code. Comment only where the *why* stays non-obvious despite that context (constraint/invariant/workaround/surprise); never restate code; no boilerplate docstrings.
- Routing: *unusual code* → comment; *ordinary code, noteworthy change* → commit body; *both obvious* → neither. "Context" = the surrounding code the reader sees, not why-the-change (→ commit).
- Don't create `.md`/summary/planning files unless asked.

## Tests

- Add each check to the existing `TEST_CASE`/`SECTION` (or file) covering that member/behaviour, matching local idiom — not the nearest spot or a catch-all. A check in the right named section is self-documenting.

## Tooling

- Recommended: `clangd-lsp@claude-plugins-official` for symbol navigation + post-edit diagnostics on C++. Requires a populated `compile_commands.json` (re-run CMake configure if clangd reports spurious errors in template-heavy headers).

## Memory

- Keep memory current as facts change.
- Create memory files without asking, but announce each one and its purpose.
- On wrap-up or a "memory pass" request: review memory — update/remove obsolete, flag new.

## Docs

- On memory or practice changes, check the root `.md` files for drift from reality and **offer** fixes (CLAUDE.md = practice, README/CONTRIBUTING = facts).
