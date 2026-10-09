# TrapWin Limiter

TW-2 clipper a true-peak limiter pro trap a underground (VST3 / AU / CLAP, Windows + macOS).
Postaveno na [DPF](https://github.com/DISTRHO/DPF) (ve složce `dpf/`).

> Soukromý repozitář – zdrojový kód placeného produktu, nezveřejňovat.

## Jak se staví nová verze

Všechno dělá GitHub sám (`.github/workflows/build.yml`):

| Kdy | Co se stane |
|---|---|
| každý push do `main` | otestuje zvuk, sestaví Windows i Mac verzi, ověří plugin (pluginval, auval) a hotové soubory nechá ke stažení v záložce **Actions** → poslední běh → **Artifacts** |
| **Actions → Build TrapWin Limiter → Run workflow**, vyplníš verzi (např. `2.5`) | to samé + vytvoří **Release** s hotovými soubory pro zákazníky (stejně funguje i push tagu `v2.5`) |

Soubory pro zákazníky (Release):
- `TrapWin_Limiter_v2.5_Windows_Setup.exe` – instalátor pro Windows
- `TrapWin_Limiter_v2.5_Windows.zip` – ruční instalace (přetáhnout do složky VST3)
- `TrapWin_Limiter_v2.5_macOS.pkg` – instalátor pro Mac (Intel + Apple Silicon; VST3, AU, CLAP)
- `TrapWin Limiter - Manual.pdf`

Nová verze: změň číslo v `plugin/TrapLimitPlugin.cpp` (`d_version`) a nápis `"v2.5"` v
`plugin/TrapWinBetonUI.cpp`, pak spusť **Run workflow** s verzí `2.6`.

## Mac: podpis a notarizace (nutné pro prodej)

Bez toho macOS plugin zákazníkům zablokuje. Potřebuješ účet **Apple Developer Program** (99 USD/rok).
V Apple Developer účtu vytvoř certifikáty **Developer ID Application** a **Developer ID Installer**,
exportuj je z Klíčenky jako `.p12` a v GitHubu (Settings → Secrets and variables → Actions) přidej:

| Secret | Obsah |
|---|---|
| `MACOS_CERT_APP_P12` | Developer ID Application `.p12` převedený do base64 (`base64 -i app.p12 \| pbcopy`) |
| `MACOS_CERT_INSTALLER_P12` | Developer ID Installer `.p12` v base64 |
| `MACOS_CERT_PASSWORD` | heslo, které jsi zadal při exportu `.p12` |
| `APPLE_ID` | e-mail tvého Apple ID |
| `APPLE_TEAM_ID` | Team ID (10 znaků, najdeš v developer.apple.com → Membership) |
| `APPLE_APP_PASSWORD` | heslo pro aplikace z appleid.apple.com → Sign-In and Security → App-Specific Passwords |

Jakmile tam jsou, každé sestavení Mac verzi samo podepíše, notarizuje a „sešije“ (staple).
Dokud tam nejsou, Mac instalátor je jen na testování.

## Lokální sestavení

- Windows (z Linuxu): `scripts/build-windows.sh`
- macOS: `make -C plugin` (výsledek v `bin/`)
- Testy zvuku: `cd test && g++ -std=c++17 -O2 -I../plugin -I../dpf/distrho -I../dpf/distrho/src dsptest.cpp -o dsptest -lpthread -ldl && ./dsptest`

## Složky

- `plugin/` – zdrojový kód pluginu (DSP `TrapLimitPlugin.cpp`, UI `TrapWinBetonUI.cpp`)
- `dpf/` – framework DPF (ISC licence)
- `test/` – testy zvuku a UI
- `dist/` – soubory přikládané zákazníkům (manuál, EULA, licence, návod)
- `docs/manual/` – zdroj manuálu (HTML → PDF přes `mkpdf.js`)
- `installer/` – instalátor Windows (Inno Setup) a Mac (pkg)
