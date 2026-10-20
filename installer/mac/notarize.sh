#!/bin/bash
# Sourced by build_pkg.sh / build_vault_pkg.sh. notarize <file>: submits to Apple's notary service and waits.
# On anything but "Accepted" it prints Apple's issue list as a GitHub annotation (readable without the raw log).
notary_auth() {
    if [ -n "${NOTARY_KEY_PATH:-}" ]; then
        NOTARY_ARGS=(--key "$NOTARY_KEY_PATH" --key-id "$NOTARY_KEY_ID" --issuer "$NOTARY_ISSUER")
    elif [ -n "${NOTARY_APPLE_ID:-}" ]; then
        NOTARY_ARGS=(--apple-id "$NOTARY_APPLE_ID" --team-id "$NOTARY_TEAM_ID" --password "$NOTARY_PASSWORD")
    else
        return 1
    fi
}
notarize() {
    local f="$1" out id status
    notary_auth || return 2
    out=$(xcrun notarytool submit "$f" "${NOTARY_ARGS[@]}" --wait --timeout 60m --output-format json 2>&1) || true
    echo "$out"
    id=$(printf '%s' "$out" | python3 -c 'import json,sys
try: print(json.loads(sys.stdin.read()).get("id",""))
except Exception: print("")')
    status=$(printf '%s' "$out" | python3 -c 'import json,sys
try: print(json.loads(sys.stdin.read()).get("status",""))
except Exception: print("")')
    if [ "$status" = "Accepted" ]; then return 0; fi
    local detail="$out"
    if [ -n "$id" ]; then
        detail=$(xcrun notarytool log "$id" "${NOTARY_ARGS[@]}" 2>&1 | python3 -c 'import json,sys
t=sys.stdin.read()
try:
    j=json.loads(t); iss=j.get("issues") or []
    print("; ".join("%s: %s (%s)" % (i.get("severity"), i.get("message"), i.get("path")) for i in iss[:12]) or t[:1500])
except Exception: print(t[:1500])')
    fi
    detail=$(printf '%s' "$detail" | tr '\n' ' ' | cut -c1-3000)
    echo "::error title=Notarization ${status:-failed} ($(basename "$f"))::$detail"
    return 1
}
