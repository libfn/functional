
# Developing with Nix

## Get Nix
First you will need to install Nix from [https://nixos.org/download](https://nixos.org/download).
It's as simple as running a command in your terminal.

## Usage
> [!NOTE]
> First build will trigger multiple downloads.

Software immediately available in the build environment:
- Compiler (clang or gcc)
- clang-format
- clang-tidy
- clangd
- cmake
- ccache
- ninja

The default compiler is gcc.

---

Create a build environment:
```bash
$ nix develop .
```

Or just build and run the tests:
```bash
$ nix build .
```

### Changing the compiler
Currently two compilers are supported: gcc 13 and clang 18.

To select a specific compiler use this notation (works for both `build` and `develop`):
```bash
$ nix develop .#gcc
$ nix develop .#clang
```

### VSCode
> [!NOTE]
> The executable `code` is assumed to be available in your `$PATH`.
> on Mac it's in `/Applications/Visual Studio Code.app/Contents/Resources/app/bin`

Once you are in a build environment it's possible to use VSCode directly by running it from that environment:
```bash
$ git clone git@github.com:libfn/functional.git
$ cd functional && nix develop .
bash-5.2$ code .
```

This makes the selected compiler available as a kit in VSCode as well as allowing the project to be automatically configured by cmake once the kit is selected.

## Avoiding ODR violations: use one libfn version per build

> [!WARNING]
> libfn is header-only, so linking two **different versions** of it into a single binary risks an ODR (One Definition Rule) violation.

This concerns different **patch** releases of the **same** minor version (e.g. `0.1.2` vs `0.1.3`): we keep the API and ABI stable across patches, but do **not** guarantee identical inline definitions. Different **minor** versions are *not* affected; the minor number is part of a hidden inline namespace, so symbols and types from different minor versions are distinct and cannot collide.

Conan, vcpkg and Bazel resolve a dependency to a single version automatically. **Nix does not deduplicate transitive flake inputs**, so if your project depends on libfn *and* on another flake that **also** depends on libfn, you can end up consuming two versions and causing ODR violations in your program. You can avoid this by unifying them to a single version with an input `follows`:

```nix
inputs.someDependency.inputs.libfn.follows = "libfn";
```
