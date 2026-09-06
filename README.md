# SDLPoP for PlayStation Portable (PSP)

Native port of **SDLPoP** (open-source Prince of Persia 1 engine) for the Sony PlayStation Portable (PSP-1000 / PSP-2000 / PSP-3000 / PSP-Go / PS Vita Adrenaline) and PPSSPP emulator, built using the modern `pspdev` toolchain.

---

## Key Features & Enhancements

- **Decoupled 1:1 Integer Text & Menu Overlay**:
  - The in-game pause menu, settings sub-menus, level customization dialog, and confirmation dialogs bypass the game playfield and render at **strict 1:1 integer scale ($320 \times 200$)** centered over a GPU-blended semi-transparent dimmed backdrop.
  - Eliminates all font distortion, blurred edges, and text shimmering on the PSP screen, regardless of the chosen game aspect ratio or widescreen stretch.
- **$2\times$ Integer Status Bar / HUD Split**:
  - **Playfield ($192\text{ px}$)**: Scaled to $256\text{ px}$ ($192 \times 4/3$).
  - **Status bar / HUD ($8\text{ px}$)**: Scaled to $16\text{ px}$ ($8 \times 2$) — an exact $2\times$ integer vertical scale.
  - Eliminates scanline interpolation and shimmering on the Kid's HP triangles, guard's HP triangles, and bottom status messages ("GAME PAUSED", "LEVEL 1", time remaining).
- **Multiple Display Modes**:
  - `16:10`: Authentic Prince of Persia aspect ratio ($436 \times 272$, 22px symmetrical black pillars).
  - `16:9 Wide`: Full widescreen stretch across the entire screen ($480 \times 272$).
  - `4:3`: Authentic DOS CRT aspect ratio ($362 \times 272$, 59px symmetrical black pillars).
- **On-Demand OGG Music Streaming**:
  - Bundled with all 22 official authentic DOS OGG music tracks in `data/music/`.
  - Streams on-demand via `stb_vorbis` without startup preloading or memory bloat (~150KB peak RAM vs 20MB).
- **Expanded Hardware Memory**:
  - Unlocked the full 64MB user memory partition on PSP-2000/3000/Go (`MEMSIZE 1` in PRX).
  - Configured a 51MB+ continuous newlib runtime heap (`PSP_HEAP_SIZE_KB(-2048)`).
- **Hardware-Tuned Audio**:
  - Increased DMA audio buffer to 2048 samples (46.4ms) to eliminate buffer underruns and comb-filter phasing on hardware.
  - Safe saturation clamping to $[-32768, 32767]$ prevents digital clipping when music and sound effects overlap.
  - Lightweight stack chunk mixing replaces per-chunk heap allocations in real-time audio thread.
- **Ergonomic Handheld Controls**:
  - Smooth action on Face buttons and Shoulder triggers tailored for handheld play.

---

## Controls

| PSP Button | Function in Game | Function in Menus |
|---|---|---|
| **D-Pad / Analog Stick** | Move Left / Right, Crouch Down, Jump Up | Navigate Options |
| **Cross ($\times$)** | Action / Shift (Grab ledge, strike sword, careful walk) | Confirm / Select |
| **Square ($\square$)** | Action / Shift (Grab ledge, strike sword, careful walk) | Confirm / Select |
| **Triangle ($\triangle$)** | Jump Up / Climb Ledge | - |
| **Circle ($\bigcirc$)** | Jump Up / Climb Ledge | Cancel / Back |
| **L / R Shoulders** | Action / Shift (easy hold while running) | - |
| **Start** | Pause Game / In-Game Menu | Close / Return to Game |
| **Select** | Display Time Remaining | - |

---

## Installation & Playing on Real Hardware

1. Download or build `EBOOT.PBP`.
2. Connect your PSP to your computer via USB (or insert the Memory Stick).
3. Copy the `SDLPoP` folder to:
   ```
   ms0:/PSP/GAME/SDLPoP/
   ├── EBOOT.PBP
   ├── SDLPoP.ini
   └── data/
   ```
4. Disconnect USB and launch **Prince of Persia** from **Game $\to$ Memory Stick** on your PSP XMB.

---

## Building from Source

### Prerequisites
- [pspdev](https://github.com/pspdev/pspdev) toolchain installed (includes GCC, `psp-cmake`, `pack-pbp`, SDL2, SDL2_image).

### Build Command
Run the included build script:
```bash
./build.sh
```

This will:
1. Run `psp-cmake` targeting PRX mode (`BUILD_PRX=ON`) with CMake 3.10+.
2. Compile and link `prince.prx`.
3. Package the final `EBOOT.PBP` with PSP icon and title metadata.
4. Assemble a ready-to-use distribution bundle into `dist/SDLPoP/`.

---

## Automated Deployment

```bash
./deploy.sh
```

Automatically detects and deploys to:
1. **PPSSPP Emulator**: `~/.config/ppsspp/PSP/GAME/CAT_Homebrew/SDLPoP/` and `~/.config/ppsspp/PSP/GAME/SDLPoP/`.
2. **Physical PSP Hardware**: Direct USB mounts (`/Volumes/NO NAME/...`) and network automounts (e.g. via `ssh n`).

---

## In-Game Configuration

Configure settings in `SDLPoP.ini` or on-the-fly in the in-game **Settings $\to$ Visuals** menu:

```ini
; Display mode on PSP (480x272 screen):
; * 16:10 = Authentic Prince of Persia aspect ratio (436x272, 22px pillars). (default)
; * wide  = Full widescreen stretch across the entire screen (480x272).
; * 4:3   = Authentic DOS CRT pillarbox (362x272, 59px pillars).
psp_display_mode = 16:10

; HUD 2x integer split during gameplay (levels 1-14):
; Splits the screen into 256px playfield + 16px (2x integer) status bar.
enable_hud_split = true

; Decoupled 1:1 integer overlay:
; Renders pause/settings menu at 1:1 integer scale (320x200) over dimmed backdrop.
decouple_menu_overlay = true
```

---

## Original Documentation

For upstream PC/DOS documentation, modding specifications, CusPoP support, replays, and technical notes, see [README.upstream.md](README.upstream.md).

---

## Credits

- **Dávid Nagy** and contributors: [SDLPoP](https://github.com/NagyD/SDLPoP)
- **Jordan Mechner**: Creator of the original Prince of Persia
- **PSPDEV**: [pspdev](https://github.com/pspdev/pspdev) toolchain and SDK libraries
- **striga, sharkwouter**: Initial PSP scaffolding
- **chanunnaki**: Enhanced PSP port (decoupled 1:1 overlay, 2x integer HUD split, streaming audio, 64MB RAM unlock, modern CMake build)
