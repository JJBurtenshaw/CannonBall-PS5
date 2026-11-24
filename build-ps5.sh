#!/usr/bin/env bash
#
# CannonBall PS5 v1.0.0
# Standalone native PS5 build procedure.
#
# Requires Ubuntu 24.04 or equivalent Linux environment.
# WSL2 is supported.
#
# No proprietary ROM files are downloaded or included.

set -Eeuo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

DEPS="$ROOT/.deps"
SDK="$DEPS/native/ps5-payload-sdk"
PORTS="$DEPS/pacbrew/v0.40.2/sysroot/user/homebrew"

SDL_SRC="$DEPS/cannonball-sdl2"
SDL_BUILD="$SDL_SRC/build-cannonball-release"

BOOST_ARCHIVE="$DEPS/boost_1_83_0.tar.bz2"
BOOST_URL="https://archives.boost.io/release/1.83.0/source/boost_1_83_0.tar.bz2"
BOOST_SHA="6478edfe2f3305127cffe8caf73ea0176c53769f4bf1585be237eb30798c3b8e"

SDL_COMMIT="ee4c47dc0d617b3bc8f35108f9956baf228a1322"
VIDEO_SHA="f7b7310daf71284217420b430e1ad73d52eaac481d4b29afd15cea1fdf2dd404"

TITLE="PPSA98537"
JOBS="${BUILD_JOBS:-2}"

export BUILD_JOBS="$JOBS"

trap 'echo "Build stopped at line $LINENO." >&2' ERR

echo "========================================"
echo " CANNONBALL PS5 v1.0.0"
echo " NATIVE APPLICATION BUILD"
echo "========================================"

echo
echo "=== 1. Check host requirements ==="

for COMMAND in \
    bash python3 git curl tar bzip2 \
    cmake make clang-18 clang++-18 \
    ld.lld-18 sha256sum
do
    command -v "$COMMAND" >/dev/null || {
        echo "Missing required tool: $COMMAND" >&2
        exit 1
    }
done

clang-18 --version | head -n 1
cmake --version | head -n 1

[[ "$JOBS" =~ ^[1-9][0-9]*$ ]]

if [[ -L "$DEPS" ]]; then
    echo "Refusing to use a symbolic-link dependency directory." >&2
    exit 1
fi

mkdir -p "$DEPS"

echo
echo "=== 2. Prepare application configuration ==="

if [[ ! -e .env ]]; then
    cp .env.example .env
fi

python3 - <<'PY'
from pathlib import Path

def settings(path):
    result = {}

    for line in Path(path).read_text().splitlines():
        line = line.strip()

        if not line or line.startswith("#") or "=" not in line:
            continue

        k, v = line.split("=", 1)
        result[k.strip()] = v.strip().strip('"').strip("'")

    return result

expected = settings("release-build-settings.txt")
actual = settings(".env")

for key, value in expected.items():
    if actual.get(key) != value:
        raise SystemExit(
            f"Release build configuration differs: {key}"
        )

print("Release configuration verified.")
PY

echo
echo "=== 3. Install pinned native dependencies ==="

bash tools/setup-native-dependencies.sh

test -x "$SDK/bin/prospero-clang"
test -f "$SDK/toolchain/prospero.cmake"

export PS5_PAYLOAD_SDK="$SDK"

echo
echo "=== 4. Install pinned PacBrew dependencies ==="

bash tools/setup-pacbrew-dependencies.sh --resolve sdl2

test -s "$PORTS/include/iconv.h"
test -s "$PORTS/lib/libiconv.a"

(
    cd "$PORTS"
    sha256sum -c \
        "$ROOT/third-party/sdl2/iconv-SHA256SUMS.txt"
)

# Match the isolated SDK configuration used for RC3.

mkdir -p \
    "$SDK/target/user/homebrew/include" \
    "$SDK/target/user/homebrew/lib"

install -m 644 \
    "$PORTS/include/iconv.h" \
    "$SDK/target/user/homebrew/include/iconv.h"

install -m 644 \
    "$PORTS/lib/libiconv.a" \
    "$SDK/target/user/homebrew/lib/libiconv.a"

echo
echo "=== 5. Retrieve and reconstruct Boost ==="

if [[ ! -f "$BOOST_ARCHIVE" ]]; then
    curl -fL --retry 3 \
        "$BOOST_URL" \
        -o "$BOOST_ARCHIVE.part"

    printf '%s  %s\n' \
        "$BOOST_SHA" "$BOOST_ARCHIVE.part" \
        | sha256sum -c -

    mv "$BOOST_ARCHIVE.part" "$BOOST_ARCHIVE"
fi

printf '%s  %s\n' \
    "$BOOST_SHA" "$BOOST_ARCHIVE" \
    | sha256sum -c -

TEMP="$(mktemp -d "$DEPS/boost-build.XXXXXXXX")"

trap 'rm -rf -- "$TEMP"' EXIT

tar -xjf "$BOOST_ARCHIVE" \
    -C "$TEMP" \
    --strip-components=1 \
    boost_1_83_0/boost

(
    cd "$TEMP"

    git apply --check \
        "$ROOT/third-party/boost/cannonball-ps5.patch"

    git apply \
        "$ROOT/third-party/boost/cannonball-ps5.patch"

    while IFS= read -r FILE; do
        [[ -z "$FILE" ]] && continue

        [[ "$FILE" != /* && "$FILE" != *..* ]]

        rm -- "boost/$FILE"

    done < "$ROOT/third-party/boost/excluded-files.txt"
)

python3 - "$TEMP/boost" <<'PY'
import sys
from pathlib import Path

root = Path(sys.argv[1])

files = [p for p in root.rglob("*") if p.is_file()]

assert len(files) == 15653, len(files)

print("Reconstructed Boost headers:", len(files))
PY

mkdir -p local-includes

if [[ -e local-includes/boost ]]; then
    # Do not silently overwrite a modified header tree.

    python3 - local-includes/boost "$TEMP/boost" <<'PY'
import hashlib
import sys
from pathlib import Path

def inventory(root):
    return {
        str(p.relative_to(root)):
            hashlib.sha256(p.read_bytes()).hexdigest()
        for p in root.rglob("*")
        if p.is_file()
    }

a, b = map(Path, sys.argv[1:])

assert inventory(a) == inventory(b), \
    "Existing Boost headers differ from the release version."

print("Existing Boost headers verified.")
PY
else
    mv "$TEMP/boost" local-includes/boost
fi

echo
echo "=== 6. Retrieve pinned SDL2 source ==="

SDL_URL="$(cat third-party/sdl2/upstream-url.txt)"

if [[ ! -e "$SDL_SRC" ]]; then
    git clone --no-checkout "$SDL_URL" "$SDL_SRC"

    git -C "$SDL_SRC" checkout --detach "$SDL_COMMIT"

    git -C "$SDL_SRC" apply \
        "$ROOT/third-party/sdl2/cannonball-ps5.patch"
fi

[[ "$(git -C "$SDL_SRC" rev-parse HEAD)" == "$SDL_COMMIT" ]]

# Verify the complete existing SDL2 source against the
# pinned upstream source plus the release patch.

mkdir "$TEMP/sdl-reference"

git -C "$SDL_SRC" archive "$SDL_COMMIT" \
    | tar -xf - -C "$TEMP/sdl-reference"

(
    cd "$TEMP/sdl-reference"

    git apply \
        "$ROOT/third-party/sdl2/cannonball-ps5.patch"
)

python3 - "$SDL_SRC" "$TEMP/sdl-reference" <<'PY'
import subprocess
import sys
from pathlib import Path

actual, reference = map(Path, sys.argv[1:])

tracked = subprocess.check_output([
    "git", "-C", str(actual),
    "ls-files", "-z"
]).decode().split("\0")

names = {n for n in tracked if n}

names.update(
    str(p.relative_to(reference))
    for p in reference.rglob("*")
    if p.is_file()
)

for name in sorted(names):
    a = actual / name
    b = reference / name

    assert a.exists() == b.exists(), name

    if a.is_file():
        assert a.read_bytes() == b.read_bytes(), name

print("Patched SDL2 source verified.")
PY

printf '%s  %s\n' \
    "$VIDEO_SHA" "$SDL_SRC/src/video/ps5/SDL_ps5video.c" \
    | sha256sum -c -

git -C "$SDL_SRC" config core.abbrev 7

echo
echo "=== 7. Configure patched SDL2 ==="

mapfile -t SDL_OPTIONS \
    < third-party/sdl2/cmake-options.txt

cmake \
    -S "$SDL_SRC" \
    -B "$SDL_BUILD" \
    -G "Unix Makefiles" \
    "-DCMAKE_TOOLCHAIN_FILE=$SDK/toolchain/prospero.cmake" \
    "-DCMAKE_C_FLAGS=-I$PORTS/include" \
    "-DCMAKE_EXE_LINKER_FLAGS=-L$PORTS/lib" \
    "${SDL_OPTIONS[@]}"

CONFIG="$SDL_BUILD/include-config-release/SDL2/SDL_config.h"

grep -q '^#define HAVE_ICONV_H 1' "$CONFIG"
grep -q '^#define HAVE_ICONV 1' "$CONFIG"
grep -q '^#define SDL_USE_LIBICONV 1' "$CONFIG"

cmp \
    third-party/sdl2/reference-SDL_config.h \
    "$CONFIG"

cmp \
    third-party/sdl2/reference-SDL_revision.h \
    "$SDL_BUILD/include/SDL2/SDL_revision.h"

echo
echo "=== 8. Compile patched SDL2 ==="

cmake --build "$SDL_BUILD" \
    --target SDL2-static \
    --parallel "$JOBS"

test -s "$SDL_BUILD/libSDL2.a"

if LC_ALL=C grep -aqE \
    'cannonball-sdl-render\.csv|cannonball-sdl-video\.csv|cb_render_record|cb_video_record' \
    "$SDL_BUILD/libSDL2.a"
then
    echo "Unexpected SDL2 diagnostics detected." >&2
    exit 1
fi

mkdir -p local-libs

install -m 644 \
    "$SDL_BUILD/libSDL2.a" \
    local-libs/libSDL2.a

echo
echo "=== 9. Build native runtime ==="

bash tools/rebuild-libc.sh

echo
echo "=== 10. Compile CannonBall ==="

make app

DIST="$ROOT/dist/$TITLE"

test -s "$DIST/eboot.bin"
test -s "$DIST/sce_module/libc.prx"

echo
echo "=== 11. Assemble ROM-free application ==="

mkdir -p "$DIST/res" "$DIST/roms"

install -m 644 \
    launch-data/config.xml \
    "$DIST/config.xml"

cp -a launch-data/res/. "$DIST/res/"

install -m 644 \
    launch-data/roms/README.md \
    "$DIST/roms/README.md"

for FILE in \
    sce_sys/icon0.png \
    sce_sys/pic0.dds \
    sce_sys/pic1.dds \
    sce_sys/param.json
do
    cmp "$FILE" "$DIST/$FILE"
done

python3 - "$DIST" <<'PY'
import sys
import json
from pathlib import Path

root = Path(sys.argv[1])

metadata = json.loads(
    (root / "sce_sys/param.json").read_text()
)

assert metadata["titleId"] == "PPSA98537"

roms = sorted(
    str(p.relative_to(root / "roms"))
    for p in (root / "roms").rglob("*")
    if p.is_file()
)

assert roms == ["README.md"], roms

for p in root.rglob("*"):
    assert not p.is_symlink(), str(p)

    if p.is_file():
        assert not p.name.lower().startswith(
            ("epr-", "mpr-", "opr-")
        ), str(p)

print("ROM-free application verified.")
PY

echo
echo "=== 12. Compare reference checksums ==="

if sha256sum -c \
    third-party/checksums/reference-build.txt
then
    echo "Reference binary comparison: IDENTICAL"
else
    echo "Build completed with binary differences."
    echo "Review toolchain versions before distribution."
fi

echo
echo "=== 13. Generate application checksums ==="

(
    cd "$DIST"

    find . -type f -print0 \
        | sort -z \
        | xargs -0 sha256sum

) > "$ROOT/dist/$TITLE-SHA256SUMS.txt"

echo
echo "========================================"
echo " CANNONBALL PS5 BUILD COMPLETE"
echo "========================================"
echo "Application: $DIST"
echo "ROM files included: NONE"
echo "Application checksums:"
echo "$ROOT/dist/$TITLE-SHA256SUMS.txt"
echo
echo "Supply your own legally obtained OutRun ROMs."
echo "See project documentation for installation."
