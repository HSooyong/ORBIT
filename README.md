# ORBIT — Gravity Warp (M1 sound prototype)

New product (plugin code `OrGw`, bundle `com.pluginlab.orbit.gravitywarp`), no v0.3 state compatibility.
Design: `/workspace/orbit-design/ORBIT-GravityWarp-설계서.md`. Base reference: Mac v0.3 copy at `/workspace/ORBIT-v03-mac`.

- `Source/engine/WarpEngine.h` – JUCE-free DSP: orbit (Kepler, Stages, Hold, PPQ sync incl. 4/1 2/1 1/2D 1/4D),
  gravity mapper (d -> g, 3 zones, ±100 cent cap), 1–4 voice Hermite modulated delay, feedback ≤70 %, Tone, Tape, Horizon Bloom.
- `Source/engine/Presets.h` – 7 factory presets. `Source/PluginProcessor.*` – 25 APVTS params, generic editor (M1).
- `tests/render_test.cpp` – offline tests + WAV renders (`renders/`).

Build: `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j8`
Test:  `./build/orbit_render_test renders`
Validate: `xvfb-run -a pluginval --strictness-level 5 --validate-in-process "build/ORBIT_GravityWarp_artefacts/Release/VST3/ORBIT Gravity Warp.vst3"`

## M2 (2026-10-26)
- Custom 920x640 GUI: `Source/PluginEditor.*`, `Source/OrbitLookAndFeel.*` (from v0.3), `Source/GravityField.*` (black hole visual + planet/hole drag).
- Snapshot: `xvfb-run -a build/orbit_gui_snapshot_artefacts/Release/orbit_gui_snapshot docs 3` -> `docs/gui_m2.png`, `docs/gui_m2_advanced.png`.
- Tests: `build/orbit_render_test renders` (adds mono-sum, zipper, CPU). Bloom A/B: `build/orbit_bloom_ab renders/ab` -> `tests/bloom_ab_results.txt`.
- renders/ and build logs are git-ignored.
- Bloom zone A/B vs 350c85d engine: `build/orbit_bloom_zone_ab` -> `tests/bloom_zone_ab_results.txt`.
