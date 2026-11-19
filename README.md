# ReXGlue SDK: Steam Deck (Linux/Vulkan) fork

This fork holds the Vulkan backend work that makes **Fable II** (the [Fable 2 recompilation](https://github.com/himdo/Fable-2-Recomp)) run well on the **Steam Deck**, and fixes the Vulkan-only rendering bugs that D3D12 didn't have.

- **Code:** branch [`steam-deck-vulkan`](https://github.com/darokk/rexglue-sdk/tree/steam-deck-vulkan). This `main` branch only carries this page.
- **Pull request upstream:** [himdo/rexglue-sdk#34](https://github.com/himdo/rexglue-sdk/pull/34), with the full technical breakdown and every new setting.
- **Game side:** [darokk/Fable-2-Recomp](https://github.com/darokk/Fable-2-Recomp), branch `steam-deck-linux` ([himdo/Fable-2-Recomp#202](https://github.com/himdo/Fable-2-Recomp/pull/202)).

## Results on a Steam Deck OLED

SteamOS, Gaming Mode, 1120x720 internal resolution, 60 fps cap; medians of unattended runs from the same save (the standing market figure is from a play session).

| | Before | Now |
|---|---|---|
| Busy village scene, many NPCs | 33-35 fps | **52.4 fps** |
| GPU work per frame | 27.8 ms | 17.2 ms |
| Lighter areas | 45-55 fps | 60 fps (cap) |
| Bowerstone market, turning the camera | 43.9 fps, 38 stutters of 70 ms+ in 2 min | **50.0 fps, no stutters** |
| Bowerstone market, standing | about 51 fps (v3) | **60 fps** (cap) |
| Bowerstone Inn, upstairs room | 37-43 fps (v3) | **60 fps** (cap) |

## Changes

Every change that made the game faster or fixed a bug, with its approximate effect on the Deck. Figures come from the logs of the round that added each change; spots and builds differ between rounds, so they don't add up exactly.

### GPU and EDRAM emulation
- **Occlusion queries work** (v4): the game's visibility tests were all answered "visible", so nothing hidden behind walls was culled; Bowerstone Inn 37-43 → 60 fps, Bowerstone market about 51 → 60 fps, 40-50% fewer draws in towns and interiors.
- **Predicated tiles merged into one pass** (`edram_merge_tiles`, `edram_merge_tiles_early`): the frame's second tile is no longer drawn again, about 950 fewer draws per frame; with clear forwarding the test spot went from 37 to 40-45 fps and render target copies from 81 to 27 per frame.
- **Direct3D 9 clears forwarded** (`edram_forward_4x_clears`): clears go straight to the render target that owns the memory instead of being drawn into a 4x copy and copied back, which removed most of the remaining render target copies (28 → 22 more in v2).
- **Direct resolves to textures** (`resolve_direct_to_texture`, `resolve_direct_skip_memory`, `resolve_direct_mip_chains`): render-to-texture results go straight into the textures instead of through guest memory, resolves read back through memory 90 of 90 → 5 of 83; together with the next item, 33-35 → 37 fps and 2.3 ms less GPU work.
- **Transfers skipped between differently laid-out render targets** (`edram_skip_layout_mismatch_transfers`): memory that is being reused isn't copied between render targets for nothing (part of the 33-35 → 37 fps above).
- **Direct HDR and depth resolves** (`resolve_direct_hdr_depth`, v2): the HDR scene and depth go into their textures through a conversion shader, texture loads 1.3 → 0.3 ms per frame, about +2 fps.
- **Unchanged uploads and texture reloads skipped** (`shared_memory_skip_unchanged_uploads`): about 70 textures (5.7 MB) were reloaded every frame unchanged; texture loads about 2.0 → 1.3 ms per frame.
- **CPU-computed memexport draws skipped** (`memexport_skip_cpu_computed`, v2): terrain and skinning draws the CPU already computed don't run on the GPU, GPU work about -0.2 ms (+0.2 fps together with the early tile drop).

### CPU
- **Submission worker thread** (`vulkan_async_submit`): command buffers are replayed and submitted on a worker thread, so the GPU command thread no longer waits in the driver (not measurable on its own: the Deck was GPU-limited in that round).
- **Cheaper draws**: fetch constants and samplers skipped when unchanged, descriptor sets reused, constant and texture lookups memoised; town spot 20-22 → 26.5 fps, plus 33.4 → 34.7 fps from the lookup memos.
- **Fewer write-watch `mprotect` calls** (`shared_memory_protect_ahead`, merged runs): 290-500 → about 115 calls per frame, 10.3% → 4.1% of the GPU command thread; with ThinLTO, town spot 30 → 33.4 fps.
- **No `/proc/self/maps` parsing on memory commits** (v3): the camera-turn stutter is gone, market turn test 43.9 → 50.0 fps and stutters of 70 ms or more 38 → 0 in 2 minutes.
- **Fable II's CPU skinning and terrain tessellation**: bone matrices decoded once per draw instead of per vertex, skinning 4.6% → 1.4% of its thread.
- **Timer queue waits instead of spinning**: one less core busy-waiting, which leaves more of the 15 W power budget to the rest (not measured on its own).

### Build
- **Profile-guided optimisation of the SDK libraries**: town spot about 26.5 → 30 fps.
- **ThinLTO for the GPU plugin**: part of the 30 → 33.4 fps step above.

### Bug fixes
- **Quest trail flickering and disappearing**: shader-written depth for float24 depth buffers is scaled as on D3D12.
- **Black character eyes**: the texture result exponent is decoded separately from the LOD bias.
- **Forced readbacks no longer wait for the GPU every frame** (`readback_resolve_force_delayed` on by default): without it, the game lost about a third of its frame rate.
- **Delayed readbacks synchronised** with the submission that wrote their buffer (stale data on the CPU).
- **Achievement toast**: registered for drawing only while it shows, so the window no longer repaints at the display refresh rate and presents frames twice.
- **Fable II's D3D12-only draw fixes on Vulkan**: CPU terrain tessellation and skinning, depth-clear transfer skip, async pipeline wait, dog fur depth bias, relative constant clamp.
- **Linux**: builds and runs correctly (medium code model, thread names within the 15-byte limit, RenderDoc only when loaded) and polls `WAIT_REG_MEM` faster.

Each optimisation can be turned off on its own with the setting in brackets, for comparison or if a game misbehaves. The occlusion query fix makes the emulation correct and has no setting.

## Patch notes

Each entry is one update pushed to `steam-deck-vulkan`.

### v4 · 2026-11-19 · occlusion queries
- Fable II's occlusion culling works: the game tests the bounding boxes of its world cells against the depth buffer and skips cells hidden behind walls, but every test was answered "visible", so everything in front of the camera was drawn.
- Cause: Xbox D3D writes a query's begin and end counts to two different addresses, and the command processor turned host queries off for the session when they differed. Each begin/end pair is now measured with a Vulkan query in the current render pass, and the result is written back once the GPU has finished, without the CPU waiting for it.
- Bowerstone Inn 37-43 → 60 fps (5,100 → 2,700 draws per frame), Bowerstone market about 51 → 60 fps (4,300 → 2,500 draws); no visible pop-in while play-testing around Bowerstone. Open areas gain little: there's not much hidden there.
- Vulkan only so far; the D3D12 backend has the same bug.

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
