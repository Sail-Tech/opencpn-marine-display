#!/usr/bin/env bash
# Local macOS plugin package (.tar.gz) for OpenCPN's plugin manager (Options > Plugins > Import plugin...).
#
# Builds a universal library with build-macos-local.sh and packs it in the layout of the template's
# darwin-wx32 tarballs: <name>/metadata.xml and <name>/OpenCPN.app/Contents/{PlugIns,SharedSupport}.
#
# Usage:
#   WXDEPS=/path/to/macos_deps_universal [DIST=folder] ci/package-macos-local.sh [--install]
#
# --install does what Import plugin does: copies the files into ~/Library/Application Support/OpenCPN
# and records them in OpenCPN's install data, so the plugin manager lists it and can uninstall it.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${OUT:-$ROOT/build-local}"
DIST="${DIST:-$OUT}"  # Where the package goes.
ARCHS="${ARCHS:-x86_64 arm64}"
PKG=marine_display_pi
NAME=MarineDisplay

VERSION="$(sed -n 's/^set(PKG_VERSION \([0-9.]*\)).*/\1/p' "$ROOT/Plugin.cmake")"
SUMMARY="$(sed -n 's/^set(PKG_SUMMARY "\(.*\)").*/\1/p' "$ROOT/Plugin.cmake")"
DESCRIPTION="$(sed -n 's/^set(PKG_DESCRIPTION \[=\[\(.*\)\]=\]).*/\1/p' "$ROOT/Plugin.cmake")"
AUTHOR="$(sed -n 's/^set(PKG_AUTHOR "\(.*\)").*/\1/p' "$ROOT/Plugin.cmake")"
HOMEPAGE="$(sed -n 's/^set(PKG_HOMEPAGE "\(.*\)").*/\1/p' "$ROOT/Plugin.cmake")"
TARGET_ARCH="$(echo "$ARCHS" | tr ' ' '\n' | sort | paste -sd ';' -)"
BASE="$NAME-${VERSION}_darwin-wx32-10-$(echo "$TARGET_ARCH" | tr ';' '-')"

if [ "${1:-}" = "--install" ] && pgrep -x OpenCPN >/dev/null; then
  echo "OpenCPN is running: quit it first. Overwriting a loaded plugin makes macOS kill OpenCPN." >&2
  exit 1
fi

OUT="$OUT" ARCHS="$ARCHS" "$ROOT/ci/build-macos-local.sh"

metadata() {  # $1: tarball URL, $2: checksum
  cat <<XML
<?xml version="1.0" encoding="UTF-8"?>
<plugin version="1">
  <name> $NAME </name>
  <version> $VERSION </version>
  <release> 0 </release>
  <summary> $SUMMARY </summary>
  <api-version> 1.18 </api-version>
  <open-source> yes </open-source>
  <author> $AUTHOR </author>
  <source> $HOMEPAGE </source>
  <info-url> $HOMEPAGE </info-url>
  <description> $DESCRIPTION </description>
  <target>darwin-wx32</target>
  <target-version>10</target-version>
  <target-arch>$TARGET_ARCH</target-arch>
  <tarball-url> $1 </tarball-url>
  <tarball-checksum> $2 </tarball-checksum>
</plugin>
XML
}

STAGE="$OUT/package"
find "$STAGE" -mindepth 1 -delete 2>/dev/null || true
APP="$STAGE/$BASE/OpenCPN.app/Contents"
mkdir -p "$APP/PlugIns" "$APP/SharedSupport/plugins/$PKG/data"
cp "$OUT/lib$PKG.dylib" "$APP/PlugIns/"
cp "$ROOT"/data/*.svg "$APP/SharedSupport/plugins/$PKG/data/"
metadata "file://$DIST/$BASE.tar.gz" "" >"$STAGE/$BASE/metadata.xml"
mkdir -p "$DIST"
COPYFILE_DISABLE=1 tar -czf "$DIST/$BASE.tar.gz" -C "$STAGE" "$BASE"
SUM="$(shasum -a 256 "$DIST/$BASE.tar.gz" | cut -d' ' -f1)"
echo "$SUM  $BASE.tar.gz" >"$DIST/$BASE.tar.gz.sha256"
metadata "file://$DIST/$BASE.tar.gz" "$SUM" >"$DIST/$BASE.xml"
echo "Packaged $DIST/$BASE.tar.gz"

if [ "${1:-}" = "--install" ]; then
  DEST="$HOME/Library/Application Support/OpenCPN"
  DATA="$HOME/Library/Preferences/opencpn/plugins/install_data"
  mkdir -p "$DATA/imports"
  LIST="$(mktemp)"
  (cd "$STAGE/$BASE/OpenCPN.app" && find Contents -mindepth 1) | sort | while read -r rel; do
    src="$STAGE/$BASE/OpenCPN.app/$rel"
    if [ -d "$src" ]; then
      mkdir -p "$DEST/$rel"
      echo "$DEST/$rel/"
    else
      cp "$src" "$DEST/$rel.new"
      mv -f "$DEST/$rel.new" "$DEST/$rel"
      echo "$DEST/$rel"
    fi
  done >"$LIST"
  lower="$(echo "$NAME" | tr '[:upper:]' '[:lower:]')"
  mv "$LIST" "$DATA/$lower.files"
  echo "$VERSION" >"$DATA/$lower.version"
  printf 'share: %s\nlib64: %s\nlib: %s\nbin: %s\n' "$DEST/Contents" "$DEST/Contents/PlugIns" \
    "$DEST/Contents/PlugIns" "$DEST/Contents/PlugIns" >"$DATA/$lower.dirs"
  cp "$DIST/$BASE.xml" "$DATA/imports/$lower.xml"
  echo "Installed $NAME $VERSION into $DEST (start OpenCPN)"
fi
