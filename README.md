# SDLPoP-PSP
Native port of **Prince of Persia** (SDLPoP) for the Sony PlayStation Portable (PSP-1000 / PSP-2000 / PSP-3000 / PSP-Go / PS Vita Adrenaline) and PPSSPP emulator, built using the modern `pspdev` toolchain.

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
- **Switchable Visual Asset Packs (DOS / SNES / SNES Alt)**:
  - Switch graphics styles live on-the-fly from the in-game **Visuals $\to$ Graphics pack** menu or via `SDLPoP.ini`:
    - **DOS**: Classic authentic PC MS-DOS visuals (`res_dos.pak`, default).
    - **SNES**: Super Nintendo 16-bit sprites and color palettes (`res_snes.pak`).
    - **SNES Alt**: Hybrid 256-color palette adaptation by Maurice Kaltofen / mk1995 (`res_snes_alt.pak`).
  - **Zero-Latency In-RAM PAK Architecture**: Assets are pre-baked into binary archives (`data/res_*.pak`), replacing ~1,000+ loose file operations on the slow Memory Stick with instant in-memory switching while preserving the authentic original 12-level campaign and gameplay.
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
- **In-Game MODS & Levelset Selector**:
  - Dynamically scans the `mods/` directory and lists custom mods alongside the **Original Game** directly inside the in-game menu (**Settings $\to$ MODS**).
  - Displays active levelset status (`ACTIVE` indicator) and switches mods on-the-fly without returning to the PSP XMB.
  - Seamless in-memory soft reset reloads graphics, sounds, and levels, restarting directly into the mod's title and intro sequence.
  - Safe error recovery: if a configured mod folder is missing or deleted, the game warns the player, automatically reverts to the original game, and heals `SDLPoP.ini`.
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
3. Copy the `SDLPoP-PSP` folder to:
   ```
   ms0:/PSP/GAME/SDLPoP-PSP/
   ├── EBOOT.PBP
   ├── SDLPoP.ini
   ├── data/
   └── mods/
       ├── Princess of Persia/
       └── ...
   ```
4. Disconnect USB and launch **Prince of Persia** from **Game $\to$ Memory Stick** on your PSP XMB.

---

## Custom Mods Support

SDLPoP-PSP features full native support for custom levelsets and total conversion mods (available from [popot.org](https://www.popot.org/custom_levels.php)):

### Installing Mods
1. Create or open the `mods/` directory inside your game folder:
   ```
   ms0:/PSP/GAME/SDLPoP-PSP/mods/
   ```
2. Place any mod folder inside `mods/` (for example: `mods/Princess of Persia/`).
3. **Important for PSP / FAT32**: All `.DAT` files inside the mod directory must have uppercase names and extensions (e.g. `LEVELS.DAT`, `PRINCE.DAT`, `TITLE.DAT`).
4. Custom EXE modifications (`PRINCE.EXE`) and mod configs (`mod.ini`) inside the mod folder are fully supported and loaded automatically.

### Switching Mods In-Game
1. Press **Start** during gameplay to open the pause menu.
2. Navigate to **Settings $\to$ MODS**.
3. Highlight your desired mod (or **Original Game**) and press **Cross ($\times$)**.
4. Confirm the restart prompt. SDLPoP-PSP will immediately write the choice to `SDLPoP.ini`, reload all assets, and smoothly soft-reset directly into the mod's title and intro sequence!

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
1. **PPSSPP Emulator**: `~/.config/ppsspp/PSP/GAME/CAT_Homebrew/SDLPoP-PSP/` and `~/.config/ppsspp/PSP/GAME/SDLPoP-PSP/`.
2. **Physical PSP Hardware**: Direct USB mounts (`/Volumes/NO NAME/...`) and network automounts (e.g. via `ssh n`).

---

## In-Game Configuration

Configure settings in `SDLPoP.ini` or on-the-fly in the in-game **Settings $\to$ Visuals** menu:

```ini
; Levelset / Mod Selector:
; * original          = Play authentic Prince of Persia levels (default)
; * <Folder_Name>     = Load custom levelset from mods/<Folder_Name> (e.g. Princess of Persia)
levelset = original

; Graphics Pack Selector:
; * dos      = Classic DOS PC graphics (res_dos.pak) (default)
; * snes     = Super Nintendo 16-bit graphics (res_snes.pak)
; * snes_alt = Alternative SNES visual rework (res_snes_alt.pak)
graphics_pack = dos

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
- **chanunnaki**: Enhanced PSP port (decoupled 1:1 overlay, 2x integer HUD split, streaming audio, 64MB RAM unlock, modern CMake build, fast binary PAK asset runtime)

### Graphics Packs & Custom Level Authors
- **KJ**: Creator of *SNES Remnants* (`0000149` / `0000296` on [popot.org](https://www.popot.org/custom_levels.php?mod=0000149)), source for the **SNES Graphics Pack** (`res_snes.pak`):
  - **brain**: Authentic SNES guard palettes and sprites, Prince sprites, and Palace environment tiles (*Total Pack 3.0*).
  - **PoP_MoreSNES**: Character sprites (Kid, Fat guard, Skeleton, Jaffar/Vizier, Shadow, Princess & Vizier).
  - **Total Pack 3.0**: *SNES Blue* dungeon environment graphics.
  - **tV2 & programmer**: SNES-style intro cinematic backdrop (*Revolution mod*).
- **Maurice Kaltofen (mk1995)**: Creator of *SNES-PC-set* (`0000042` on [popot.org](https://www.popot.org/custom_levels.php?mod=0000042)), source for the **SNES Alt Graphics Pack** (`res_snes_alt.pak`):
  - 256-color palette adaptation and sprite rework bringing DOS PC Prince of Persia assets closer to the SNES visual presentation.
