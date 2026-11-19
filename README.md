# ReXGlue SDK: Steam Deck (Linux/Vulkan) fork

This fork holds the Vulkan backend work that makes **Fable II** (the [Fable 2 recompilation](https://github.com/himdo/Fable-2-Recomp)) run well on the **Steam Deck**, and fixes the Vulkan-only rendering bugs that D3D12 didn't have.

- **Code:** branch [`steam-deck-vulkan`](https://github.com/darokk/rexglue-sdk/tree/steam-deck-vulkan). This `main` branch only carries this page.
- **Pull request upstream:** [himdo/rexglue-sdk#34](https://github.com/himdo/rexglue-sdk/pull/34), with the full technical breakdown and every new setting.
- **Game side:** [darokk/Fable-2-Recomp](https://github.com/darokk/Fable-2-Recomp), branch `steam-deck-linux` ([himdo/Fable-2-Recomp#202](https://github.com/himdo/Fable-2-Recomp/pull/202)).

## Results on a Steam Deck OLED

SteamOS, Gaming Mode, 1120x720 internal resolution, 60 fps cap; medians of unattended runs from the same save.

| | Before | Now |
|---|---|---|
| Busy village scene, many NPCs | 33-35 fps | **52.4 fps** |
| GPU work per frame | 27.8 ms | 17.2 ms |
| Lighter areas | 45-55 fps | 60 fps (cap) |
| Bowerstone market, turning the camera | 43.9 fps, 38 stutters of 70 ms+ in 2 min | **50.0 fps, no stutters** |

## What changed, in short

- **GPU (EDRAM emulation):** render-to-texture results go straight into the textures instead of through guest memory, including the HDR scene and depth; the game's two predicated tiles are drawn as one pass (about 950 fewer draws per frame); Direct3D 9 clears are forwarded instead of copied; render target copies per frame 171 → 22.
- **CPU:** submission on a worker thread, cheaper draws, fewer write-watch `mprotect` calls, unchanged uploads skipped, CPU-computed terrain/skinning draws skipped on the GPU, no `/proc/self/maps` parsing on memory commits (the camera-turn stutter).
- **Rendering fixes:** the quest trail (float24 shader-written depth), black eyes (texture exponent bias), delayed readbacks.
- **Diagnostics** (off by default): `perf_log.txt` summaries per 5 s with CPU per thread, GPU time per kind of work and per render pass, clocks and APU power, and a frame dump.

Each optimisation has its own setting and can be turned off for comparison.

## Patch notes

Each entry is one update pushed to `steam-deck-vulkan`.

### v3 · 2026-11-19 · stutter fix
- No more stutter when turning the camera in busy areas: committing guest memory no longer reads and parses all of `/proc/self/maps` (up to about 70 ms per burst of commits).
- Bowerstone market camera-turn test: 43.9 → 50.0 fps, stutters of 70 ms or more 38 → 0, slowest frame per 5 s 135 → 30 ms.

### v2 · 2026-11-18 · direct HDR/depth resolves, clear forwarding, Deck diagnostics
- The HDR scene and float24 depth resolve straight into their textures through a conversion compute shader (texture loads 1.3 → 0.3 ms per frame).
- More Direct3D 9 clears forwarded; merged second-tile draws dropped before setup; CPU-computed memexport draws skipped on the GPU.
- Diagnostics: GPU time per render pass, APU power, write-watch faults by thread and page.
- Busy village 48.6 → 50.8 fps (52.4 fps with the game-code PGO build).

### v1 · 2026-11-17 · first version
- Vulkan gets Fable II's D3D12-only draw fixes; Linux build fixes.
- Submission worker thread, cheaper draws, fewer `mprotect` calls, unchanged uploads skipped.
- Direct resolves to textures, layout-mismatch transfers skipped, clears forwarded, predicated tiles merged.
- Quest trail and black-eye fixes, delayed readback by default.
- PGO/ThinLTO build options and performance logging.
- Busy village 33-35 → 48.6 fps.

---

*Below: the upstream README.*

> [!CAUTION]
> This project is in early development. Expect things to not work quite right and there to be significant changes and breaking public API updates as development progresses. Contributions and feedback are welcome, but please be aware that the codebase is still evolving rapidly.

<h1 align="center">
  <br>
  <a href="https://github.com/rexglue/rexglue-sdk">
    <img src="https://github.com/rexglue/rexglue-media/blob/main/ReX_Banner.png" alt="ReXGlue banner">
  </a>
  <br>
  <br>
  <a href="https://discord.gg/CNTxwSNZfT">
    <img src="https://img.shields.io/badge/Discord-Join%20Server-5865F2?logo=discord&logoColor=white" alt="Discord">
  </a>
  <a href="https://github.com/rexglue/rexglue-sdk/stargazers">
    <img src="https://img.shields.io/github/stars/rexglue/rexglue-sdk" alt="rexglue-sdk stargazers">
  </a>
</h1>

ReXGlue converts Xbox 360 PowerPC code into portable C++ that runs natively on modern platforms.

ReXGlue is heavily rooted on the foundations of [Xenia](https://github.com/xenia-project), the Xbox 360 emulator. Rather than interpreting or JIT-compiling PPC instructions at runtime, ReXGlue takes a different path: it generates C++ source code ahead of time, an approach inspired by [XenonRecomp](https://github.com/hedge-dev/XenonRecomp) and [rexdex's recompiler](https://github.com/rexdex/recompiler).

Latest SDK builds and releases are published on [GitHub Releases](https://github.com/rexglue/rexglue-sdk/releases). Join the [Discord server](https://discord.gg/CNTxwSNZfT) for updates and share what you have created.

## Builds

| Channel | CI | Download |
| --- | --- | --- |
| Release | [![win-amd64](https://github.com/rexglue/rexglue-sdk/actions/workflows/build-win-amd64.yaml/badge.svg)](https://github.com/rexglue/rexglue-sdk/actions/workflows/build-win-amd64.yaml) [![linux-amd64](https://github.com/rexglue/rexglue-sdk/actions/workflows/build-linux-amd64.yaml/badge.svg)](https://github.com/rexglue/rexglue-sdk/actions/workflows/build-linux-amd64.yaml) [![linux-arm64](https://github.com/rexglue/rexglue-sdk/actions/workflows/build-linux-aarch64.yaml/badge.svg)](https://github.com/rexglue/rexglue-sdk/actions/workflows/build-linux-aarch64.yaml) | [Latest stable](https://github.com/rexglue/rexglue-sdk/releases/latest) |
| Nightly | [![nightly](https://github.com/rexglue/rexglue-sdk/actions/workflows/nightly.yaml/badge.svg)](https://github.com/rexglue/rexglue-sdk/actions/workflows/nightly.yaml) | [Latest pre-release](https://github.com/rexglue/rexglue-sdk/releases?q=prerelease%3Atrue) |

## Quickstart

For quick start guide, full CLI reference, and config file options, see the [wiki](https://github.com/rexglue/rexglue-sdk/wiki).

# **Disclaimer**
ReXGlue is not affiliated with nor endorsed by Microsoft or Xbox. It is an independent project created for educational and development purposes. All trademarks and copyrights belong to their respective owners. 

This project is not intended to promote piracy nor unauthorized use of copyrighted material. Any misuse of this software to endorse or enable this type of activity is strictly prohibited.


# Credits

## ReXGlue
- [Tom (crack)](https://github.com/tomcl7) - Project Founder
- [Loreaxe](https://github.com/Loreaxe) - Linux Contributor
- [mystixor](https://github.com/Mystixor) - Windows Contributor
- [Graine25](https://github.com/Graine25) - Project Support
- [Carlos Estrague (mrcmunir)](https://github.com/mrcmunir) - Linux / ARM64 Contributor
- [sanjay900](https://github.com/sanjay900) - Linux / SDL Contributor
- [Toby](https://github.com/TbyDtch) - Project Support
- [Roxxsen](https://github.com/Roxxsen) - CI/CD Contributor

The list above is not exhaustive. Thanks to everyone in the ReXGlue community who contributes code, files issues, tests builds, and keeps the project moving.

## Very Special Thank You:
- [Project Xenia](https://github.com/xenia-project/xenia/tree/master/src/xenia) - Their invaluable work on Xbox 360 emulation laid the groundwork for ReXGlue's development. This project (and numerous others) would not exist without their hard work and dedication.
- [XenonRecomp](https://github.com/hedge-dev/XenonRecomp) - For pioneering the modern static recompilation approach for Xbox 360. A lot of the codegen analysis logic and instruction translations are based on their work. Thank you!
- [rexdex's recompiler](https://github.com/rexdex/recompiler) - The OG static recompiler for Xbox 360. 
- Many others in the Xbox 360 homebrew and modding communities whose work and research have contributed to the collective knowledge that makes projects like this possible.
