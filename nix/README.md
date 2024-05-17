
# Developing with Nix

## Get Nix
First you will need to install Nix from [https://nixos.org/download](https://nixos.org/download).
It's as simple as running a command in your terminal.

## Usage
> [!NOTE]  
> First build will trigger multiple downloads. 

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

This makes the selected compiler available as a kit in VSCode as well as allows the project to be automatically configured by cmake once the kit is selected.
