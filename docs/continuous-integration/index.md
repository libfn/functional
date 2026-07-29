---
title: Continuous Integration
---

Every push and pull request runs the checks below. They fall into three groups: what proves the
library works, what proves it can be consumed, and what builds the containers the rest run in.

---

## Proving the library works {style: "api"}

* `build` — compiles and runs the tests across the compiler matrix: gcc 12 to 16 and clang 16 to
  22, in Debug and Release, with clang additionally against libstdc++. A C++23 lane builds C++20
  as well, so the compilers it covers skip their standalone C++20 job rather than build it twice.
* `pre-commit` — runs the hooks of `.pre-commit-config.yaml`, `clang-format` among them, over the
  whole tree rather than the diff.
* `codecov` — builds an instrumented tree, reports coverage to
  [codecov.io](https://app.codecov.io/gh/libfn/functional).
* `sonarcloud` — builds with the compilation database and reports analysis to
  [SonarCloud](https://sonarcloud.io/summary/new_code?id=libfn_functional).
* `licence` — scans dependencies with [FOSSA](https://app.fossa.com/projects/git%2Bgithub.com%2Flibfn%2Ffunctional).
* `docs` — builds this site, and publishes it only from `main`; a pull request builds it as a
  check, so a broken reference fails review rather than the deployment.

## Proving the library can be consumed {style: "api"}

Each of these takes the library the way a user would, rather than building it in place:
`package-test-conan`, `package-test-vcpkg`, `package-test-nix` and `package-test-bazel`. Between
them they cover every packaging route the project offers, which is what keeps the packaging
metadata honest as the headers move.

## Building the containers {style: "api"}

`ci-build`, `ci-pre-commit` and `ci-docs` build the images the other workflows run in, from the
definitions in `ci/gcc`, `ci/clang`, `ci/pre-commit` and `ci/docs`. They run on the 11th and 26th
of each month, whenever an image definition changes on `main`, and on request. Each image is built
for amd64 and arm64 and published to `libfn.azurecr.io` under a tag naming the commit it was built
from; the workflows that consume an image name that exact tag, so a rebuild never changes what a
build runs against until the pin is moved deliberately.

---

## Analysis of a pull request {style: "api"}

Coverage and static analysis need credentials to report, and a pull request from a fork is not
given them — so `codecov` and `sonarcloud` each come in two parts. The workflow triggered by the
pull request builds without secrets and uploads its results as an artifact; a second workflow,
`codecov-pr-scan` or `sonarcloud-pr-scan`, wakes on that run completing, and reports from the
base repository where the credentials live.

The two parts must agree about what was measured. The build is made at the pull request's head
commit rather than the merge commit GitHub offers by default, because the head is what the report
is attributed to; the artifact records that commit, and the scan refuses an artifact whose commit
is not the head it is reporting on — which is what a stale artifact looks like.

Setting a fork up to report against your own Codecov and SonarCloud accounts is described in
[CONTRIBUTING](contributing/index).
