#!/usr/bin/env bash
# Builds the macOS installer package (VST3 + AU + CLAP) from ./bin.
# Signing + notarization run only when these environment variables are set:
#   MACOS_SIGN_APP        "Developer ID Application: Name (TEAMID)"
#   MACOS_SIGN_INSTALLER  "Developer ID Installer: Name (TEAMID)"
#   APPLE_ID, APPLE_TEAM_ID, APPLE_APP_PASSWORD   (for notarytool)
# Usage: installer/macos/build_pkg.sh <version> <docs dir> <output dir>
set -euo pipefail
VERSION="$1"; DOCS="$2"; OUT="$3"
cd "$(dirname "$0")/../.."
ROOT="$PWD"; WORK="$ROOT/out/macpkg"; rm -rf "$WORK"; mkdir -p "$WORK" "$OUT"
ID="com.trapwin.traplimit"

# ---- sign the plugin bundles (hardened runtime is required for notarization)
for b in bin/TrapLimit.vst3 bin/TrapLimit.component bin/TrapLimit.clap; do
  if [ -n "${MACOS_SIGN_APP:-}" ]; then
    codesign --force --deep --timestamp --options runtime --sign "$MACOS_SIGN_APP" "$b"
  else
    codesign --force --deep --sign - "$b"     # ad-hoc (unsigned build, for testing only)
  fi
  codesign --verify --strict --verbose=2 "$b"
done

# ---- one component package per format
mk() { # name bundle install-location
  local name="$1" bundle="$2" loc="$3"
  mkdir -p "$WORK/root-$name"; cp -R "$bundle" "$WORK/root-$name/"
  pkgbuild --root "$WORK/root-$name" --identifier "$ID.$name" --version "$VERSION" \
           --install-location "$loc" "$WORK/$name.pkg"
}
mk vst3 bin/TrapLimit.vst3      "/Library/Audio/Plug-Ins/VST3"
mk au   bin/TrapLimit.component "/Library/Audio/Plug-Ins/Components"
mk clap bin/TrapLimit.clap      "/Library/Audio/Plug-Ins/CLAP"

# ---- distribution with selectable formats, EULA and readme
mkdir -p "$WORK/res"
cp "$DOCS/EULA.txt" "$WORK/res/EULA.txt"
cat > "$WORK/res/README.txt" <<EOF
TrapWin Limiter $VERSION for macOS (Intel + Apple Silicon)

This installer copies the plugin to:
  VST3: /Library/Audio/Plug-Ins/VST3/TrapLimit.vst3
  AU:   /Library/Audio/Plug-Ins/Components/TrapLimit.component  (Logic Pro, GarageBand)
  CLAP: /Library/Audio/Plug-Ins/CLAP/TrapLimit.clap

After installing, restart your DAW (Logic rescans Audio Units automatically).
EOF
cat > "$WORK/distribution.xml" <<EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
  <title>TrapWin Limiter $VERSION</title>
  <license file="EULA.txt"/>
  <readme file="README.txt"/>
  <options customize="allow" require-scripts="false" hostArchitectures="x86_64,arm64"/>
  <domains enable_localSystem="true"/>
  <os-version min="10.15"/>
  <choices-outline>
    <line choice="vst3"/><line choice="au"/><line choice="clap"/>
  </choices-outline>
  <choice id="vst3" title="VST3" description="For Ableton Live, FL Studio, Cubase, Studio One, Reaper and other VST3 hosts."><pkg-ref id="$ID.vst3"/></choice>
  <choice id="au" title="Audio Unit (AU)" description="For Logic Pro, GarageBand and other AU hosts."><pkg-ref id="$ID.au"/></choice>
  <choice id="clap" title="CLAP" description="For Bitwig, Reaper and other CLAP hosts."><pkg-ref id="$ID.clap"/></choice>
  <pkg-ref id="$ID.vst3" version="$VERSION">vst3.pkg</pkg-ref>
  <pkg-ref id="$ID.au" version="$VERSION">au.pkg</pkg-ref>
  <pkg-ref id="$ID.clap" version="$VERSION">clap.pkg</pkg-ref>
</installer-gui-script>
EOF

PKG="$OUT/TrapWin_Limiter_v${VERSION}_macOS.pkg"
SIGNARGS=()
[ -n "${MACOS_SIGN_INSTALLER:-}" ] && SIGNARGS=(--sign "$MACOS_SIGN_INSTALLER" --timestamp)
productbuild --distribution "$WORK/distribution.xml" --resources "$WORK/res" --package-path "$WORK" ${SIGNARGS[@]+"${SIGNARGS[@]}"} "$PKG"

# ---- notarize + staple
if [ -n "${MACOS_SIGN_INSTALLER:-}" ] && [ -n "${APPLE_ID:-}" ] && [ -n "${APPLE_APP_PASSWORD:-}" ] && [ -n "${APPLE_TEAM_ID:-}" ]; then
  xcrun notarytool submit "$PKG" --apple-id "$APPLE_ID" --team-id "$APPLE_TEAM_ID" --password "$APPLE_APP_PASSWORD" --wait
  xcrun stapler staple "$PKG"
  spctl --assess --type install --verbose "$PKG"
  echo "NOTARIZED=yes"
else
  echo "NOTARIZED=no  (no Apple certificates configured - this package is for testing only)"
fi
ls -la "$PKG"
