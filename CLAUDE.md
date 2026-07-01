# CLAUDE.md

Conventions for AI agents in this repo (you are the primary reader — keep this terse).

## CI

- Red CI is top priority — fix before other work; a failed build masks failures behind it. Check CI state when starting new work: via `gh` if it's available here, else ask the user.
- `gh` is **optional** — use it when `GH_TOKEN` is set (repo `libfn/functional`); token scope varies per session, so attempt what the user asks and fall back to drafting/asking if a permission blocks. The token is a RAM-only env var injected at launch holding a short-lived fine-grained PAT — never `gh auth login` (persists the token to disk).

## Commits

- Trailer `Assisted-by: Claude:<exact session model id>` (Linux-kernel convention), e.g. `claude-opus-4-8`. No `Co-Authored-By:`.
- Offer commits; never commit without confirmation. Terse messages: imperative topic, body only if needed.
- A feature and its tests written together land as one commit — don't split implementation and tests.
- Never `git push` or sign commits — the user signs (GPG) and pushes.

## Git state

- Starting work, orient first: `git status -sb` (branch, dirty state, ahead/behind origin) + `git log --oneline -5`. Catches silent branch switches; unpushed commits await the user's push. Read-only git is free; `git diff` can be large — use judiciously.

## Code

- Default to no comment; assume the reader reads the surrounding code. Comment only where the *why* stays non-obvious despite that context (constraint/invariant/workaround/surprise); never restate code; no boilerplate docstrings.
- Routing: *unusual code* → comment; *ordinary code, noteworthy change* → commit body; *both obvious* → neither. "Context" = the surrounding code the reader sees, not why-the-change (→ commit).
- Don't create `.md`/summary/planning files unless asked.
- A new file's copyright-comment year is the year it's added to the codebase — the current year; if unsure, infer it from the latest commit's date.
- In `include/` headers, anchor the standard library as `::std::`, never bare `std::` — a user's `fn::std` would otherwise win lookup inside namespace `fn`. Not needed in test files.

## Layering

Three header layers; each may depend only on those below it:
- `include/fn` — may use `fn/detail` and `pfn`
- `include/fn/detail` — may use `pfn`, never `fn`
- `include/pfn` — base (standalone C++23/26 polyfill)

Never include upward from `detail` to `fn` — to give an `fn/detail` file something that lives in `fn`, hoist it: move the implementation into `fn/detail/X.hpp` as `fn::detail::_name` (no doxygen — detail headers aren't user-facing) and leave `fn/X.hpp` a thin public wrapper re-exporting it as `fn::name` (pattern: `fn/functional.hpp`).

## C++20

C++20 is the sole export surface: fn + pfn build and pass their tests as C++20 on all supported compilers, incl. MSVC; CI additionally validates C++23 via the test-only `VALIDATE_CXX23` lanes. Keep `include/` C++20 — spell C++23-isms as their C++20 equivalents: `static operator()` → `const` member; a `static constexpr` local in a constexpr function → non-static; `std::unreachable` → `pfn::unreachable`; `0uz` → `std::size_t{0}`.

## Tests

- Add each check to the existing `TEST_CASE`/`SECTION` (or file) covering that member/behaviour, matching local idiom — not the nearest spot or a catch-all. A check in the right named section is self-documenting.

## Tooling

- Use `clangd-lsp@claude-plugins-official` for C++ symbol navigation (go-to-def, find-refs) and post-edit diagnostics; prefer it over grep/whole-file reads where it fits — targeted lookups should cut context, not add it. Needs a populated `compile_commands.json`. Ask the user to populate it and offer help if it's unavailable/empty.
- clangd reflects one local toolchain, not the CI matrix — a clean clangd buffer is NOT portability clearance; full `-Werror` builds + CI stay the authority.

## Memory

- Keep memory current as facts change.
- Create memory files without asking, but announce each one and its purpose.
- On wrap-up or a "memory pass" request: review memory — update/remove obsolete, flag new.

## Docs

- On memory or practice changes, check the root `.md` files for drift from reality and **offer** fixes (CLAUDE.md = practice, README/CONTRIBUTING = facts).
