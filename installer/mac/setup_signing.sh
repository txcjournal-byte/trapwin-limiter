#!/bin/bash
# CI helper (GitHub Actions, macOS): import the Developer ID certificates into a temporary keychain and
# expose what build_pkg.sh / build_vault_pkg.sh need through $GITHUB_ENV.
# Secrets (repository secrets, same names as the owner's prepared files):
#   MAC_CERT_APP_P12        base64 of the "Developer ID Application" .p12
#   MAC_CERT_INSTALLER_P12  base64 of the "Developer ID Installer" .p12
#   MAC_CERT_PASSWORD       password of both .p12 files
#   APPLE_API_KEY_P8        App Store Connect API key (.p8 text, or base64 of it)   - notarization
#   APPLE_API_KEY_ID, APPLE_API_ISSUER_ID, APPLE_TEAM_ID
# Without the certificate secrets the script does nothing and the builds stay unsigned.
set -euo pipefail
ENVF="${GITHUB_ENV:-/dev/null}"
TMP="${RUNNER_TEMP:-/tmp}"

if [ -z "${MAC_CERT_APP_P12:-}" ] || [ -z "${MAC_CERT_PASSWORD:-}" ]; then
    echo "::notice title=Mac signing::MAC_CERT_APP_P12 / MAC_CERT_PASSWORD not set - pkgs and plugins stay unsigned"
    exit 0
fi

KC="$TMP/trapwin-signing.keychain-db"
KCPW="$(uuidgen)"
security create-keychain -p "$KCPW" "$KC"
security set-keychain-settings -lut 21600 "$KC"
security unlock-keychain -p "$KCPW" "$KC"

decode() { printf '%s' "$1" | tr -d '\r\n\t ' | base64 --decode; }
for var in MAC_CERT_APP_P12 MAC_CERT_INSTALLER_P12; do
    val="${!var:-}"
    [ -n "$val" ] || { echo "::warning title=Mac signing::$var not set"; continue; }
    f="$TMP/$var.p12"
    decode "$val" > "$f"
    security import "$f" -k "$KC" -P "$MAC_CERT_PASSWORD" -f pkcs12 \
        -T /usr/bin/codesign -T /usr/bin/productbuild -T /usr/bin/productsign -T /usr/bin/pkgbuild
    rm -f "$f"
done
security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k "$KCPW" "$KC" > /dev/null
# put the temporary keychain first in the search list (keep the existing ones)
security list-keychains -d user -s "$KC" $(security list-keychains -d user | tr -d '"')

APP_ID=$(security find-identity -v -p codesigning "$KC" | sed -n 's/.*"\(Developer ID Application: [^"]*\)".*/\1/p' | head -n 1)
INST_ID=$(security find-identity -v -p basic "$KC" | sed -n 's/.*"\(Developer ID Installer: [^"]*\)".*/\1/p' | head -n 1)
echo "Application identity: ${APP_ID:-<none>}"
echo "Installer identity:   ${INST_ID:-<none>}"
[ -n "$APP_ID" ] && echo "MAC_APP_SIGN_ID=$APP_ID" >> "$ENVF"
[ -n "$INST_ID" ] && echo "MAC_INST_SIGN_ID=$INST_ID" >> "$ENVF"
echo "MAC_SIGN_KEYCHAIN=$KC" >> "$ENVF"

if [ -n "${APPLE_API_KEY_P8:-}" ] && [ -n "${APPLE_API_KEY_ID:-}" ] && [ -n "${APPLE_API_ISSUER_ID:-}" ]; then
    P8="$TMP/AuthKey_${APPLE_API_KEY_ID}.p8"
    if printf '%s' "$APPLE_API_KEY_P8" | grep -q "BEGIN PRIVATE KEY"; then
        printf '%s\n' "$APPLE_API_KEY_P8" > "$P8"
    else
        decode "$APPLE_API_KEY_P8" > "$P8"
    fi
    chmod 600 "$P8"
    { echo "NOTARY_KEY_PATH=$P8"; echo "NOTARY_KEY_ID=$APPLE_API_KEY_ID"; echo "NOTARY_ISSUER=$APPLE_API_ISSUER_ID"; } >> "$ENVF"
    echo "Notarization: App Store Connect API key $APPLE_API_KEY_ID"
else
    echo "::notice title=Mac signing::APPLE_API_KEY_P8 / _ID / _ISSUER_ID not set - signed but not notarized"
fi
