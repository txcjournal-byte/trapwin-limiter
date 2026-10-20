# BIT ERA by TrapWin

Anime-era game sounds instrument: 12 eras (1917 to now), each with its own cover, skin and sounds.
v0.1 test build: 36 embedded sounds (3 per era), pitched on C3, Volume / Attack / Decay / Release / Tone / Space,
play buttons, Drag WAV into the DAW, first-launch screen.

- Build: `cmake -S . -B build-bitera && cmake --build build-bitera --config Release`
- Tests: `BitEraTests` renders every preset; `BitEraSnapshot <dir>` renders the window of every era to PNG.
- CI: `.github/workflows/bitera.yml` publishes the latest build to the `bitera-test` release (fixed links listed there).
- Codes (never change after release): manufacturer `Trpw`, plugin `Btea`, bundle id `com.trapwin.bitera`.
