# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A port of the [PaulStoffregen/Audio](https://github.com/PaulStoffregen/Audio) Teensy audio library to x86 (Linux/macOS). The Teensy hardware-specific pieces (Arduino core, `AudioStream`, SD card) are replaced by stub libraries so Teensy audio-graph sketches can compile and run on a desktop. Real audio I/O is provided through [libsoundio](https://github.com/andrewrk/libsoundio).

## Build

Out-of-source CMake build (in-source builds are not supported):

```sh
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .                       # or: make
```

- The two stub dependencies (`teensy-x86-stubs`, `teensy-x86-sd-stubs`) and `cmake-declare-and-fetch` are pulled automatically at configure time via CMake `FetchContent` — no manual install needed. This requires network access during `cmake`.
- The core library target is `teensy_audio_x86_stubs` (static lib built from `src/`).
- The soundio library `teensy_audio_x86_stubs_soundio` is only added if `find_library(LIB_soundio soundio)` succeeds, i.e. **libsoundio must be installed** (`libsoundio-dev` on Debian/Ubuntu). Build just it with `make teensy_audio_x86_stubs_soundio`.
- Examples under `extras/soundio/examples/` are only configured when `-DBUILD_SOUNDIO_EXAMPLES=ON` is passed (and libsoundio is present).

There is no test/lint runner wired into CMake; "tests" in CI are the compile of the libraries themselves. The two GitHub Actions (`.github/workflows/audio-x64.yml`, `soundio.yml`) just configure + build in Debug. `audio-x64` builds everything; `soundio` installs `libsoundio-dev` and builds only the soundio target.

## Architecture

Three layers, separated by directory:

1. **`src/` — the audio library port.** These files keep the *same names and class APIs* as PaulStoffregen/Audio (`mixer.cpp`, `synth_sine.cpp`, `effect_*`, `filter_*`, `analyze_*`, `play_*`, `record_*`, etc.). They are DSP/graph nodes deriving from `AudioStream`. `Audio.h` is the umbrella include. When porting or fixing a node, treat the upstream Teensy file as the source of truth and keep the API identical.

2. **Stub dependencies (fetched, not in this repo).** `teensy-x86-stubs` provides `Arduino.h`, `AudioStream.h`, `AudioConnection`, `audio_block_t`, `AudioMemory()`, `Serial`, etc. `teensy-x86-sd-stubs` provides the SD/filesystem stubs used by `play_sd_*` / `record_queue`. The hardware I/O nodes in `src/` (`input_i2s`, `output_i2s`, `input_adc`, `output_dac`, `output_pwm`, …) compile but do nothing real on x86 — they exist only so sketches link.

3. **`extras/soundio/` — real I/O via libsoundio.** `AudioOutputSoundIO` / `AudioInputSoundIO` are `AudioStream` nodes (in `extras/soundio/src/`) that bridge the Teensy audio graph to the host sound card, replacing the no-op I2S nodes. This is the piece that makes a graph audible. `extras/soundio/shared/` holds in-memory `AudioSample*` waveform data used by examples.

A program here is a **Teensy Audio sketch**: instantiate `AudioStream` nodes, wire them with `AudioConnection`, call `AudioMemory(n)` in `setup()`, and let the update ISR pull audio. See `extras/soundio/examples/basic/basic.cpp` for the canonical shape (`setup()`/`loop()`, `AudioConnection`, an `AudioOutputSoundIO` sink). The `teensy_main.h` entry point comes from the stub libs.

## Conventions

- C++11 (`CMAKE_CXX_STANDARD 11`), C for `data_*.c` / `*.c` utility files.
- When adding a node to `src/`, you must register both the `.cpp` and `.h` in the `SOURCE_FILES`/`HEADER_FILES` lists in `src/CMakeLists.txt` — there is no globbing.
- macOS/XCode 15 has a libsoundio dynamic-loading bug (missing `_soundio_backend_name`); fix documented in `README.md` via `install_name_tool`.
