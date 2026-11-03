#!/bin/bash
# Builds a complete qimgv-x64 package with the MSYS2 UCRT64 toolchain.
# Result is placed in qimgv/qimgv-x64_<version>
#
# To keep the runtime DLL set small, exiv2, libavif and libheif are built from
# source into local prefixes under _external/ instead of using the msys2
# packages:
#   exiv2   - no curl/webready support (drops the whole curl/openssl chain)
#   libavif - dav1d decoder only (drops aom/rav1e/svt-av1 encoder DLLs)
#   libheif - libde265 decoder only (drops x265/openh264 encoder DLLs)
#
# Caching: external lib prefixes, the qimgv build dir and the image format
# plugins are reused between runs, so repeated builds only recompile what
# changed. CLEAN=1 also wipes the qimgv build dir; FULL=1 additionally wipes
# _external/ (forcing exiv2/libavif/libheif + plugins to rebuild from scratch).
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

if [[ "${FULL:-0}" == "1" ]]; then
    rm -rf "$EXT_DIR"
fi
mkdir -p "$EXT_DIR"

# ------------------------------------------------------------------------------
echo "PREPARING BUILD DIR"
if [[ "${CLEAN:-0}" == "1" || "${FULL:-0}" == "1" ]]; then
    rm -rf "$BUILD_DIR"
fi
mkdir -p "$BUILD_DIR"

# ------------------------------------------------------------------------------
echo "INSTALLING UCRT64 BUILD DEPS"
# extra-cmake-modules provides ECM, required by the jxl and avif image plugins.
# libde265/dav1d are the decoders linked into the source-built libheif/libavif;
# zlib/libiconv/inih are exiv2's remaining dependencies.
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
    mingw-w64-ucrt-x86_64-extra-cmake-modules \
    mingw-w64-ucrt-x86_64-opencv \
    mingw-w64-ucrt-x86_64-libjxl \
    mingw-w64-ucrt-x86_64-libraw \
    mingw-w64-ucrt-x86_64-libde265 \
    mingw-w64-ucrt-x86_64-dav1d \
    mingw-w64-ucrt-x86_64-zlib \
    mingw-w64-ucrt-x86_64-libiconv \
    mingw-w64-ucrt-x86_64-libinih

# Native cmake + pkg-config need Windows-style paths; a few .pc files hardcode
# the POSIX prefix which cmake would otherwise report as a non-existent path.
sed -i "s|/ucrt64/|$PREFIX/|g" "$PREFIX"/lib/pkgconfig/*.pc "$PREFIX"/share/pkgconfig/*.pc 2>/dev/null || true

QMAKE="$(command -v qmake-qt5 || command -v qmake || true)"
if [[ -z "$QMAKE" ]]; then
    echo "ERROR: qmake not found after installing qt5" >&2
    exit 1
fi

# ccache's compiler launcher does not work in this MSYS2/CI setup (ccache
# cannot resolve the Windows-style compiler path from PATH), so builds run
# without it; the prefix/plugin caches below cover the reuse instead.

# ------------------------------------------------------------------------------
echo "BUILDING SUPPORT LIBS (exiv2 / libavif / libheif)"
# Clones one repo at a tag, configures it and installs into
# _external/<name>-prefix. Install prefixes use Windows-style paths because the
# resulting .pc/cmake files are consumed by native cmake and pkgconf.
# Skipped entirely when the prefix already exists with the same tag + options
# (a .built-stamp file records the last build); delete _external/ or set FULL=1
# to force a rebuild.
build_from_git() {
    local name="$1" repo="$2" tag="$3"
    shift 3
    local stamp="$EXT_DIR/$name-prefix/.built-stamp"
    local stamp_value="$tag $*"
    if [[ -f "$stamp" && "$(cat "$stamp")" == "$stamp_value" ]]; then
        echo "--- $name ($tag): cached, skipping"
        return 0
    fi
    echo "--- $name ($tag)"
    rm -rf "$EXT_DIR/$name" "$EXT_DIR/$name-build" "$EXT_DIR/$name-prefix"
    git clone --depth 1 --branch "$tag" "$repo" "$EXT_DIR/$name"
    cmake -S "$EXT_DIR/$name" -B "$EXT_DIR/$name-build" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$(cygpath -m "$EXT_DIR/$name-prefix")" \
        "$@"
    ninja -C "$EXT_DIR/$name-build" install
    printf '%s\n' "$stamp_value" > "$stamp"
}

# exiv2: no network support; enough for reading and writing EXIF tags
build_from_git exiv2 https://github.com/Exiv2/exiv2.git v0.28.3 \
    -DEXIV2_ENABLE_WEBREADY=OFF \
    -DEXIV2_ENABLE_CURL=OFF \
    -DEXIV2_ENABLE_BROTLI=OFF \
    -DEXIV2_ENABLE_XMP=OFF \
    -DEXIV2_ENABLE_VIDEO=OFF \
    -DEXIV2_ENABLE_NLS=OFF \
    -DEXIV2_BUILD_EXIV2_COMMAND=OFF \
    -DEXIV2_BUILD_SAMPLES=OFF \
    -DEXIV2_BUILD_UNIT_TESTS=OFF \
    -DEXIV2_BUILD_FUZZ_TESTS=OFF \
    -DEXIV2_BUILD_DOC=OFF

# libavif: dav1d decode support only, no AV1 encoders
build_from_git libavif https://github.com/AOMediaCodec/libavif.git v1.1.1 \
    -DAVIF_CODEC_DAV1D=ON \
    -DAVIF_CODEC_AOM=OFF \
    -DAVIF_CODEC_RAV1E=OFF \
    -DAVIF_BUILD_APPS=OFF \
    -DAVIF_BUILD_TESTS=OFF \
    -DAVIF_ENABLE_GTEST=OFF

# libheif: libde265 (HEVC) decode support only, no image encoders
build_from_git libheif https://github.com/strukturag/libheif.git v1.19.5 \
    -DWITH_LIBDE265=ON \
    -DWITH_X265=OFF \
    -DWITH_OPENH264=OFF \
    -DWITH_AOM_DECODER=OFF \
    -DWITH_AOM_ENCODER=OFF \
    -DWITH_AOM_ENCODER_PARAMS=OFF \
    -DWITH_DAV1D=OFF \
    -DWITH_RAV1E=OFF \
    -DWITH_SvtEnc=OFF \
    -DWITH_HEADER_COMPRESSION=OFF \
    -DWITH_GDK_PIXBUF=OFF \
    -DWITH_EXAMPLES=OFF \
    -DBUILD_TESTING=OFF

# Route pkg-config lookups to the local prefixes first, so qimgv (exiv2) and
# the heif plugin (libheif) link against the decoder-only builds. pkgconf on
# Windows expects ';' separators and Windows-style paths; its compiled-in
# defaults still cover /ucrt64.
PKG_CONFIG_PATH=""
for p in exiv2 libavif libheif; do
    PKG_CONFIG_PATH+="$(cygpath -m "$EXT_DIR/$p-prefix")/lib/pkgconfig;"
done
export PKG_CONFIG_PATH+="$PREFIX/lib/pkgconfig;$PREFIX/share/pkgconfig"

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
# Each plugin is cloned into _external/<name> and built into _external/<name>/build;
# both steps are skipped when the built plugin DLL is already there.
plugin_cached() {
    find "$EXT_DIR/$1" -name "$2" -print -quit 2>/dev/null | grep -q .
}

if ! ( set +e
    if plugin_cached qt-jpegxl-image-plugin libqjpegxl5.dll; then
        echo "--- qt-jpegxl-image-plugin: cached, skipping"
        exit 0
    fi
    cd "$EXT_DIR"
    [[ -d qt-jpegxl-image-plugin ]] || git clone --depth 1 https://github.com/novomesk/qt-jpegxl-image-plugin.git
    cd qt-jpegxl-image-plugin &&
    cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DQT_MAJOR_VERSION=5 \
        -DCMAKE_PREFIX_PATH="$PREFIX" &&
    ninja -C build
); then
    echo "WARNING: qt-jpegxl-image-plugin failed; JXL support will be missing" >&2
fi

if ! ( set +e
    if plugin_cached qt-avif-image-plugin libqavif5.dll; then
        echo "--- qt-avif-image-plugin: cached, skipping"
        exit 0
    fi
    cd "$EXT_DIR"
    [[ -d qt-avif-image-plugin ]] || git clone --depth 1 https://github.com/novomesk/qt-avif-image-plugin.git
    cd qt-avif-image-plugin &&
    cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DQT_MAJOR_VERSION=5 \
        -DCMAKE_PREFIX_PATH="$(cygpath -m "$EXT_DIR/libavif-prefix");$PREFIX" &&
    ninja -C build
); then
    echo "WARNING: qt-avif-image-plugin failed; AVIF support will be missing" >&2
fi

if ! ( set +e
    if plugin_cached QtApng qapng.dll; then
        echo "--- QtApng: cached, skipping"
        exit 0
    fi
    cd "$EXT_DIR"
    [[ -d QtApng ]] || git clone --depth 1 https://github.com/Skycoder42/QtApng.git
    cd QtApng &&
    mkdir -p build && cd build &&
    "$QMAKE" .. &&
    mingw32-make -j"$(nproc)"
); then
    echo "WARNING: QtApng failed; APNG support will be missing" >&2
fi

if ! ( set +e
    if plugin_cached qt-heif-image-plugin libqheif.dll; then
        echo "--- qt-heif-image-plugin: cached, skipping"
        exit 0
    fi
    cd "$EXT_DIR"
    [[ -d qt-heif-image-plugin ]] || git clone --depth 1 https://github.com/jakar/qt-heif-image-plugin.git
    cd qt-heif-image-plugin &&
    # The plugin relies on the compiler's default search paths, which cover the
    # msys2 prefix but not our source-built libheif; inject it explicitly.
    export CPATH="$(cygpath -m "$EXT_DIR/libheif-prefix/include")${CPATH:+:$CPATH}"
    export LIBRARY_PATH="$(cygpath -m "$EXT_DIR/libheif-prefix/lib")${LIBRARY_PATH:+:$LIBRARY_PATH}"
    cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="$PREFIX" &&
    ninja -C build
); then
    echo "WARNING: qt-heif-image-plugin failed; HEIF support will be missing" >&2
fi

if ! ( set +e
    if plugin_cached qtraw qtraw.dll; then
        echo "--- qtraw: cached, skipping"
        exit 0
    fi
    cd "$EXT_DIR"
    [[ -d qtraw ]] || git clone --depth 1 https://gitlab.com/mardy/qtraw
    cd qtraw &&
    mkdir -p build && cd build &&
    "$QMAKE" .. DEFINES+="LIBRAW_WIN32_CALLS=1" &&
    mingw32-make -j"$(nproc)"
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
mkdir -p "$PACKAGE_DIR/platforms"
# qwindows is the only platform plugin a windowed app needs; qdirect2d,
# qminimal and qoffscreen are debugging/alternate backends.
cp "$QT_PLUGINS/platforms/qwindows.dll" "$PACKAGE_DIR/platforms/"
for d in styles iconengines imageformats printsupport; do
    cp -r "$QT_PLUGINS/$d" "$PACKAGE_DIR/"
done
# qicns (macOS icons), qwbmp and qtga cover formats qimgv never opens
rm -f "$PACKAGE_DIR/imageformats/qicns.dll" \
      "$PACKAGE_DIR/imageformats/qwbmp.dll" \
      "$PACKAGE_DIR/imageformats/qtga.dll"

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

# 5 - runtime DLLs: walk the dependency graph with ldd instead of a fixed list.
# The local prefixes are put on PATH first so the decoder-only libheif/libavif/
# exiv2 DLLs resolve and get collected instead of failing ldd.
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
        done < <(ldd "$f" 2>/dev/null | awk '{print $3}' | grep -E "^(/ucrt64/|$EXT_DIR/)" || true)
    done
}

mapfile -t bins < <(find "$PACKAGE_DIR" -maxdepth 2 -type f \( -name '*.exe' -o -name '*.dll' \))
export PATH="$EXT_DIR/exiv2-prefix/bin:$EXT_DIR/libavif-prefix/bin:$EXT_DIR/libheif-prefix/bin:$PATH"
collect_ucrt_dlls "${bins[@]}"

dll_count=$(find "$PACKAGE_DIR" -maxdepth 1 -name '*.dll' | wc -l)
if (( dll_count < 20 )); then
    echo "ERROR: only $dll_count runtime DLLs collected, packaging is incomplete" >&2
    exit 1
fi

# Guard: the decoder-only rebuilds must keep encoder/network DLLs out of the
# package even when msys2's full-featured libavif/libheif got installed as a
# dependency of something else (e.g. gdk-pixbuf2 pulls libavif).
leaked_dlls="$(find "$PACKAGE_DIR" -maxdepth 2 -type f -iname '*.dll' \
    | grep -iE '(libaom|librav1e|libsvtav1|libx265|libx264|libopenh264|libkvazaar|libcryptopp|libcurl|libssh|libssl|libcrypto|libnghttp|libpsl|libidn)' || true)"
if [[ -n "$leaked_dlls" ]]; then
    echo "ERROR: unwanted encoder/network DLLs leaked into the package:" >&2
    echo "$leaked_dlls" >&2
    exit 1
fi

# Guard: HEIF/AVIF/EXIF support must actually be present, not silently dropped
for required_dll in "libde265*.dll" "libdav1d*.dll" "libheif*.dll" "libavif*.dll" "libexiv2*.dll"; do
    if [[ -z "$(find "$PACKAGE_DIR" -maxdepth 2 -type f -name "$required_dll" -print -quit)" ]]; then
        echo "ERROR: required DLL missing from package: $required_dll" >&2
        exit 1
    fi
done
for required_plugin in imageformats/libqheif.dll imageformats/libqavif5.dll; do
    if [[ ! -f "$PACKAGE_DIR/$required_plugin" ]]; then
        echo "ERROR: required plugin missing from package: $required_plugin" >&2
        exit 1
    fi
done

cd "$SRC_DIR"
echo "PACKAGING DONE ($dll_count runtime dlls)"
