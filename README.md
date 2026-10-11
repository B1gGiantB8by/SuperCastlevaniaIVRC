# Super Castlevania IV Recomp

**Version 0.1.0** — a native PC recompilation of Super Castlevania IV (USA), built with [snesrecomp](https://github.com/RetroPortingToolKit/snesrecomp) and [recomp-ui](https://github.com/mstan/recomp-ui).

Play the original single-player campaign with a Castlevania-themed launcher, modern display and input settings, save states, rewind, and additional modes. This is a hybrid recompilation with interpreter fallback.

You must supply your own legally obtained ROM. ROMs and generated game code are not included in this repository.

## Features

- Castlevania-themed launcher and pause menu with controller navigation.
- Keyboard and controller remapping, audio settings, and display settings.
- CRT Soft, LCD Grid, Sharp, and Warm Composite shader presets.
- Bezel display mode and original or enhanced game timing.
- Hold-to-rewind, fast-forward, and 12 save-state slots.
- Boss Rush: fight all 14 bosses, from Rowdain to Dracula, with a gameplay timer.
- One Life Mode (experimental): one life for the entire campaign; bonus lives, rewind, and save states are disabled.
- Level Select: all 30 substages and the original stage map transitions.
- 37 offline achievements worth 593 G, with local progress, badges, and unlock notifications. No account is required; this does not sync with RetroAchievements.
- Other Castle (Easy) support using a separately supplied, already patched ROM.

Widescreen development is shelved for this release. It is disabled in ordinary launches and absent from the menus; the experimental implementation remains available through `--widescreen` for development.

## Getting started

Clone the repository and its pinned dependencies:

```sh
git clone --recurse-submodules https://github.com/B1gGiantB8by/SuperCastlevaniaIVRC.git
cd SuperCastlevaniaIVRC
```

Version 0.1 includes project-specific dependency patches. Apply them before generating or building:

```sh
bash tools/apply-project-patches.sh
```

On PowerShell, use `./tools/apply-project-patches.ps1` instead. The scripts are safe to run again when the patches are already applied. Do not update the dependency pins without reviewing these patches.

### Required ROM

Use the **headerless USA ROM**, exactly 1,048,576 bytes:

| Identity | Value |
| --- | --- |
| Filename | `Super Castlevania IV (USA).sfc` |
| CRC32 | `b64ffb12` |
| SHA-256 | `0ef6f4cce5a2273fa49fe1ce724e0048a8e39c91da6b00dbb693fe1ba909177d` |

ROM identity is checked before generation and launch. Keep the ROM outside the repository.

### Build from source

Install Git, Python 3, CMake, a C/C++ toolchain, and the framework's platform dependencies and analyzer as described in [snesrecomp's README](snesrecomp/README.md). Then:

```sh
bash tools/regen.sh --rom "/path/to/Super Castlevania IV (USA).sfc"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Generated C remains local in `src/gen/`. Regenerate after changing recompilation configuration or dependency patches.

The existing `build.ps1` and `play.ps1` are convenience helpers for the original development machine; their toolchain and ROM paths need adjusting on another machine. See [LOCAL_SETUP.md](LOCAL_SETUP.md) for those local paths and early development notes.

### Launch

Run `build/SuperCastlevaniaIvSNESRecomp.exe` on Windows, or `build/SuperCastlevaniaIvSNESRecomp` on other platforms. The launcher lets you select and verify your ROM. You can also pass a quoted ROM path directly:

```sh
./build/SuperCastlevaniaIvSNESRecomp "/path/to/Super Castlevania IV (USA).sfc"
```

The repository publishes source. The manual [Release workflow](https://github.com/B1gGiantB8by/SuperCastlevaniaIVRC/actions/workflows/release.yml) builds setup packs that generate and rebuild the game from a player's own ROM. Platform packages require a successful workflow run; the locally tested build is Windows.

## Publishing release packages

The Release workflow is already included in this repository and runs manually. It builds setup packs for Windows, Linux, macOS Apple Silicon, and macOS Intel. Each player supplies their own ROM; no ROM upload is required to run the workflow.

To build packages for the existing **v0.1.0** tag:

1. Open [Actions → Release](https://github.com/B1gGiantB8by/SuperCastlevaniaIVRC/actions/workflows/release.yml) and click **Run workflow**.
2. Under **Use workflow from**, select the **v0.1.0 tag**.
3. Set **Version** to `0.1.0`.
4. Keep **Publish release** and **Embed toolchain** checked; leave the remaining inputs at their defaults.
5. Run the workflow. After every platform build succeeds, downloads and checksums appear on the [Releases page](https://github.com/B1gGiantB8by/SuperCastlevaniaIVRC/releases).

If preflight reports that `tag v0.1.0 already exists`, select that tag instead of `main`. The workflow refuses to publish an existing version from a branch. To publish later changes from `main`, choose a new version such as `0.1.1`; leaving Version blank automatically bumps the latest tag according to the Bump input.

Uncheck **Publish release** to build workflow artifacts without creating a public release. A failed build does not publish packages; inspect the failed step's log in Actions before retrying.

## Controls and saves

Use the launcher Controls page to view and remap bindings. During gameplay, **Escape** or **L3 + R3** opens the paused menu. Defaults include **F12 / L2** for held rewind, **Tab** for held fast-forward, and **F11 / Back + RB** for Save States. Releasing rewind resumes gameplay.

In Save States, Up/Down selects a slot and A opens Load/Save/Back. B returns; Escape outside the prompt resumes. Saves and local achievement progress live in `saves/` beside the executable. Settings and ROM paths are local and excluded from Git.

Starting a mode or a selected stage replaces the current session. Save your campaign first. Boss Rush supports practice starts through `--boss-rush=N` (1–14); `--one-life --no-launcher` starts One Life Mode. `--level-select=N` selects a substage (1–30), and `--level-map=N` starts its preceding map where supported.

## Other Castle (Easy)

Supply your own patched `SuperCastlevaniaIVOtherCastle.sfc` and select **Other Modes → Other Castle (Easy)**. Apply the Easy xdelta patch separately before selecting the ROM. Neither the ROM nor the patch is included here.

The supported patched image is 4 MiB with SHA-256 `8e920c40c3810d624f686d231ec884507a421e35e3e2ab4f541c3b9e596d3f40`. It runs through the interpreter, uses separate saves in `saves/other-castle/`, and disables stock campaign achievements and additional modes. Choose **Return to Original Game** to switch back. Direct launch: `--other-castle --rom "/path/to/patched.sfc"`.

## MSU-1 soundtrack (local development)

This feature is present in the current local development build. Its implementation is not included in the published v0.1.0 tag or the current GitHub source.

Open **Audio → Choose MSU-1 ROM**, select your patched USA ROM, then choose **Play MSU-1 Soundtrack**. This option also appears in the paused frontend; the title overlay links to it under **Options → Audio → MSU-1 Soundtrack**. Switching restarts the game. Choose **Return to Original Soundtrack** to restart with your original USA ROM.

The supported patched ROM is 1 MiB with SHA-256 `49e8321030cc5d923a1b03cd8ff83843e0fd0a2445a7799b945ba736dd91be3d`. Keep `<ROM filename without extension>-<track>.pcm` and the `.msu` file beside it. This ROM uses interpreter execution and separate saves under `saves/msu1/`. Selecting the ROM directly on the command line automatically detects this build; `--msu-game --rom "/path/to/patched.sfc"` also works. The patched ROM and soundtrack are player supplied.

Save states and rewind do not restore the MSU-1 stream position; music may resume from a different point after using them. Full soundtrack and mode compatibility are still experimental.

## Achievements

The local catalog adapts 36 challenges and badges from [RetroAchievements game 562](https://retroachievements.org/game/562), plus **One Go, Fourteen Foes** for clearing Boss Rush in one attempt. Tracking can be disabled or reset from Achievements.

Save-state loading and rewind invalidate ongoing damage-free, full-quest, and single-attempt Boss Rush challenge eligibility. Earned progress remains. During Boss Rush, only its completion achievement is eligible. Start a fresh campaign or run after enabling or resetting tracking for full-run challenges.

## Development

| Path | Purpose |
| --- | --- |
| `src/` | Game integration, menus, modes, achievements, and rendering |
| `recomp/` | Analysis configuration and symbols |
| `assets/` | Launcher artwork, achievement badges, and sound |
| `tools/` | Generation, dependency patches, and regression checks |
| `scripts/` | Setup-pack packaging |
| `snesrecomp/`, `recomp-ui/` | Pinned upstream dependencies |

Regression scripts cover achievements, Boss Rush, Level Select, One Life, Other Castle, title menus, timing, shaders, and rewind. Some smoke checks require a built executable and the local ROM; see each `tools/test-*.ps1` script for options. Widescreen checks are retained for development.

## Motion interpolation (local development)

This experimental feature is present in the current local development build. Its implementation is not included in the published v0.1.0 tag or the current GitHub source.

Experimental sprite/camera interpolation is available in the title menu under
Options > Video > Motion (Experimental), or in the launcher/pause menu under
Display Options > Motion (Experimental). It is off by default and persists in
interpolation.cfg. SCIV_INTERPOLATION=1 enables it for a test run. Game updates
retain their original clock; presentations target the display refresh above
60 Hz, otherwise approximately 120 Hz. A high-refresh display is needed to
see the extra frames. Standard Mode 1 gameplay interpolates compatible OAM
positions and BG1/BG2/BG3 scrolls below the HUD at native pixel resolution, with current-frame
artwork. The HUD strip, menus, Mode 7, widescreen, room changes, sprite identity
changes and large jumps retain original positions. This adds up to one game
frame of visual latency. Motion automatically disables Frame Blend to prevent
sprite trails; enabling Frame Blend in the title menu turns Motion off. Other
Castle and MSU variants have not been visually validated. The replay stores
about 15 MiB of raster state and adds rendering work per intermediate frame.


When BG1/BG2/BG3 tilemaps change (for example, a ledge crumbles), Motion presents
an original frame for that transition instead of interpolating changed terrain.
Sprite motion also requires unchanged underlying tile artwork, not only matching
OAM tile numbers. This conservative fallback can briefly reduce smoothness.

OAM is draw order rather than object identity: sprite matching now searches
across slots for a unique mutual nearest match in world coordinates with
unchanged artwork. This keeps repeated ledge tiles stationary when the game
rotates its sprite table; ambiguous matches retain the camera-only fallback.

## Credits and license

Built on snesrecomp and recomp-ui. Achievement challenge names and badges are credited to the RetroAchievements community. Shader attribution and dependency notices remain in the corresponding dependency trees.

Project source uses the [PolyForm Noncommercial License 1.0.0](LICENSE). Dependencies have their own licenses; see [framework attribution](snesrecomp/THIRD_PARTY_ATTRIBUTION.md). Game content and supplied artwork remain subject to their respective owners' rights. This is an unofficial fan project and is not affiliated with or endorsed by Konami.
