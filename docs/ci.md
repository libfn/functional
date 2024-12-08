---
title: Continuous Integration
---

##### Workflows

Workflows for continuous integration:

* `build` - check build for `gcc` and `clang` compilers on [Debian Linux](https://debian.org/)
* `nix` - check build on [nix](https://nixos.org/) platform
* `license` - license scan by [FOSSA](https://app.fossa.com/projects/git%2Bgithub.com%2Flibfn%2Ffunctional)
* `pre-commit` - enforce rules defined in `.pre-commit-config.yaml`, including `clang-format`
* `coverage` - submit unit tests coverage to [codecov.io](https://app.codecov.io/gh/libfn/functional)
* `docs` - build this documentation site [libfn.org](https://libfn.org/)
* `ci-...` - build [docker images](https://hub.docker.com/r/libfn) for continuous integration

##### Images

Images for continuous integration are defined in:

* `ci/build/gcc` - [GCC compiler](https://gcc.gnu.org/) for `build` and `coverage` workflows
* `ci/build/clang` - [Clang compiler](https://clang.llvm.org/) for `build` workflow
* `ci/pre-commit` - [pre-commit](https://pre-commit.com/) for `pre-commit` workflow
* `ci/docs` - [Znai](https://testingisdocumenting.org/znai/) for `docs` workflow

Images are refreshed at least once a month by `ci-...` workflows
