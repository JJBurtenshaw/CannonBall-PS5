# CannonBall PS5 — Third-party notices

## Native build dependencies

The application build uses LLVM/Clang/lld, zlib 1.3.2, and the public
[PS5 payload SDK](https://github.com/ps5-payload-dev/sdk). The bootstrapper
downloads SDK v0.42 after verifying SHA-256
`8cfbc7cd5811e719eb4f0c47eea668d3dc7b40bc8ab11c4a5031d40c23ec02da`.
It downloads zlib 1.3.2 from the upstream source archive after verifying
SHA-256 `bb329a0a2cd0274d05519d61c667c062e06990d72e125ee2dfa8de64f0119d16`
and compiles its static archive locally. Both dependencies remain under ignored
`.deps/native/`, retain their upstream licenses, and are not distributed by
this repository. No Sony SDK file is included.

Target C++ compilation uses the LLVM libc++ headers distributed by the public
SDK. Those headers retain the Apache-2.0 WITH LLVM-exception license recorded
upstream. The application does not redistribute or dynamically load the
complete libc++ or libc++abi archives.

The project’s PS5 ELF converter and FSELF writer are independently authored
GPL-3.0-or-later code. SharpProspero was a useful public format reference during
development but is not fetched, copied, linked, or required by the build.

## Host test dependency

The host unit-test target downloads
[GoogleTest](https://github.com/google/googletest) 1.17.0 after verifying
SHA-256 `65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c`.
It remains under ignored `.deps/test/`, retains its BSD-3-Clause license, and
is not linked into any PS5 application, runtime, or package artifact.

## Optional PacBrew dependencies

When selected through `PACBREW_*` build variables, the build downloads the prebuilt ports image
from [ps5-payload-dev/pacbrew-repo](https://github.com/ps5-payload-dev/pacbrew-repo)
release `v0.40.2`, verifies its published SHA-256, and extracts only the
`target/user/homebrew` prefix under ignored `.deps/pacbrew/`. It does not
replace the pinned SDK or install files globally. PacBrew recipes and every
linked third-party library retain their upstream licenses; applications must
review those terms before redistribution.

## Optional UFS2Tool dependency

When `.ffpkg` output is requested, the platform bootstrapper fetches
[SvenGDK/UFS2Tool](https://github.com/SvenGDK/UFS2Tool) at commit
`b5307a60d5b4e3a68ba680e0e33cfadf05017c77` into the ignored
`.deps/UFS2Tool` cache and builds it with the host .NET SDK. UFS2Tool is
BSD-2-Clause software and is not distributed by this repository.

## Optional MkPFS dependency

When `.ffpfsc` output is requested, the platform bootstrapper fetches
[PSBrew/MkPFS](https://github.com/PSBrew/MkPFS) at commit
`6cb8313dfe0c988ac52617794553f343243d3a56` into the ignored `.deps/MkPFS`
cache and installs its Python dependencies into an ignored virtual environment
there. MkPFS and its dependencies retain their own licenses and are not
distributed by this repository.

## Independently authored runtime shim

`tooling/native/libc_builder.cpp` and the manifests under
`tooling/native/runtime/` are independently authored for this project and
licensed under GPL-3.0-or-later. The generated `runtime/libc.prx` contains
project-authored compatibility stubs, startup code, and semantic loader
metadata. It contains no Sony runtime implementation.

Original ps5-native-app-boilerplate code is Copyright (C) 2026
BlackBearReloaded and licensed under GPL-3.0-or-later. Source and script files
carry matching SPDX identifiers.

## CannonBall PS5 presentation assets

The PS5 icon and splash artwork were created for
the CannonBall PS5 project.

The original BlackBearReloaded presentation assets
and default selection track are not included.

Artwork redistribution rights remain subject to
final release review.


## CannonBall engine

Copyright Chris White and the CannonBall team.
All rights reserved.

Original project:
https://github.com/djyt/cannonball

CannonBall retains its original non-commercial
redistribution license.

See:
third-party/cannonball/LICENSE.txt

## SDL2

The PS5 port uses a modified SDL2 2.30.12
implementation based on the PS5 Payload SDK fork.

https://github.com/ps5-payload-dev/SDL

The pinned upstream revision, original license
and complete project patch are preserved under
third-party/sdl2/.

## Boost

The build uses Boost 1.83.0 under the Boost
Software License 1.0.

The original license and local modifications
are preserved under third-party/boost/.

## CannonBall PS5 port

PS5 integration, implementation, optimization
and hardware testing: JJBurtenshaw.

https://github.com/JJBurtenshaw/CannonBall-PS5

Special thanks to BlackBearReloaded, the
PS5 Payload SDK contributors, PacBrew contributors,
SDL contributors and the ShadowMountPlus team.

## Game data

No original OutRun ROM files are distributed
with CannonBall PS5.

OutRun is a trademark of SEGA.

This project is unofficial and is not affiliated
with SEGA or Sony Interactive Entertainment.
