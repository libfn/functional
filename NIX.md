
# Developing with Nix

## Get Nix
First you will need to install Nix from [https://nixos.org/download](https://nixos.org/download).
It's as simple as running a command in your terminal.

## Usage
Any of the commands below will download the toolchain and all required tooling at first run so give it some time. It will be fast for the next build.

Create a development shell:
```bash
$ nix develop .
``` 

Or just build and run the tests:
```bash
$ nix build .
```

To select the toolchain use these:
```bash
$ nix develop .#gcc
$ nix develop .#clang
```
