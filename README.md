# TrapWin Limiter

TW-2 clipper a true-peak limiter pro trap a underground (VST3 / AU / CLAP, Windows + macOS).
Postaveno na [DPF](https://github.com/DISTRHO/DPF) (ve složce `dpf/`).

> Soukromý repozitář – zdrojový kód placeného produktu, nezveřejňovat.

## Jak se staví nová verze

Všechno dělá GitHub sám (`.github/workflows/build.yml`). Číslo verze je v souboru `VERSION`
(a musí sedět s `d_version` v `plugin/TrapLimitPlugin.cpp` a nápisem v `plugin/TrapWinBetonUI.cpp` – CI to hlídá).

| Kdy | Co se stane |
|---|---|
| každý push do `main` | testy zvuku, sestavení Windows + Mac, pluginval, auval, oba zákaznické ZIPy jako Artifacts |
| **Actions → Build TrapWin Limiter → Run workflow**, `publish = yes` | to samé + **Release v&lt;VERSION&gt;** se dvěma ZIPy pro e-shop |

Zákaznické ZIPy (dvě varianty v e-shopu):
- `TrapWin_Limiter_v1.0.0_Windows.zip` – instalátor .exe, složka Manual Install (VST3), manuál PDF, EULA, licence, návod
- `TrapWin_Limiter_v1.0.0_macOS.zip` – instalátor .pkg (VST3 + AU), složka Manual Install (VST3 + AU), manuál PDF, EULA, licence, návod.
  Bez Apple údajů se jmenuje `..._macOS_NEPODEPSANO.zip` a **nesmí se prodávat**.

## Mac: podpis a notarizace (nutné pro prodej)

Bez toho macOS plugin zákazníkům zablokuje. Potřebuješ účet **Apple Developer Program** (99 USD/rok).
V Apple Developer účtu vytvoř certifikáty **Developer ID Application** a **Developer ID Installer**,
exportuj je z Klíčenky jako `.p12` a v GitHubu (Settings → Secrets and variables → Actions) přidej:

| Secret | Obsah |
|---|---|
| `MAC_CERT_APP_P12` | Developer ID Application `.p12` převedený do base64 (`base64 -i app.p12 \| pbcopy`) |
| `MAC_CERT_INSTALLER_P12` | Developer ID Installer `.p12` v base64 |
| `MAC_CERT_PASSWORD` | heslo, které jsi zadal při exportu `.p12` |
| `APPLE_ID` | e-mail tvého Apple ID |
| `APPLE_TEAM_ID` | Team ID (10 znaků, najdeš v developer.apple.com → Membership) |
| `APPLE_APP_PASSWORD` | heslo pro aplikace z appleid.apple.com → Sign-In and Security → App-Specific Passwords |

Názvy jsou stejné jako u TW-7 (trapwin-tw7-dynamic-eq). Secrets se nastavují pro každý repozitář zvlášť. Jakmile tam jsou, každé sestavení Mac verzi samo podepíše, notarizuje a „sešije“ (staple).
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
