# CannonBall PS5 v1.0.0 verification

The RC3 executable passed all 18 planned hardware
acceptance tests on November 23, 2026.

Test environment:

- Real PS5 hardware
- Firmware 13.40
- ShadowMountPlus 1.7 Beta 12

Verified functionality:

- Native dashboard launch
- 1920 × 1080 display output
- Smooth 60 FPS gameplay
- DualSense controls
- Music and sound effects
- Hi-Res, Enhanced and Widescreen modes
- Pause menu and game restart
- Persistent settings and high scores
- Application exit and relaunch

Independent compilation using build-ps5.sh also
completed successfully.

No precompiled SDL2 or Boost libraries were reused.

The native runtime and optimized PS5 video object
were reproduced byte-for-byte.

All non-executable application files matched.

No compiler warnings were reported.

The independently rebuilt executable was not
byte-for-byte identical to the hardware-tested RC3.

The official release candidate retains the exact
hardware-tested executable.

Reference SHA-256:

7ed94f285b7f9fcc47a3fa741a05e2f35c8f6c14ab2064ba86160f63e4452db6
