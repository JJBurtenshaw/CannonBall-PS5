# CannonBall PS5

**A native PlayStation 5 port of CannonBall — the enhanced OutRun engine.**

Experience the classic 1986 arcade racer on PS5 with smooth 60 FPS gameplay, enhanced widescreen graphics, improved sprite scaling and native DualSense controls.

Based on [CannonBall](https://github.com/djyt/cannonball) by Chris White and the CannonBall team.

## Features

- Native PS5 application with dashboard integration
- 1920 × 1080 display output
- Smooth 60 FPS gameplay
- Optimized software rendering for PS5
- Widescreen and Hi-Res graphics modes
- Original and Enhanced gameplay modes
- Continuous Mode and Time Trial Mode
- Analog steering, acceleration and braking
- Manual and automatic transmission options
- Original arcade music and sound effects
- In-game pause menu with touchpad support
- Configurable gameplay enhancements
- Persistent settings and high scores

## Download

[Latest release](https://github.com/JJBurtenshaw/CannonBall-PS5/releases/latest)

**OutRun ROM files are not included.**

## Requirements

- A PS5 capable of running compatible native homebrew applications.
- A compatible homebrew launcher, such as ShadowMountPlus.
- The required original OutRun arcade ROM files.

Developed and tested on real PS5 hardware using firmware 13.40 and ShadowMountPlus 1.7 Beta 12.

Compatibility with other firmware versions and launchers has not been fully tested.

## Installation

1. Download and extract the latest release ZIP.
2. Place your legally obtained OutRun ROM files in the `PPSA98537/roms/` directory.
3. Transfer the complete `PPSA98537` folder to `/data/homebrew/` on your PS5 using FTP.
4. Register the application using ShadowMountPlus.
5. Launch CannonBall PS5 directly from the dashboard.

See [ROM requirements](docs/ROM_REQUIREMENTS.md) for the required filenames.

## Default DualSense Controls

| Control | Action |
|---|---|
| Left analog stick | Analog steering |
| D-pad | Digital steering / menu navigation |
| R2 | Analog acceleration |
| L2 | Analog braking |
| R1 | Digital acceleration |
| L1 | Digital braking |
| Cross | Gear change / select |
| Circle | Secondary gear control / pause-menu back |
| Square | Change camera view |
| Triangle | Insert coin |
| Options | Arcade Start |
| Touchpad click | Pause / resume |

The secondary gear control is used when the corresponding transmission mode is selected.

The default configuration uses Free Play, so inserting coins is normally unnecessary.

## Game Modes

**Original Mode** — Play with the original arcade gameplay settings.

**Enhanced Mode** — Experience CannonBall's enhanced gameplay settings.

**Continuous Mode** — Drive through the game's stages in a continuous run.

**Time Trial Mode** — Race against the clock.

## Settings

CannonBall PS5 includes configurable video, audio, controls and gameplay settings.

| Setting | Default |
|---|---|
| Display output | 1920 × 1080 |
| Frame rate | 60 FPS |
| Widescreen | On |
| Hi-Res | Off |
| FPS counter | Off |
| Analog controls | On |
| Transmission | Manual |
| Free Play | On |

Press the DualSense touchpad during gameplay to access the pause menu.

Settings and high scores are stored in the application's persistent storage and retained between sessions.

## Building from Source

The project includes a standalone build script for Ubuntu 24.04 and compatible Linux environments, including WSL2.

```bash
git clone https://github.com/JJBurtenshaw/CannonBall-PS5.git
cd CannonBall-PS5
bash build-ps5.sh
```

The build script downloads the pinned dependencies, reconstructs the modified SDL2 and Boost implementations, compiles the native runtime and builds the PS5 application.

The resulting application folder is located at:

`dist/PPSA98537/`

See [Building from source](docs/BUILDING-PS5.md) for prerequisites and further information.

## Technical Notes

CannonBall PS5 uses a native software-rendering implementation optimized for the PlayStation 5.

The port includes:

- A customized SDL2 PS5 video backend.
- Cached direct-memory framebuffers for efficient CPU rendering.
- An optimized FastBlit rendering path.
- Native controller input and persistent storage.

The release executable has been validated on real PS5 hardware. Independent source compilation has also been verified, although rebuilt executables are not necessarily byte-for-byte identical.

## Credits

- **Chris White and the CannonBall team** — Original CannonBall engine.
- **SEGA and the original OutRun development team** — OutRun.
- **BlackBearReloaded** — Native PS5 application boilerplate and runtime implementation.
- **PS5 Payload SDK contributors** — PS5 development toolchain.
- **SDL contributors** — SDL2 and its PS5 backend.
- **PacBrew contributors** — PS5 library dependencies.
- **Boost contributors** — Boost C++ libraries.
- **ShadowMountPlus contributors** — Native homebrew launching.
- **Jonthan J. Burtenshaw** — CannonBall PS5 port, integration, optimization and hardware testing.
- **ChatGPT** — ;) 

## Licensing

CannonBall retains its original non-commercial redistribution license. Individual third-party components retain their respective licenses.

See the accompanying license files and third-party notices for details.

## Disclaimer

CannonBall PS5 is an unofficial, non-commercial homebrew project.

OutRun is a trademark of SEGA. PlayStation is a trademark of Sony Interactive Entertainment.

This project is not affiliated with or endorsed by SEGA or Sony Interactive Entertainment.

No original OutRun ROM files are distributed with this project.
