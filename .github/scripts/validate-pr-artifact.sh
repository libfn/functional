#!/usr/bin/env bash
# Validate a downstream-scan inputs artifact and emit the pull request metadata it carries.
#
# Invoked by the privileged `*-pr-scan` workflows (which run with repository secrets) BEFORE the
# untrusted pull request head is checked out, so it treats the artifact as hostile: the tarball is
# produced by the triggering workflow running on fork-PR code. Validate the contents before
# extracting to prevent path traversal etc. security issues.
#
# Keep in sync with both consumers: codecov-pr-scan.yml and sonarcloud-pr-scan.yml each `bash` this
# exact script. Reads $ARCHIVE (the downloaded `<artifact-name>.tar.gz`); writes the validated
# number/head_sha/head_ref/base_ref to $GITHUB_OUTPUT. The flags mirror GitHub's `shell: bash`.
set -eo pipefail

# Fixed artifact contract: triggering workflows write PR metadata into `.pr-meta`.
META_DIR=.pr-meta

# Entry types come from the verbose listing (first column); names come from the name-only
# listing so paths with spaces are handled correctly and we do not parse `tar -v` formatting.
types=$(tar -tvzf "$ARCHIVE")
bad_type=$(printf '%s\n' "$types" | grep -vE '^[-d]' || true)
if [ -n "$bad_type" ]; then
  printf '::error::Disallowed entry types in artifact:\n%s\n' "$bad_type" >&2
  exit 1
fi
names=$(tar -tzf "$ARCHIVE")
if printf '%s\n' "$names" | grep -qE '(^/|(^|/)\.\.(/|$))'; then
  printf '::error::Unsafe path traversal in artifact:\n%s\n' "$names" >&2
  exit 1
fi
bad=$(printf '%s\n' "$names" | grep -vE "^(\.build|${META_DIR//./\\.})(/|\$)" || true)
if [ -n "$bad" ]; then
  printf '::error::Unexpected entries in artifact:\n%s\n' "$bad" >&2
  exit 1
fi
EXTRACT_DIR=$(dirname "$ARCHIVE")
tar -xzf "$ARCHIVE" -C "$EXTRACT_DIR" "$META_DIR"
META="$EXTRACT_DIR/$META_DIR"
read_field() {
  local name=$1 pattern=$2 value
  IFS= read -r value < "$META/$name" || true
  if ! printf '%s' "$value" | grep -qE "$pattern"; then
    printf '::error::Invalid %s in artifact metadata\n' "$name" >&2
    return 1
  fi
  printf '%s' "$value"
}
# Branch names are attacker-controlled (fork PRs) and are interpolated into the privileged
# downstream job, so accept a shell-safe subset of Git's refname grammar rather than mirroring
# it: Git also permits shell metacharacters (e.g. ; & $ ` ( ) ! ') which must not pass here.
ref='^[A-Za-z0-9._/+@-]+$'
number=$(read_field number   '^[0-9]+$')         || exit 1
head_sha=$(read_field head_sha '^[0-9a-f]{40}$') || exit 1
head_ref=$(read_field head_ref "$ref")           || exit 1
# The consumers check out head_sha and report it to SonarCloud/codecov, so coverage built against any
# other revision is scanned against the wrong source: line numbers drift once the base moves ahead,
# and only sonar notices (codecov mis-attributes in silence). build_sha is what the producer actually
# built, so the two must agree.
if [ ! -f "$META/build_sha" ]; then
  printf '::error::Artifact carries no build_sha; its producer predates the check. Rebase the pull request onto main.\n' >&2
  exit 1
fi
build_sha=$(read_field build_sha '^[0-9a-f]{40}$') || exit 1
if [ "$build_sha" != "$head_sha" ]; then
  printf '::error::Coverage was built at %s but the pull request head is %s. Rebase the pull request onto main.\n' \
    "$build_sha" "$head_sha" >&2
  exit 1
fi
{
  echo "number=$number"
  echo "head_sha=$head_sha"
  echo "head_ref=$head_ref"
} >> "$GITHUB_OUTPUT"
# base_ref is optional; only some triggering workflows record it.
if [ -f "$META/base_ref" ]; then
  base_ref=$(read_field base_ref "$ref") || exit 1
  echo "base_ref=$base_ref" >> "$GITHUB_OUTPUT"
fi
