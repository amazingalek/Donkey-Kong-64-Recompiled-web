# Building Guide

This guide will help you build the project on your local machine. The process will require you to provide a decompressed ROM of the US version of the game.

These steps cover: decompressing the ROM, running the recompiler and finally building the project.

## 1. Clone the DonkeyKong64Recompiled Repository
This project makes use of submodules so you will need to clone the repository with the `--recurse-submodules` flag.

```bash
git clone --recurse-submodules
# if you forgot to clone with --recurse-submodules
# cd /path/to/cloned/repo && git submodule update --init --recursive
```

## 2. Install Dependencies

### Linux
For Linux the instructions for Ubuntu are provided, but you can find the equivalent packages for your preferred distro.

```bash
# For Ubuntu, simply run:
sudo apt-get install cmake ninja-build libsdl2-dev libgtk-3-dev lld llvm clang
```

### Windows
You will need to install [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/).
In the setup process you'll need to select the following options and tools for installation:
- Desktop development with C++
- C++ Clang Compiler for Windows
- C++ CMake tools for Windows

You'll also need [vcpkg](https://vcpkg.io/en/). Once you have it built and installed, in the root of the DK64 recomp repo you'll need to run

```bash
vcpkg install curl
```

to install the required curl dependency for the mod loader.

The last tool necessary will be `make` which can be installed via [Chocolatey](https://chocolatey.org/):
```bash
choco install make
```

## 3. Decompressing the target ROM
You will need to decompress the NTSC-U 1.0 N64 Donkey Kong 64 ROM (sha1: cf806ff2603640a748fca5026ded28802f1f4a50) before running the recompiler.

The most straightforward way to do this is to set up the [Donkey Kong 64 decompilation](https://gitlab.com/dk64_decomp/dk64), which will decompress the ROM when building. Alternatively, you can run the [Donkey Kong 64 ROM Decompressor Python Script](https://gitlab.com/dk64_decomp/dk64/-/blob/main/tools/generate_decompressed_rom.py) directly, which is what the decompilation uses to decompress the ROM.

Regardless of which method you use, copy the decompressed ROM to the root of the DonkeyKong64Recompiled repository with this filename:
- `donkeykong64.decompressed.us.z64`

## 4. Generating the C code

Now that you have the required files, you must build [N64Recomp](https://github.com/N64Recomp/N64Recomp) and run it to generate the C code to be compiled. The building instructions can be found [here](https://github.com/N64Recomp/N64Recomp?tab=readme-ov-file#building). That will build the executables: `N64Recomp` and `RSPRecomp` which you should copy to the root of the DonkeyKong64Recompiled repository.

After that, go back to the repository root, and run the following commands:
```bash
./N64Recomp us.toml
./RSPRecomp n_aspMain.toml
```

## 5. Building the Project

Finally, you can build the project! :rocket:

On Windows, you can open the repository folder with Visual Studio, and you'll be able to `[build / run / debug]` the project from there.

If you prefer the command line or you're on a Unix platform you can build the project using CMake:

```bash
cmake -S . -B build-cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -G Ninja -DCMAKE_BUILD_TYPE=Release # or Debug if you want to debug
cmake --build build-cmake --target DK64Recompiled -j$(nproc) --config Release # or Debug
```

## 6. Success

Voilà! You should now have a `DK64Recompiled` executable in the build directory! If you used Visual Studio this will be `out/build/x64-[Configuration]` and if you used the provided CMake commands then this will be `build-cmake`. You will need to run the executable out of the root folder of this project or copy the assets folder to the build folder to run it.

> [!IMPORTANT]
> In the game itself, you should be using a standard ROM, not the decompressed one.

## The web page

This fork's `main` branch also builds with [Emscripten](https://emscripten.org) into a page, `DK64Recompiled.html`,
that draws through WebGPU. `lib/rt64`, `lib/N64ModernRuntime` and `lib/RecompFrontend` are the web forks at the
commits [N64Recomp-browser](https://github.com/amazingalek/N64Recomp-browser)'s `forks.txt` names, and `lib/N64Recomp-browser` makes the
game a page (v0.11.0 here). Mods and the mod store are off on the page. RT64's render to RAM stays on, as the game reads its own
frames: a Banana Fairy photo is a 160x128 patch of the frame on screen that the CPU copies out of RDRAM
(`func_global_asm_806FFF88`, patched in `patches/patches_heap.c`), and without render to RAM nothing draws the frame
there, so the photo would show whatever the memory last held. In Chrome the readback makes every display list wait
for the GPU process (5-6 ms a list, measured on another game). The game's other uses of a frame, the snapshots in
transitions (`func_global_asm_8070A848`, patched in `patches/patches_framebuffer.c`), copy it on the GPU, which RT64
does either way.

Do steps 1 to 4 with the N64Recomp in `lib/N64ModernRuntime/N64Recomp` (the web fork's: its `funcs.h` declares every
function the generated code calls, which a WebAssembly build needs); the generated code is the same for both builds.
Then, with the Emscripten SDK installed and activated and [naga](https://github.com/gfx-rs/wgpu/tree/trunk/naga)
installed (`cargo install naga-cli`):

```bash
embuilder build sdl2-mt freetype-legacysjlj      # once
emcmake cmake -S . -B build-web -G Ninja -DCMAKE_BUILD_TYPE=Release -DRT64_NAGA_EXECUTABLE=<naga> \
    -DPATCHES_C_COMPILER=<clang with a MIPS target> -DPATCHES_LD=<ld.lld>
cmake --build build-web --target DK64Recompiled
```

A first build compiles about 670 files, which takes half an hour or more on Windows. Two things make later builds
shorter:

- [ccache](https://ccache.dev) on `PATH` when you configure. Every compile then goes through it, so a new build dir,
  or another checkout of this repo with its build dir in the same place, compiles nothing that was compiled before:
  a second, fresh build dir took 8 minutes instead of 35 on a busy machine, most of it the link. Keep the build dir
  inside the checkout (`build-web`): from one outside it, about half the files miss. The cache is per user, so it also
  serves other recomps built on the same web support.
- `-DN64RECOMP_BROWSER_FAST_LINK=ON` when you only want to try something. The link then takes seconds instead of
  minutes, because it skips Binaryen's optimization of the whole program. Leave it off for a page that people play.

vcpkg isn't needed for the page: it brings curl, for the mod store. `build-web` then holds `DK64Recompiled.html`,
`.js`, `.wasm`, `.data` and `coi_serviceworker.js`. The page needs a browser with WebGPU (Chrome or Edge 113 and up)
and must be cross-origin isolated: serve the folder with `Cross-Origin-Opener-Policy: same-origin` and
`Cross-Origin-Embedder-Policy: require-corp`, or on a host that can't set headers, as is, and `coi_serviceworker.js`
sets them after one reload. The player picks their ROM (the plain one, not the decompressed one) in the launcher; it,
the settings and the saves are kept in the browser's IndexedDB for that origin.

Before the game passes its first frame, the page builds every shader in `src/rt64_shaders.txt` (311, specialized
when the page is built) under a "Preparing graphics" bar, so no scene waits on a shader or draws with the slow
ubershader meanwhile. In Chrome that takes about 40 seconds on a desktop GPU, on every visit: Chrome's pipeline cache
can't hold them. Firefox builds none and skips it. To add shaders the list misses, open the page with
`?record-shaders`, play, and add the `rt64 shader:` lines the console prints that the list doesn't have yet.
