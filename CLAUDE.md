# CLAUDE.md

Conventions for AI agents in this repo (you are the primary reader — keep this terse).

## Collaboration

- Pushing back **and** asking questions is welcome — a challenged design beats a silently implemented flawed one.

## CI

- Red CI is top priority — fix before other work; a failed build masks failures behind it. Check CI state when starting new work: via `gh` if available, else ask the user.
- `gh` is **optional** — use it when `GH_TOKEN` is set (repo `libfn/functional`); scope varies per session, so attempt what the user asks and fall back to drafting/asking when a permission blocks. The token is a RAM-only, short-lived PAT injected at launch — never `gh auth login` (persists it to disk).

## Commits

- Trailer `Assisted-by: Claude:<exact session model id>` (Linux-kernel convention), e.g. `claude-opus-4-8`. No `Co-Authored-By:`.
- Offer commits; never commit without confirmation. Terse messages: imperative topic, body only if needed.
- A feature and its tests written together land as one commit.
- Never `git push` or sign commits — the user signs (GPG) and pushes.

## Git state

- Starting work, orient first: `git status -sb` + `git log --oneline -5` — catches silent branch switches; unpushed commits await the user's push. Read-only git is free; `git diff` can be large — use judiciously.

## Code

- Default to no comment; assume the reader reads the surrounding code. Comment only where the *why* stays non-obvious despite that context (constraint/invariant/workaround/surprise); never restate code; no boilerplate docstrings.
- Routing: *unusual code* → comment; *ordinary code, noteworthy change* → commit body; *both obvious* → neither. "Context" = code the reader sees; why-the-change → commit.
- Don't create `.md`/summary/planning files unless asked.
- A new file's copyright year = the year it enters the codebase (the current year; if unsure, infer from the latest commit).
- In `include/` headers, anchor the standard library as `::std::`, never bare `std::` — a user's `fn::std` would otherwise win lookup inside namespace `fn`. Not needed in tests.

## Layering

Three header layers; each may depend only on those below it:
- `include/fn` — may use `fn/detail` and `pfn`
- `include/fn/detail` — may use `pfn`, never `fn`
- `include/pfn` — base (standalone C++23/26 polyfill)

To give an `fn/detail` file something that lives in `fn`, hoist it: the implementation moves into `fn/detail/X.hpp` as `fn::detail::_name` (no doxygen — detail headers aren't user-facing); `fn/X.hpp` stays a thin public wrapper re-exporting it as `fn::name` (pattern: `fn/functional.hpp`).

## C++20

C++20 is the sole export surface — fn + pfn build and pass tests as C++20 on all supported compilers, incl. MSVC; CI validates C++23 via the test-only `VALIDATE_CXX23` lanes. Keep `include/` C++20 — spell C++23-isms as C++20: `static operator()` → `const` member; a `static constexpr` local in a constexpr function → non-static; `std::unreachable` → `pfn::unreachable`; `0uz` → `std::size_t{0}`.

## Tests

- Add each check to the existing `TEST_CASE`/`SECTION` (or file) covering that member/behaviour, matching local idiom — not the nearest spot or a catch-all. A check in the right named section is self-documenting.
- Every behaviour gets both a runtime `CHECK` (a `static_assert`-only branch is a codecov hole) and a constant-evaluation twin (`static_assert`) — constexpr diagnoses UB at compile time, and users rely on it.

## Tooling

- Prefer `clangd-lsp@claude-plugins-official` over grep/whole-file reads for C++ symbol navigation (go-to-def, find-refs) and post-edit diagnostics — targeted lookups should cut context, not add it. Needs a populated `compile_commands.json`; if unavailable/empty, ask the user to populate it and offer help.
- clangd reflects one local toolchain, not the CI matrix — a clean clangd buffer is NOT portability clearance; full `-Werror` builds + CI stay the authority.

## Memory

- Keep memory current as facts change.
- Create memory files without asking, but announce each one and its purpose.
- On wrap-up or a "memory pass" request: review memory — update/remove obsolete, flag new.

## Docs

- Map: README.md = user-facing overview (purpose, usage, project shape, support surface; no agent directives, no internal mechanics; CI surfaced as evidence only, never mechanics); CONTRIBUTING.md = contributor facts (coding + tests standards, build environment, workflows, all CI details, mechanics of every aspect; no agent directives, no library usage); CHANGELOG.md = design history (dated entries, newest first); docs/ = API reference (Doxygen → Pages; also usage); CLAUDE.md = agent practice + the critical selection of standards (coding, tests, documentation).
- Living documents (README, CONTRIBUTING, docs/) are timeless present tense — no "now", "no longer", "previously"; when reading or updating them, remove recency bias. A change that obsoletes documented design gets a dated CHANGELOG.md entry in the same change, saying what it obsoleted and why.
- Recency bias defence (all documentation except CHANGELOG.md, and code comments): you over-weight whatever you just worked on, so text written right after a change reads as a diff against your context, not a document for a reader who arrives fresh. The banned transition words are only the shallow symptom; test deeper: (1) day-one test — would this sentence exist had the feature or fix always been here? if not, cut it; (2) effort test — is the detail sized by reader need, or by how hard the work was? cut whatever answers questions no reader asked; (3) placement test — is it where a newcomer would look, or where your recent work pulls it? Defence: after editing, reread the whole file top-to-bottom as a first-time reader and re-judge the new text's length and position against the whole document — never review only your diff. CHANGELOG.md and commit messages are exempt: both are read as an increment on top of previous state, so change-perspective is their correct form.
- On memory or practice changes, check the root `.md` files for drift from reality and **offer** fixes (CLAUDE.md = practice, README/CONTRIBUTING = facts).
