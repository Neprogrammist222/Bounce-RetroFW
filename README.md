# Nokia Bounce — RetroFW Port (RS-97)

C++ / SDL1.2 port of the classic Nokia **Bounce** game for **RS-97** handheld consoles running **RetroFW** (with host PC build support).

## Features
- **Classic Gameplay:** Faithful recreation of Nokia Bounce mechanics, physics, rings, spikes, inflators/deflators, and level progression.
- **Native RetroFW Support:** Runs natively on RS-97 (`320×240`) with `.opk` packaging and persistent progress saving (`bounce.sav`).
- **One-Command Build:** Includes `Makefile` for MIPS32 (`mipsel-RetroFW-linux-uclibc-g++`) and host Linux builds, plus `build_all.sh` to bootstrap the RetroFW toolchain from scratch.

## Controls (RS-97)

| Button | Action |
| :--- | :--- |
| **D-Pad Left / Right** | Roll Left / Right |
| **A / D-Pad Up** | Jump / Select in Menu |
| **B** | Back to Menu |
| **Start** | Pause / Confirm |
| **Select** | Back / Exit |

## Building from Source

1. **Build `.opk` for RS-97 (requires RetroFW Buildroot toolchain):**
   ```bash
   make opk
   ```
   *(If you don't have the toolchain installed yet, run `./build_all.sh` to automatically clone and build `retrofw-buildroot` with SDL & SDL_image and package `bounce.opk`.)*

2. **Build for Host PC (for testing):**
   ```bash
   make host
   ./bounce_host
   ```

3. Copy the resulting `bounce.opk` to `apps/` on your SD card.

## Credits
- Original J2ME game: **Nokia** / **Rovio Entertainment**
- Decompiled Java source code, level data & sprite assets: **[rndtrash](https://github.com/rndtrash)** ([nokia-bounce-decomp](https://github.com/rndtrash/nokia-bounce-decomp))
- C++ / SDL1.2 RetroFW port & sound synthesizer: **Neprogrammist222**
