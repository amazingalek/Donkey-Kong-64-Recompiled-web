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
there, so the photo would show whatever the memory last held. On the page RT64 copies each frame into RDRAM a list
late (`recompui::renderer::set_render_to_ram_late`): the copy lands when the next list starts, or at a VI once the GPU
has it, so a list doesn't wait for the GPU process to hand the frame back. In headless Chrome 155 on an RTX 2070, through
the menus and walking about DK Isles, a list takes 3.7 ms (median, two runs) against 7.8 ms with the copy made before
the list ends. The photo then holds the frame a list or two earlier, which shows only while the camera turns. Before
the game starts, every VI draws a dummy frame into 0x80700000 or 0x80725800, where the game's main code and audio
microcode load; a late copy of one landed on them and crashed 2 of about 26 boots in the audio microcode (`Unhandled
jump target in microcode n_aspMain`). RecompFrontend-web 75fe596 keeps the dummy frame out of RDRAM on a page, and
since then none of 29 cold boots in headless Chrome 155 and 25 in Firefox 158 crashed. Most of them report one CPU
write to a framebuffer with a copy pending (`RT64: the CPU wrote to the framebuffer at 0x00034000 ...`): the boot draws
there, and the overlay loaded at 0x80024000 then zeroes its BSS over it. RT64 drops that copy, so the BSS keeps its
zeroes. What it can't see is a zero written over a zero, which would get the frame, but that framebuffer's black is
0x00010001 throughout the boot and none of its words is 0. `patches/shader_survey.c` gives
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
when the page is built, so no scene draws with the slow ubershader. The list is in 15 sections, 475 descriptions and
361 pipelines: section 0, the logos (10 pipelines), then the DK Rap (49), which every visit reaches (Start in the logos
goes to it) and whose first frame wants 31 of its shaders at once, then the menus (55: a moment of DK TV's Jungle Japes
demo, the title, the main menu and its screens), then the new game's opening, Training Grounds and DK's house, DK
Isles, and a world each with its lobby. In Chrome the whole list is built behind the game from its first frame,
sections 0 and 1 first on half the host's cores and then the rest one at a time, a section the game has reached moved
to the front, and the game waits only when a frame wants 4 or more of a section's shaders that aren't built yet, under
a small "Preparing graphics" indicator in a corner. Firefox builds a pipeline in line with the rest of its GPU work, so
there section 0 is built before the game passes its first frame and each other section when the game first draws with
its shaders, the game waiting, and it keeps none between visits, so every visit waits as long: the logos 0.5-0.7 s,
the DK Rap 6.3-6.5 s as it begins and the menus 4.3 s (Firefox 158), Frantic Factory 3.6 s (Firefox 157). The logos
are a section of their own so that the page draws its first frame at once: as one section with the Rap and the menus,
115 pipelines, Firefox waited 13-30 s before it. A shader the list
misses is built behind the game, and the page remembers it and builds it with its section on the next visit. To
record the list again, build the page and run `bash tools/record_shaders.sh build-web <the plain ROM>` (its header
says how): it takes about half an hour, and then the page is built again with the new list. `tools/play_plan.json`
plays the page as a player would, for N64Recomp-browser's `tools/play_page.mjs`: the logos and the DK Rap, then the
main menu and Jungle Japes, reached by `tools/shader_survey.js`, and about a minute of DK on foot there (its header
says more).

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

Played again from the page built on N64Recomp-browser ed8764a (v0.36.0, main b0f0275) on 2026-10-14, on that canvas,
an idle host, one page at a time, 90 s: Firefox 158 waited 12.6 s for the boot's 115 and then drew 57-61 throughout,
rt64's pacing never on (it judges drops over a whole 2 s of presents now, not the first few after the wait); Chrome
155 never waited and drew 53-56; neither was short of sound.

Played again from the page built on N64Recomp-browser 2efc795 (rt64-web 9ed449d, main 2a6532e) on 2026-10-14, on that
canvas, a quiet host, one page at a time, 25 s after the waits: Firefox 158 waited 16.1 s for the boot's 115 and
then drew 59-60, Chrome 155 never waited and drew 56-60, rt64's pacing never on, no sound short, no WebGPU errors.

On a laptop's Intel UHD (Tiger Lake, Windows 11 in its best power efficiency mode) on 2026-10-14, from the published
page (main 2a6532e, N64Recomp-browser 2efc795), by `tools/shader_plan.json` (`--play 90` and `--section factory`) with
`play_page.mjs` at 1256x645, Firefox 158 and headless Chrome 155, a fresh profile each, one page at a time; presents a
second are medians after the waits, and the RTX beside the Intel chip drew nothing. Chrome never waited and drew 30.0
(29.9-36.3), the game's 30, and 29.9 to the Factory, reaching map 80 with its cutscene drawn right and no sound short.
Firefox never booted, in either run: the plan's probe found no map loaded, and 22-27 s in the page logged `Failed to
execute task type: 2` (`--play`) or `RuntimeError: index out of bounds` in the wasm (`--section factory`); nothing
drew and the sound ran dry. The ROM was byte-swapped (.v64), which the page converts, and the same file played in
Chrome. No WebGPU errors. The ROM's byte order wasn't the cause: those were the runtime's dummy frames, drawn into
0x80700000 every VI before the game starts and copied to RDRAM, which Firefox's late reports of finished GPU work left
seconds behind; they kept landing after Start, on the game's code and audio microcode. In Firefox 158 on an RTX 2070,
with `onSubmittedWorkDone` held back 100 ms by a preload (a stand-in for that laptop), the published page failed the
same way, at the same wasm offset, in three boots of three; with RecompFrontend-web 75fe596, which keeps the dummy
frame out of RDRAM, a local build waited 15 s for the boot's shaders and played into the Factory.

Played again from the page built on N64Recomp-browser d3b9031 (N64ModernRuntime-web f0e0884, RecompFrontend-web 75fe596,
rt64-web f34b19c; main ed643af, gh-pages 1c20bcc) on 2026-10-15, its files served locally with the cross-origin headers,
RTX 2070, 1256x645, a fresh profile each, the host's every core busy with other work, by `tools/shader_plan.json`'s
start and then 40 s with `play_page.mjs`: headless Chrome 155 never waited and drew 59.6 presents a second (median),
Firefox 158 waited 15.9 s for the boot's 115 and then drew 59.5. No sound short, no WebGPU errors or exceptions in
either.

Played again from Pages, built on N64Recomp-browser 7277a2e (rt64-web 8bea892, tile copies reused from one display list
to the next; main 5391696, gh-pages 90d5216) on 2026-10-15, RTX 2070, 1256x645, a fresh profile each, the host's every
core busy with other work, by `tools/shader_plan.json`'s `factory` section with `play_page.mjs`: headless Chrome 155
never waited and drew 59.2 presents a second (median), Firefox 158 waited 13.3 s for the boot's 115 and 2.3 s for the
Factory's 55 and drew 58.5. No sound short, no WebGPU errors or exceptions in either.

Played from Pages with render to RAM's copy a list late (main 6a59ade, gh-pages fcf1307) on 2026-10-15, RTX 2070, 1256x645, a fresh
profile each, a host ~30% busy, by `tools/shader_plan.json`'s `isles` section twice with `play_page.mjs`: headless
Chrome 155 never waited and drew 58.7 presents a second (median), Firefox 158 waited 29.9 s for the boot's 115 and 7 s
for DK Isles' 74 and drew 58.6. Sound short only in Chrome's first seconds (396 frames), no WebGPU errors; Firefox's
one exception was the first visit's load cut off by `coi_serviceworker.js`'s reload.

The same page in Firefox 158 on 2026-10-15, RTX 2070, a fresh profile each, a host ~20% busy, by `tools/shader_plan.json`'s
start and then the game by itself: with `onSubmittedWorkDone` held back 100 ms by a preload (the laptop's late reports;
1256x645, 40 s) it booted, waited 17.4 s for the boot's 115 and drew 18.8 presents a second, which the hold caps, with
no sound short, no WebGPU errors and no exception after the first load's; at 3840x2000, the stand-in for a laptop's
integrated graphics (25 s, no hold), it waited 17.4 s and drew 59.6, no sound short.

Played from Pages with the boot's section the logos alone (main 4cc24c6, N64Recomp-browser 91c0557) on 2026-10-15,
RTX 2070, a fresh profile each, a host ~20% busy, the game by itself after Start Game: Firefox 158 at 1256x645 waited
0.7 s for the logos' 10 before the first frame and 6.4 s for the DK Rap's 49 as the Rap began, then drew 59.4 presents
a second (median) for 90 s; pressing Start through the Rap to the main menu, it waited 0.5 and 6.3 s, 0.1 s for a
single shader and 4.3 s for the menus' 55, then drew 59.6. Headless Chrome 155 never waited,
built all 361 by 47 s in and drew 59.6. At 3840x2000, the stand-in for a laptop's integrated graphics (25 s), Firefox
waited 0.5 and 6.5 s and drew 59.0, Chrome never waited and drew 59.6. No sound short, no WebGPU errors; Firefox's one
exception was the first visit's load cut off by `coi_serviceworker.js`'s reload.

Played on N64Recomp-browser v0.46.0 (main 353a4b0 built with its pin moved there, served locally) on 2026-10-15, RTX
2070, a fresh profile each, one page at a time, a host 14-27% busy, by `tools/shader_plan.json` with `play_page.mjs`:
through `factory` at 1256x645 headless Chrome 155 never waited and drew 59.4 presents a second (median), Firefox 158
waited 0.6 s for the logos, 6.4 s as the Rap began, 4.0 s for the menus and 2.4 s for the Factory and drew 59.4; at
3840x2000 (`--play 25`) Chrome never waited and drew 59.6, Firefox waited 0.5 and 7.0 s and drew 59.5. No sound short,
no WebGPU errors or exceptions.

Played on N64Recomp-browser v0.50.0 (main 6262de0 built with its pin moved there, served locally) on 2026-10-16, RTX
2070, a fresh profile each, one page at a time, a quiet host, by `tools/shader_plan.json` through `factory` with
`play_page.mjs`: at 1256x645 headless Chrome 155 never waited and drew 55.6 presents a second (median; headless Chrome
drew about 56 frames a second that night: four games played from Pages beside their builds, as a control, read 54.8-55.6
too), Firefox 158 waited 11.0 s (13.9 s on v0.46.0) and drew 59.6; uncapped at 3840x2000 (`--uncapped --play 25`, the
first such run) Chrome drew 346.1, at the uncapped rate's ceiling, and Firefox 225.0 after a 4.6 s wait. No sound short
and no WebGPU errors in any.
