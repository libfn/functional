---
title: CI
---

##### Images

Images for continuous integration are defined in:

* `ci/build/gcc` - [GCC compiler](https://gcc.gnu.org/) for `build` and `coverage` workflows
* `ci/build/clang` - [Clang compiler](https://clang.llvm.org/) for `build` workflow
* `ci/pre-commit` - [pre-commit](https://pre-commit.com/) for `pre-commit` workflow
* `ci/docs` - [Znai](https://testingisdocumenting.org/znai/) for `docs` workflow

Images are refreshed at least once a month by `ci-...` workflows
