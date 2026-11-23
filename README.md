# Lyrics Display

A standalone C++ / JUCE 8 **VST3** audio plugin (plus Standalone app) for **Ableton Live 11** (and other VST3 hosts) that searches, edits, and displays synchronized karaoke lyrics locked to your DAW transport.

## Features

- **DAW Transport Sync:** Zero-latency stereo pass-through audio effect that reads host transport position (`timeInSeconds`, `isPlaying`, `bpm`) and smoothly highlights and auto-scrolls the active lyric line at 60 FPS.
- **Direct LRCLIB Search:** Queries [LRCLIB](https://lrclib.net) asynchronously over HTTPS (UTF-8 Latin & CJK supported) with synced-first ranking.
- **Built-in Editor & Import/Export:** Import or export `.lrc`, `.txt`, and `.json` files, or edit raw LRC timestamps and plain lyrics inline (`Edit` button or double-click the lyrics view).
- **Pin Across Tracks (Default On):** Includes a pushpin toggle button in the header that keeps the lyrics display pinned on top in a floating window whenever Ableton hides or closes the plugin frame as you focus other tracks — and hands back seamlessly when you return to the plugin's track.
- **Timing Offset & Project Persistence:** Adjust lyric timing in `-0.5s` / `+0.5s` increments (click the offset readout to reset), scale font size (`A-` / `A+`), toggle Dark/Light themes, and automatically persist lyrics and timing offset inside your DAW project file (`.als`).
- **Full Unicode & CJK Rendering:** Uses Windows DirectWrite typography for clean Chinese, Japanese, Korean, and Latin display.

## Requirements

- **CMake** 3.22+
- **C++20 compiler** (e.g., Visual Studio 2022 with MSVC on Windows)
- **Git** (to fetch [JUCE 8](https://github.com/juce-framework/JUCE))

## Building from Source

1. Clone JUCE into `third_party/JUCE` (kept out of git via `.gitignore`):

   ```bash
   git clone --depth 1 https://github.com/juce-framework/JUCE.git third_party/JUCE
   ```

2. Configure and build the Release targets:

   ```bash
   cmake -B build -G "Visual Studio 17 2022" -A x64
   cmake --build build --config Release --parallel
   ```

3. Run unit tests:

   ```bash
   ./build/Release/LyricsModelTests
   ```

### Build Outputs

- **VST3 Plugin:** `build/LyricsDisplay_artefacts/Release/VST3/Lyrics Display.vst3`
- **Standalone App:** `build/LyricsDisplay_artefacts/Release/Standalone/Lyrics Display.exe`
- **Unit Tests:** `build/Release/LyricsModelTests.exe`

## Using in Ableton Live 11

Choose either option:

1. **System VST3 Folder:** Copy `Lyrics Display.vst3` into your system VST3 folder (on Windows, `C:\Program Files\Common Files\VST3`), enable **Use VST3 Plug-In System Folders** in **Preferences → Plug-Ins**, and click **Rescan**.
2. **Custom VST3 Folder:** Enable **Use VST3 Plug-In Custom Folder** in **Preferences → Plug-Ins**, point it to the `build/LyricsDisplay_artefacts/Release/VST3` folder, and click **Rescan**.

Then drag **VST3 → AClouD → Lyrics Display** onto any Audio Track or the Master Track.

## Project Structure

```text
.
├── CMakeLists.txt
├── src/
│   ├── LyricsModel.h / .cpp        # LRC parser, query cleaner & JSON model
│   ├── LyricsService.h / .cpp      # Async LRCLIB HTTPS client, file import/export & settings
│   ├── LyricsViewport.h / .cpp     # 60 FPS smooth-scrolling karaoke display component
│   ├── PluginProcessor.h / .cpp    # Pass-through VST3 processor, playhead sync & state
│   ├── PluginEditor.h / .cpp       # Studio UI, search candidates & raw lyrics editor
│   └── Theme.h                     # Dark/Light palettes & CJK-capable font helpers
└── tests/
    └── LyricsModelTests.cpp        # Unit tests for LRC parsing, lookup & JSON round-trip
```

## Acknowledgments

- [JUCE](https://juce.com/)
- [LRCLIB](https://lrclib.net/)
