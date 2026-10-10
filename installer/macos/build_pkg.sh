#!/usr/bin/env bash
# Builds the macOS installer (.pkg with VST3 + AU) and the signed "Manual Install" bundles from ./bin.
# Signing + notarization run only when these environment variables are set (see .github/workflows/build.yml):
#   MACOS_SIGN_APP        "Developer ID Application: Name (TEAMID)"   (identity in the keychain)
#   MACOS_SIGN_INSTALLER  "Developer ID Installer: Name (TEAMID)"
#   notarytool, either App Store Connect API key (preferred, same as trapwin-tw1):
#     APPLE_API_KEY_P8 (base64 of the .p8 file), APPLE_API_KEY_ID, APPLE_API_ISSUER_ID
#   or Apple ID: APPLE_ID, APPLE_TEAM_ID, APPLE_APP_PASSWORD
# Usage: installer/macos/build_pkg.sh <version> <docs dir> <output dir>
# Output: <output dir>/TrapWin_Limiter_v<version>_macOS.pkg, <output dir>/manual/{TrapLimit.vst3,TrapLimit.component}
#         and SIGNED=yes|no written to <output dir>/signed.txt
set -euo pipefail
trap 'echo "::error title=build_pkg.sh::line $LINENO failed: $BASH_COMMAND"' ERR
VERSION="$1"; DOCS="$2"; OUT="$3"
cd "$(dirname "$0")/../.."
ROOT="$PWD"; WORK="$ROOT/out/macpkg"; rm -rf "$WORK"; mkdir -p "$WORK" "$OUT"
ID="com.trapwin.traplimit"
CAN_NOTARIZE=no
NOTARY=()
if [ -n "${MACOS_SIGN_APP:-}" ] && [ -n "${MACOS_SIGN_INSTALLER:-}" ]; then
  if [ -n "${APPLE_API_KEY_P8:-}" ] && [ -n "${APPLE_API_KEY_ID:-}" ] && [ -n "${APPLE_API_ISSUER_ID:-}" ]; then
    KEYFILE="$WORK/AuthKey_${APPLE_API_KEY_ID}.p8"
    if printf '%s' "$APPLE_API_KEY_P8" | grep -q "BEGIN PRIVATE KEY"; then printf '%s\n' "$APPLE_API_KEY_P8" > "$KEYFILE"
    else printf '%s' "$APPLE_API_KEY_P8" | base64 --decode > "$KEYFILE"; fi
    NOTARY=(--key "$KEYFILE" --key-id "$APPLE_API_KEY_ID" --issuer "$APPLE_API_ISSUER_ID")
    CAN_NOTARIZE=yes
  elif [ -n "${APPLE_ID:-}" ] && [ -n "${APPLE_APP_PASSWORD:-}" ] && [ -n "${APPLE_TEAM_ID:-}" ]; then
    NOTARY=(--apple-id "$APPLE_ID" --team-id "$APPLE_TEAM_ID" --password "$APPLE_APP_PASSWORD")
    CAN_NOTARIZE=yes
  fi
fi
notarize() { xcrun notarytool submit "$1" "${NOTARY[@]}" --wait --timeout 30m --output-format json | tee "$WORK/notary.json"; grep -q '"status" *: *"Accepted"' "$WORK/notary.json" || { echo "::error title=notarization::$(cat "$WORK/notary.json")"; id=$(sed -nE 's/.*"id" *: *"([^"]+)".*/\1/p' "$WORK/notary.json" | head -1); [ -n "$id" ] && xcrun notarytool log "$id" "${NOTARY[@]}" || true; exit 1; }; }

# ---- sign the plugin bundles (hardened runtime + timestamp are required for notarization)
BUNDLES=(bin/TrapLimit.vst3 bin/TrapLimit.component)
for b in "${BUNDLES[@]}"; do
  if [ -n "${MACOS_SIGN_APP:-}" ]; then
    codesign --force --deep --timestamp --options runtime --sign "$MACOS_SIGN_APP" "$b"
  else
    codesign --force --deep --sign - "$b"     # ad-hoc: unsigned build, for testing only
  fi
  codesign --verify --strict --verbose=2 "$b"
done

# ---- notarize the bundles themselves too, so the "Manual Install" copies pass Gatekeeper
if [ "$CAN_NOTARIZE" = yes ]; then
  ditto -c -k --keepParent bin/TrapLimit.vst3 "$WORK/vst3.zip"
  ditto -c -k --keepParent bin/TrapLimit.component "$WORK/au.zip"
  for z in vst3 au; do
    notarize "$WORK/$z.zip"
  done
  for b in "${BUNDLES[@]}"; do xcrun stapler staple "$b"; done
fi
mkdir -p "$OUT/manual"
for b in "${BUNDLES[@]}"; do ditto "$b" "$OUT/manual/$(basename "$b")"; done

# ---- component packages
mk() { # name bundle install-location
  local name="$1" bundle="$2" loc="$3"
  mkdir -p "$WORK/root-$name"; ditto "$bundle" "$WORK/root-$name/$(basename "$bundle")"
  pkgbuild --root "$WORK/root-$name" --identifier "$ID.$name" --version "$VERSION" \
           --install-location "$loc" "$WORK/$name.pkg"
}
mk vst3 bin/TrapLimit.vst3      "/Library/Audio/Plug-Ins/VST3"
mk au   bin/TrapLimit.component "/Library/Audio/Plug-Ins/Components"

# ---- distribution: selectable formats, EULA, readme
mkdir -p "$WORK/res"
cp "$DOCS/EULA.txt" "$WORK/res/EULA.txt"
cat > "$WORK/res/README.txt" <<EOF
TrapWin Limiter $VERSION for macOS (Intel + Apple Silicon)

This installer copies the plugin to:
  VST3:            /Library/Audio/Plug-Ins/VST3/TrapLimit.vst3
  Audio Unit (AU): /Library/Audio/Plug-Ins/Components/TrapLimit.component  (Logic Pro, GarageBand)

After installing, restart your DAW (Logic Pro rescans Audio Units automatically).
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
    <line choice="vst3"/><line choice="au"/>
  </choices-outline>
  <choice id="vst3" title="VST3" description="For Ableton Live, FL Studio, Cubase, Studio One, Reaper and other VST3 hosts."><pkg-ref id="$ID.vst3"/></choice>
  <choice id="au" title="Audio Unit (AU)" description="For Logic Pro, GarageBand and other Audio Unit hosts."><pkg-ref id="$ID.au"/></choice>
  <pkg-ref id="$ID.vst3" version="$VERSION">vst3.pkg</pkg-ref>
  <pkg-ref id="$ID.au" version="$VERSION">au.pkg</pkg-ref>
</installer-gui-script>
EOF

PKG="$OUT/TrapWin_Limiter_v${VERSION}_macOS.pkg"
SIGNARGS=()
[ -n "${MACOS_SIGN_INSTALLER:-}" ] && SIGNARGS=(--sign "$MACOS_SIGN_INSTALLER" --timestamp)
productbuild --distribution "$WORK/distribution.xml" --resources "$WORK/res" --package-path "$WORK" ${SIGNARGS[@]+"${SIGNARGS[@]}"} "$PKG"

# ---- notarize + staple the installer
if [ "$CAN_NOTARIZE" = yes ]; then
  notarize "$PKG"
  xcrun stapler staple "$PKG"
  spctl --assess --type install --verbose "$PKG"
  for b in "$OUT"/manual/*; do spctl --assess --type execute --verbose "$b" || spctl -a -vvv -t install "$b" || true; done
  echo "yes" > "$OUT/signed.txt"
  echo "::notice title=macOS signing::signed and notarized"
else
  echo "no" > "$OUT/signed.txt"
  echo "::warning title=macOS signing::NOT signed/notarized - add the Apple secrets (MAC_CERT_APP_P12, MAC_CERT_INSTALLER_P12, MAC_CERT_PASSWORD, APPLE_API_KEY_P8, APPLE_API_KEY_ID, APPLE_API_ISSUER_ID, APPLE_TEAM_ID) before selling"
fi
ls -la "$OUT" "$OUT/manual"
