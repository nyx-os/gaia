# Building

Building the kernel is a fairly straightforward process.

## Dependencies

Building Gaia requires the following programs to be installed:

- meson
- ninja
- clang
- lld

As for fetching the source code, `git` is required.

## Fetching the source code

The latest version of the source code as can be fetched from GitHub using the following command:

```bash
git clone https://github.com/nyx-org/gaia --recursive
```

## Preparing the build system

`meson` is used as the build system because of its user-friendliness and easy cross-compilation, but it must be set up properly in order for the kernel to be built properly.

As mentioned previously, `meson` makes cross-compilation quite easy, it is therefore quite easy to retarget the kernel and use different toolchains. Toolchains can be found in the `toolchains` directory in the root of the repository.

Here are the currently available toolchains:

- `x86_64-llvm`: build the kernel using the LLVM toolchain to run on x86_64 devices
- `riscv64-llvm`: build the kernel using the LLVM toolchain to run on riscv64 devices
- `aarch64-llvm`: build the kernel using the LLVM toolchain to run on aarch64 devices

GNU toolchains are not yet available since `clang` allows for cross-compilation in a single binary and therefore was originally simpler to set up.

The build system can be configured to use your toolchain with the following command:

```bash
meson setup build --cross-file toolchains/<toolchain>.ini
```

## Building the kernel

`meson` is not, in fact, a proper build system in itself; it rather generates `ninja` build files, the kernel can therefore be built by running `ninja` as such:

```bash
ninja -j$(nproc) -C build
```
