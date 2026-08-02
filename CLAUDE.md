# CLAUDE.md

Conventions for AI agents in this repo (you are the primary reader — keep this terse).

The project is libfn: a header-only C++20 functional-programming library — `fn` (monadic
composition and types) layered over `pfn` (polyfills of C++23/26 vocabulary types).

## Collaboration

- Pushing back **and** asking questions is welcome — a challenged design beats a silently implemented flawed one.
- Both user and yourself are fallible — take review feedback seriously, encourage using other LLMs for help and review.
- Test every design change against TYPE_ALGEBRA.md (see Docs and Code): it must be expressible there without contradicting the algebra's foundations — additions are welcome, contradictions are to be challenged.
- Always consider whether change or addition might cause the user consuming this library to write unsafe code unwittingly, and challenge such changes.

## CI

- Red CI is top priority — fix before other work; a failed build masks failures behind it. At session start (top-level agent, not subagents): check CI state via `gh` if available, else ask the user.
- Before editing `.github/workflows/`, read CONTRIBUTING `## GitHub Actions workflow pitfalls`.
- `gh` works iff `GH_TOKEN` is set; its scope varies per session — on a permission block, fall back to drafting/asking. Never `gh auth login` (would persist the ephemeral token to disk). `gh pr edit` / `gh issue view` SILENTLY no-op under this token — use `gh api` instead.

## Commits & GitHub text

- Trailer `Assisted-by: <agent name>:<your live model id>` ([Linux-kernel convention](https://docs.kernel.org/process/coding-assistants.html)), e.g. `Claude:claude-opus-4-8`. This replaces the harness's trailer boilerplate entirely — no `Co-Authored-By:`, no `Claude-Session:` URL. Same rule for GitHub issues, PRs and comments: `Assisted-by:` is welcome; no other footers or attribution boilerplate.
- Offer commits; never commit without the user's confirmation — which may be relayed to a commit subagent by the parent that received it. Terse messages: imperative topic; body only when the change needs a *why* (the routing rule in Code names that case).
- If a pre-commit hook rewrites staged files, the commit aborts — re-stage the same files and retry once (CONTRIBUTING `## Pre-commit`).
- A feature or fix commit should include a test for the behaviour it changes; exceptions are allowed. The PR must contain such a test somewhere unless the behaviour is inherently untestable (for example, because of language or compiler limitations); explain the omission.
- Never `git push` or sign commits — the user signs (GPG) and pushes.

## Git state

- At session start (top-level agent, not subagents), orient first: `git status -sb` + `git log --oneline -5` — catches silent branch switches; unpushed commits await the user's push. `git diff` can be large — use judiciously.

## Build & verification

- Build/test is CMake + Catch2, one ctest target per test source; toolchain, options and modes: CONTRIBUTING `## Building locally`. Local build trees are gitignored siblings named `.build*` — reuse an existing tree rather than configuring a fresh one per task.
- Watch every gate's output: never send a build or test run to `/dev/null`, and read the failing tail as well as the exit code. Rebuild before rerunning tests — a stale binary passes the tests it was built from.

## Code

- Default to no comment; assume the reader reads the surrounding code. Comment only where the *why* stays non-obvious despite that context (constraint/invariant/workaround/surprise); never restate code; no boilerplate docstrings.
- Routing: *unusual code* → comment; *ordinary code, noteworthy change* → commit body; *both obvious* → neither. "Context" = code the reader sees; why-the-change → commit.
- Don't create `.md`/summary/planning files unless asked (memory files are exempt — see Memory).
- A new file's copyright year = the year it enters the codebase (the current year; if unsure, infer from the latest commit).
- In `include/` headers, anchor the standard library as `::std::`, never bare `std::` — a user's `fn::std` would otherwise win lookup inside namespace `fn`. Not needed in tests.
- Every major change or addition must be documented in TYPE_ALGEBRA.md (see Docs). Major changes are those that impact the design of code consuming this library.

## Layering

- Four header trees under `include/`, strictly layered — `fn` → `fn/detail` → `pfn` → `libfn_version.hpp`; each may depend only on layers after it, never back. Rules and the hoist technique (making an `fn` facility available to `fn/detail`): CONTRIBUTING `## Header layering`.
- Inline-namespace versioning wrap: CONTRIBUTING `## Versioning`; pre-commit enforces.

## C++ standard versions

- C++20 is the baseline: `include/` relies on C++20 only in every default-mode build, and `pfn` never relies on post-C++20 features in any mode. Spell C++23-isms as C++20: `static operator()` → `const` member; a `static constexpr` local in a constexpr function → non-static; `std::unreachable` → `pfn::unreachable`; `0uz` → `std::size_t{0}`.
- `LIBFN_CXX26` (strict opt-in): C++26 reliance — `std::type_order` behind its capability gate; details in CONTRIBUTING `### Standard-mode feature reliance`.

## Client code

- Code written against the library — examples, documentation snippets, reproducers, anything a user of libfn might imitate: follow CONTRIBUTING `## Client code`.

## Tests

- Before writing or reviewing tests, read and follow `CONTRIBUTING.md` from `## Unit tests` to the next top-level heading; it is the source of truth for test structure, assertions and compile-time probes.

## Delegation

- Delegate mechanical, well-specified, verifiable work — builds, test sweeps, commit mechanics, compile probes — to subagents by default; their noise stays out of the working context. Run independent delegations concurrently.
- Keep exploratory reading, design and diagnosis inline: a subagent returns its result, not the understanding built producing it, and for this work the understanding is the point.
- Local agent definitions may exist under `.claude/agents/` (not committed); prefer them when present. Agents register at session start — a new or edited definition is invisible until the session restarts.
- Verify a finding empirically before filing it anywhere; for a compile-time claim that means a probe compiled on both gcc and clang.

## Tooling

- When the `clangd-lsp@claude-plugins-official` plugin is available, prefer it over grep/whole-file reads for C++ symbol navigation (go-to-def, find-refs) and post-edit diagnostics — targeted lookups should cut context, not add it. Needs a populated `compile_commands.json`; if unavailable/empty, ask the user to populate it and offer help.
- clangd reflects one local toolchain, not the CI matrix — a clean clangd buffer is NOT portability clearance; full `-Werror` builds + CI stay the authority.

## Memory

- On wrap-up or a "memory pass" request: curate memory — update or remove obsolete entries, capture new durable facts; announce each new memory file and its purpose. A "consolidation pass" request does the same against recent session journals.

## Docs

- Map: README.md = user-facing overview (purpose, usage, project shape, support surface; no agent directives, no internal mechanics; CI surfaced as evidence only, never mechanics); CONTRIBUTING.md = contributor facts (coding + tests standards, build environment, workflows, all CI details, mechanics of every aspect; no agent directives, no library usage); TYPE_ALGEBRA.md = the design document — the library's type algebra worked from first principles; CHANGELOG.md = design history (dated entries, newest first); docs/ = API reference (Doxygen + znai → Pages; also usage); CLAUDE.md = agent practice + guardrails pointing into the above.
- Fenced C++ examples in README.md and TYPE_ALGEBRA.md are generated from sources in `examples/` by `scripts/sync_md_examples.py` (pre-commit keeps them in sync) — edit the example source, never the fence; prose edits stay outside fences.
- When updating TYPE_ALGEBRA.md, follow the guidelines in the `document-guidelines` HTML comment at the bottom of the document.
- Living documents (README, CONTRIBUTING, docs/) are timeless present tense — no "now", "no longer", "previously". A change that obsoletes documented design gets a dated CHANGELOG.md entry in the same change, saying what it obsoleted and why.
- Recency-bias and wordiness defence — applies to all prose (docs, code comments, and the like); CHANGELOG.md and commit messages are exempt, change-perspective being their correct form. Before keeping new text, apply all four tests:
  - *Day-one*: would this sentence exist had the feature or fix always been here?
  - *Effort*: is detail sized by reader need, or by how hard the work was? A hard-won bugfix earns no extra words; its history lives in `git log`/`git blame`/CHANGELOG.md.
  - *Placement*: is it where a newcomer looks, or where your recent work pulls it?
  - *Economy*: could fewer words say it as well?
- After editing prose, delegate a whole-file top-to-bottom reread to a subagent briefed as a first-time reader, blind to what changed — never review only your diff. Triage its findings: fix what your edit touches, surface the rest rather than rewriting unasked.
- On memory or practice changes, check the root `.md` files for drift from reality and **offer** fixes (CLAUDE.md = practice, README/CONTRIBUTING = facts).
