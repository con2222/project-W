# Music Player

A desktop music player written in C++20 with a custom Dear ImGui interface and GPU-powered audio visualizations using WebGPU (Dawn).

The project is built primarily for learning graphics programming, audio processing, and application development.

## Features

- Local music library and audio playback
- Playback controls, seeking, and volume adjustment
- Music search and metadata support
- FFT-based audio spectrum analysis
- Frequency bands (bass, mid, treble)
- Audio-reactive visualizations written in WGSL
- Custom user interface with Dear ImGui

## Tech Stack

| Technology | Purpose |
|---|---|
| C++20 | Core application |
| WebGPU (Dawn) | GPU rendering |
| WGSL | Shaders and visual effects |
| SDL3 | Window management and input |
| Dear ImGui | User interface |
| miniaudio | Audio playback |
| TagLib | Audio metadata |
| CMake | Build system |

## Building

The project uses CMake.

```bash
cmake -S . -B build
cmake --build build # set USE_WAYLAND or USE_X11 on linux
```

Dependencies must be installed or configured before building.

## Status

**Work in Progress**

This is a personal learning project. Features and architecture are actively evolving.

## License

This project is licensed under the MIT License.
See [LICENSE](LICENSE) for details.
