#!/bin/bash
# Builds the BIT ERA macOS installer: VST3 + AU into /Library/Audio/Plug-Ins, Standalone into /Applications.
# Signs + notarizes with the same secrets / helpers as EVOLVE (installer/mac/setup_signing.sh, notarize.sh).
# Usage: installer/mac/build_pkg.sh <version>
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
source "$ROOT/installer/mac/notarize.sh"
trap 'echo "::error title=$(basename "$0")::line $LINENO failed: $BASH_COMMAND"' ERR

VERSION="${1:-0.1.0}"
ART="$ROOT/build-bitera/BitEra_artefacts/Release"
STAGE="$ROOT/build-bitera/pkgroot"
OUT="$ROOT/build-bitera/installer"

rm -rf "$STAGE" && mkdir -p "$STAGE/Library/Audio/Plug-Ins/VST3" "$STAGE/Library/Audio/Plug-Ins/Components" "$STAGE/Applications" "$OUT"
cp -R "$ART/VST3/Bit Era.vst3" "$STAGE/Library/Audio/Plug-Ins/VST3/"
cp -R "$ART/AU/Bit Era.component" "$STAGE/Library/Audio/Plug-Ins/Components/"
cp -R "$ART/Standalone/Bit Era.app" "$STAGE/Applications/"

if [ -n "${MAC_APP_SIGN_ID:-}" ]; then
    find "$STAGE" -maxdepth 6 \( -name "*.vst3" -o -name "*.component" -o -name "*.app" \) -type d -prune -print0 \
        | while IFS= read -r -d '' b; do
            codesign --force --deep --options runtime --timestamp --sign "$MAC_APP_SIGN_ID" "$b"
            codesign --verify --strict --verbose=2 "$b"
        done
fi

# plain zip of the (signed) bundles for manual installs
( cd "$STAGE" && ditto -c -k --keepParent "Library/Audio/Plug-Ins/VST3/Bit Era.vst3" "$OUT/BIT-ERA-VST3-Mac.zip" \
  && ditto -c -k --keepParent "Library/Audio/Plug-Ins/Components/Bit Era.component" "$OUT/BIT-ERA-AU-Mac.zip" )

pkgbuild --root "$STAGE" --identifier com.trapwin.bitera --version "$VERSION" --install-location / "$OUT/bitera-component.pkg"

cat > "$OUT/distribution.xml" <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
    <title>Bit Era by TrapWin $VERSION</title>
    <license file="EULA.txt"/>
    <options customize="never" require-scripts="false" hostArchitectures="arm64,x86_64"/>
    <choices-outline><line choice="default"/></choices-outline>
    <choice id="default"><pkg-ref id="com.trapwin.bitera"/></choice>
    <pkg-ref id="com.trapwin.bitera" version="$VERSION">bitera-component.pkg</pkg-ref>
</installer-gui-script>
XML

mkdir -p "$OUT/resources" && cp "$ROOT/installer/EULA.txt" "$OUT/resources/"
PKG="$OUT/BIT-ERA-$VERSION-macOS.pkg"
if [ -n "${MAC_INST_SIGN_ID:-}" ]; then
    productbuild --distribution "$OUT/distribution.xml" --resources "$OUT/resources" --package-path "$OUT" --sign "$MAC_INST_SIGN_ID" "$PKG"
else
    productbuild --distribution "$OUT/distribution.xml" --resources "$OUT/resources" --package-path "$OUT" "$PKG"
fi
rm -f "$OUT/bitera-component.pkg"

if [ -n "${MAC_INST_SIGN_ID:-}" ] && { [ -n "${NOTARY_KEY_PATH:-}" ] || [ -n "${NOTARY_APPLE_ID:-}" ]; }; then
    notarize "$PKG"
    xcrun stapler staple "$PKG"
fi
echo "Created $PKG"
