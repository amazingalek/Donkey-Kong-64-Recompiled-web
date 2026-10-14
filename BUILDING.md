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
that draws through WebGPU. `lib/rt64`, `lib/N64ModernRuntime` and `lib/RecompFrontend` are Donkey Kong 64:
Recompiled's own, at the same commits (its mod store and `--game` included), and
[N64Recomp-browser](https://github.com/amazingalek/N64Recomp-browser) (`lib/N64Recomp-browser`) patches their checkouts
with the web support and makes the game a page. The patches change nothing in the desktop build. Mods and the mod store
are off on the page: the store fetches with curl, which a browser hasn't got, so `web/curl` stands in for it there
and nothing reaches it. RT64's render to RAM stays on, as the game reads its own
frames: a Banana Fairy photo is a 160x128 patch of the frame on screen that the CPU copies out of RDRAM
(`func_global_asm_806FFF88`, patched in `patches/patches_heap.c`), and without render to RAM nothing draws the frame
there, so the photo would show whatever the memory last held. In headless Chrome on an RTX 2070 a list walking about DK
Isles takes 6-7 ms with it, most of that waiting for the GPU process to hand the frame back. RT64 can copy the frame
into RDRAM a list late instead (`recompui::renderer::set_render_to_ram_late`), and the list then takes about 2 ms,
but the page doesn't: in the boot the game reuses the memory of framebuffers it has just drawn while their copy is
still pending, the copy lands on what the game wrote there, and 2 of about 26 boots with it crashed at once in the
audio microcode (`Unhandled jump target in microcode n_aspMain`), none of about 12 with the exact copy. The photo
itself came out right with it, from a list or two earlier while the camera turned. `patches/shader_survey.c` gives
the camera and film on request (its `film`), which is how that was checked by script: warp to a map, Z then C-down,
then B. The game's other uses of a frame, the snapshots in
transitions (`func_global_asm_8070A848`, patched in `patches/patches_framebuffer.c`), copy it on the GPU, which RT64
does either way.

The game's sound once went silent for good on the page, a second or three after the boot began, in 5 of 7 boots in
headless Chrome. Chrome holds the graphics thread for the boot's prebuilt shaders, about 5 s. Meanwhile the scheduler
sees the RSP busy with the graphics task it started, so the audio task waits, and a retrace's message to the audio
thread lands while the thread waits for that task. The thread takes that message at the top of its next round, before
its new task is queued, and so asks the scheduler for nothing. It then waits for a task nobody starts, and with the
message never taken no other is sent. Now the retrace handler (`patches/timing_fixes.c`) starts a pending audio task
itself, as the thread's own request would, and all 9 boots with it played on. Firefox runs the game through the wait,
and 3 boots without the fix kept their sound.

With steps 1 to 3 done, patch the submodules:

```bash
cmake -P lib/N64Recomp-browser/patches/apply.cmake lib
```

`lib/RecompFrontend` takes its patch with a three-way merge, which fetches the upstream commit the patch was made on
from N64Recomp-browser's fork of it. Then step 4, with the N64Recomp built from the patched
`lib/N64ModernRuntime/N64Recomp` (its `funcs.h` declares every function the generated code calls, which a WebAssembly
build needs); the generated code is the same for both builds. Then, with the Emscripten SDK installed and activated and [naga](https://github.com/gfx-rs/wgpu/tree/trunk/naga)
installed (`cargo install naga-cli`), and `spirv-opt` on `PATH` (SPIRV-Tools, `apt install spirv-tools`; without it
the prebuilt shaders below come out several times the size):

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
  a second, fresh build dir took 8 minutes instead of 35, most of it the link. Keep the build dir
  inside the checkout (`build-web`): from one outside it, about half the files miss. The cache is per user, so it also
  serves other recomps built on the same web support.
- `-DN64RECOMP_BROWSER_FAST_LINK=ON` when you only want to try something. The link then takes seconds instead of
  minutes, because it skips Binaryen's optimization of the whole program. Leave it off for a page that people play.

vcpkg isn't needed for the page: it brings curl, for the mod store, which `web/curl` stands in for. `build-web` then holds `DK64Recompiled.html`,
`.js`, `.wasm`, `.data` and `coi_serviceworker.js`. The page needs a browser with WebGPU (Chrome or Edge 113 and up)
and must be cross-origin isolated: serve the folder with `Cross-Origin-Opener-Policy: same-origin` and
`Cross-Origin-Embedder-Policy: require-corp`, or on a host that can't set headers, as is, and `coi_serviceworker.js`
sets them after one reload. The player picks their ROM (the plain one, not the decompressed one) in the launcher; it,
the settings and the saves are kept in the browser's IndexedDB for that origin.

The page builds the shaders the game draws with from `src/rt64_shaders.txt`, specialized and run through `spirv-opt`
when the page is built, so no scene draws with the slow ubershader. The list is in sections: section 0 (115 pipelines: the logos, the DK Rap, a moment of DK TV, the title and the menus,
which every visit draws), then the new game's opening, Training Grounds and DK's house, DK Isles, and a world each with
its lobby. In Chrome the whole list is built behind the game from its first frame, section 0 first on half the host's
cores and then the rest one at a time, a section the game has reached moved to the front, and the game waits only when
a frame wants 4 or more of a section's shaders that aren't built yet, under a small "Preparing graphics" indicator in a
corner. Firefox builds a pipeline in line with the rest of its GPU work, so there section 0 is built before the game
passes its first frame and each other section when the game first draws with its shaders, the game waiting, and it
keeps none between visits: section 0 took 15-17 s on every visit, and Frantic Factory's 3.6 s (Firefox 157). A shader the list
misses is built behind the game, and the page remembers it and builds it with its section on the next visit. To
record the list again, build the page and run `bash tools/record_shaders.sh build-web <the plain ROM>` (its header
says how): it takes about half an hour, and then the page is built again with the new list.

Played from the published page (main 72f4d46, N64Recomp-browser 7b80155) on 2026-10-14 (RTX 2070, an idle host), by
`tools/shader_plan.json` with N64Recomp-browser's `tools/play_page.mjs`: the launcher, Load ROM, the DK Rap and
Frantic Factory in both browsers, with sound and no WebGPU errors. On a first visit a local build of the same commit
in headless Chrome 155 never waited and drew 49-57 frames a second, where on v0.28.0 the DK Rap drew 18-31 for about
20 s while section 0 built behind it. Firefox 158 waited 12.5 s for section 0 (17.3 s on v0.28.0) and 2.3 s for the
Factory, and then drew 60. On a 3840x2000 canvas, the stand-in for integrated graphics, Chrome never waited and drew
52-56; Firefox waited 12.5-13.7 s and then drew 58-62 in one run, and in another, alone on the GPU, presented 30-107 a
second, the GPU's memory up 3.8 GB as the wait ended: Firefox's animation frames come in bursts there, which the page
reads as a faster display.

Played again from the page built on N64Recomp-browser b1c35d1 (main a13c6d7) on 2026-10-14, on that canvas, an idle host,
one page at a time: Firefox 158 waited 12.9 s for the boot's 115, then rt64's pacing switched on 0.6 s after the wait
and held it at the game's 30 frames a second for the next 70 s, the GPU's memory up 2 GB as the wait ended and flat
after; Chrome 155 never waited and drew 55-56; neither was short of sound.
