
# Developing with Nix

## Get Nix
First you will need to install Nix from [https://nixos.org/download](https://nixos.org/download).
It's as simple as running a command in your terminal.

## Usage
> [!NOTE]  
> First build will trigger multiple downloads. 

The default compiler is gcc.

---

Create a development shell:
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
