# Building CannonBall PS5

## Requirements

The standalone build procedure was verified using
Ubuntu 24.04 running under WSL2.

Reference toolchain:

- Clang 18.1.3
- CMake 3.28.3
- GNU Make 4.3
- Python 3.12
- Git 2.43

Install the host development tools:

```bash
sudo apt update
sudo apt install build-essential clang-18 lld-18 \
    cmake git curl python3 bzip2 unzip pkg-config
```

## Build

From the repository root:

```bash
bash build-ps5.sh
```

The initial build requires an internet connection
and sufficient disk space for the dependencies.

The script automatically retrieves and verifies the
pinned dependencies, reconstructs the patched Boost
and SDL2 source, compiles the native runtime and
builds the application.

## Dependencies

| Dependency | Version |
|---|---|
| PS5 Payload SDK | v0.42 |
| PacBrew | v0.40.2 |
| SDL2 | 2.30.12 |
| Boost | 1.83.0 |
| zlib | 1.3.2 |

## Output

The native application folder is generated at:

`dist/PPSA98537/`

A SHA-256 checksum manifest is generated at:

`dist/PPSA98537-SHA256SUMS.txt`

No ROM files are downloaded or included.

## Parallel builds

The default SDL2 build uses two parallel jobs.

To change the number of jobs:

```bash
BUILD_JOBS=4 bash build-ps5.sh
```

## Important

Do not replace the customized SDL2 library with an
unmodified upstream or precompiled version.

The patched PS5 video backend contains essential
framebuffer memory and performance modifications.

Independent source compilation has been verified.
Complete executable checksums may differ between
build environments.

See [Build verification](BUILD-VERIFICATION.md)
and [ROM requirements](ROM_REQUIREMENTS.md).
