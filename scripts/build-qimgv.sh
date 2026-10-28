#!/bin/bash
# Builds a complete qimgv-x64 package with the MSYS2 UCRT64 toolchain.
# Result is placed in qimgv/qimgv-x64_<version>
set -euo pipefail

CFL='-ffunction-sections -fdata-sections -O3 -pipe'
LDFL='-Wl,--gc-sections'

if [[ "${MSYSTEM:-}" != "UCRT64" ]]; then
    echo "ERROR: run this script from an MSYS2 UCRT64 shell (MSYSTEM=UCRT64)." >&2
    exit 1
fi

PREFIX="$(cygpath -m /ucrt64)"
SCRIPTS_DIR="$(dirname "$(readlink -f "$0")")"
SRC_DIR="$(dirname "$SCRIPTS_DIR")"
BUILD_DIR="$SRC_DIR/build"
EXT_DIR="$SRC_DIR/_external"

rm -rf "$EXT_DIR"
mkdir -p "$EXT_DIR"

# ------------------------------------------------------------------------------
echo "PREPARING BUILD DIR"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

# ------------------------------------------------------------------------------
echo "INSTALLING UCRT64 BUILD DEPS"
pacman -S --noconfirm --needed \
    git p7zip mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja \
    mingw-w64-ucrt-x86_64-gcc \
    mingw-w64-ucrt-x86_64-make \
    mingw-w64-ucrt-x86_64-pkgconf \
    mingw-w64-ucrt-x86_64-qt5-base \
    mingw-w64-ucrt-x86_64-qt5-svg \
    mingw-w64-ucrt-x86_64-qt5-tools \
    mingw-w64-ucrt-x86_64-qt5-translations \
    mingw-w64-ucrt-x86_64-qt5-imageformats \
    mingw-w64-ucrt-x86_64-exiv2 \
    mingw-w64-ucrt-x86_64-opencv \
    mingw-w64-ucrt-x86_64-libjxl \
    mingw-w64-ucrt-x86_64-libavif \
    mingw-w64-ucrt-x86_64-libheif \
    mingw-w64-ucrt-x86_64-libraw

# Native cmake + pkg-config need Windows-style paths; a few .pc files hardcode
# the POSIX prefix which cmake would otherwise report as a non-existent path.
sed -i "s|/ucrt64/|$PREFIX/|g" "$PREFIX"/lib/pkgconfig/*.pc "$PREFIX"/share/pkgconfig/*.pc 2>/dev/null || true

QMAKE="$(command -v qmake-qt5 || command -v qmake || true)"
if [[ -z "$QMAKE" ]]; then
    echo "ERROR: qmake not found after installing qt5" >&2
    exit 1
fi

# ------------------------------------------------------------------------------
echo "BUILDING"
cmake -S "$SRC_DIR" -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$PREFIX" \
    -DOPENCV_SUPPORT=ON \
    -DCMAKE_CXX_FLAGS="$CFL" \
    -DCMAKE_EXE_LINKER_FLAGS="$LDFL"
ninja -C "$BUILD_DIR"

# ------------------------------------------------------------------------------
echo "BUILDING IMAGEFORMATS"
# Optional plugins: a failure here must not hide the main build result,
# the corresponding format simply stays unsupported.
if ! ( set +e
    cd "$EXT_DIR"
    git clone --depth 1 https://github.com/novomesk/qt-jpegxl-image-plugin.git &&
    cd qt-jpegxl-image-plugin &&
    cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DQT_MAJOR_VERSION=5 \
        -DCMAKE_PREFIX_PATH="$PREFIX" \
    ninja -C build
); then
    echo "WARNING: qt-jpegxl-image-plugin failed; JXL support will be missing" >&2
fi

if ! ( set +e
    cd "$EXT_DIR"
    git clone --depth 1 https://github.com/novomesk/qt-avif-image-plugin.git &&
    cd qt-avif-image-plugin &&
    cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DQT_MAJOR_VERSION=5 \
        -DCMAKE_PREFIX_PATH="$PREFIX" \
    ninja -C build
); then
    echo "WARNING: qt-avif-image-plugin failed; AVIF support will be missing" >&2
fi

if ! ( set +e
    cd "$EXT_DIR"
    git clone --depth 1 https://github.com/Skycoder42/QtApng.git &&
    cd QtApng &&
    mkdir -p build && cd build &&
    "$QMAKE" .. &&
    mingw32-make -j4
); then
    echo "WARNING: QtApng failed; APNG support will be missing" >&2
fi

if ! ( set +e
    cd "$EXT_DIR"
    git clone --depth 1 https://github.com/jakar/qt-heif-image-plugin.git &&
    cd qt-heif-image-plugin &&
    cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="$PREFIX" \
    ninja -C build
); then
    echo "WARNING: qt-heif-image-plugin failed; HEIF support will be missing" >&2
fi

if ! ( set +e
    cd "$EXT_DIR"
    git clone --depth 1 https://gitlab.com/mardy/qtraw &&
    cd qtraw &&
    mkdir -p build && cd build &&
    "$QMAKE" .. DEFINES+="LIBRAW_WIN32_CALLS=1" &&
    mingw32-make -j4
); then
    echo "WARNING: qtraw failed; RAW support will be missing" >&2
fi

# ------------------------------------------------------------------------------
echo "PACKAGING"
cd "$SRC_DIR"
BUILD_NAME="qimgv-x64_$(git describe --tags --always)"
PACKAGE_DIR="$SRC_DIR/$BUILD_NAME"
rm -rf "$PACKAGE_DIR"
mkdir -p "$PACKAGE_DIR"

test -f "$BUILD_DIR/qimgv/qimgv.exe" || { echo "ERROR: qimgv.exe was not built" >&2; exit 1; }

# 1 - qimgv itself
cp "$BUILD_DIR/qimgv/qimgv.exe" "$PACKAGE_DIR/"
cp -r "$BUILD_DIR/qimgv/translations" "$PACKAGE_DIR/"

# 2 - Qt plugins + translations
QT_PLUGINS="$PREFIX/share/qt5/plugins"
for d in platforms styles iconengines imageformats printsupport; do
    cp -r "$QT_PLUGINS/$d" "$PACKAGE_DIR/"
done
cp "$PREFIX"/share/qt5/translations/qtbase_*.qm "$PACKAGE_DIR/translations/" 2>/dev/null || true

# 3 - extra image format plugins
copy_plugin() {
    local dll_name="$1"
    local search_dir="$2"
    local found
    found="$(find "$search_dir" -name "$dll_name" -print -quit 2>/dev/null || true)"
    if [[ -n "$found" ]]; then
        cp "$found" "$PACKAGE_DIR/imageformats/"
        echo "[ok] $dll_name"
    else
        echo "WARNING: $dll_name not found in $search_dir" >&2
    fi
}
copy_plugin libqjpegxl5.dll "$EXT_DIR/qt-jpegxl-image-plugin"
copy_plugin libqavif5.dll "$EXT_DIR/qt-avif-image-plugin"
copy_plugin qapng.dll "$EXT_DIR/QtApng"
copy_plugin libqheif.dll "$EXT_DIR/qt-heif-image-plugin"
copy_plugin qtraw.dll "$EXT_DIR/qtraw"

# 4 - misc
mkdir -p "$PACKAGE_DIR/cache" "$PACKAGE_DIR/conf" "$PACKAGE_DIR/thumbnails"
cp -r "$SRC_DIR/qimgv/distrib/mimedata/data" "$PACKAGE_DIR/"

# 5 - runtime DLLs: walk the dependency graph with ldd instead of a fixed list
collect_ucrt_dlls() {
    local queue=("$@")
    local seen=" "
    while (( ${#queue[@]} )); do
        local f="${queue[0]}"
        queue=("${queue[@]:1}")
        local dep base
        while IFS= read -r dep; do
            [[ -n "$dep" ]] || continue
            base="$(basename "$dep")"
            case "$seen" in
                *" $base "*) continue ;;
            esac
            seen="$seen$base "
            cp -f "$dep" "$PACKAGE_DIR/"
            queue+=("$dep")
        done < <(ldd "$f" 2>/dev/null | awk '{print $3}' | grep '^/ucrt64/' || true)
    done
}

mapfile -t bins < <(find "$PACKAGE_DIR" -maxdepth 2 -type f \( -name '*.exe' -o -name '*.dll' \))
collect_ucrt_dlls "${bins[@]}"

dll_count=$(find "$PACKAGE_DIR" -maxdepth 1 -name '*.dll' | wc -l)
if (( dll_count < 20 )); then
    echo "ERROR: only $dll_count runtime DLLs collected, packaging is incomplete" >&2
    exit 1
fi

cd "$SRC_DIR"
echo "PACKAGING DONE ($dll_count runtime dlls)"
