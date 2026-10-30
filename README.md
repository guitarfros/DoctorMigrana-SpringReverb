# Doctor Migraña – Reverb

![Build](https://github.com/guitarfros/doctor-migrana-reverb/actions/workflows/build.yml/badge.svg)

Plugin VST3 de **reverb de resorte** para guitarra, de GuitarFros. Modelo de resorte tipo
Fender con cadea de filtros allpass dispersivos (o «chirp»/«drip» característico), dous
resortes en paralelo (estéreo), saturación de entrada controlada por *Dwell* e bypass con
fundido. Funciona en **Linux, Windows e macOS**.

## Controis
| Control | Que fai |
|---|---|
| **Mix** | Mestura entre o son seco e a reverb |
| **Decay** | Duración da cola |
| **Tone** | Pasabaixos da saída (luminosidade) |
| **Dwell** | Intensidade coa que se «golpea» o resorte (saturación de entrada) |
| **Master** | Volume de saída: -40 dB a +6 dB, 0 dB no centro (só actúa co pedal activo) |
| Interruptor | Activa/desactiva o efecto (bypass) |

Dobre clic nun knob para restablecelo. Un exemplo de son en `docs/demo_spring.wav`
(pluck sintético procesado polo mesmo motor).

## Descargar compilado
- **Releases (recomendado):** na pestana **Releases** do repositorio hai un ZIP por sistema
  (`Doctor-Migrana-Reverb-Linux-VST3.zip`, `...-Windows-VST3.zip`, `...-macOS-VST3.zip`) co VST3 e
  os ficheiros de licenza.
- **Actions:** en cada subida a `main`, GitHub Actions compila o plugin nos tres sistemas e deixa
  os ZIP como artefactos (pestana **Actions** > execución máis recente > *Artifacts*).

### Publicar unha versión nova (para o mantedor)
```bash
git tag v1.0.0
git push origin v1.0.0
```
Ao subir a etiqueta, o workflow compila nos tres sistemas e crea o Release cos tres ZIP.

## Compilar a man
CMake descarga JUCE 8.0.11 de GitHub na primeira compilación (fai falta internet).
Tamén se xera unha aplicación *Standalone* para probalo sen host.

### Linux (Ubuntu)
```bash
sudo apt install build-essential cmake pkg-config libasound2-dev libjack-jackd2-dev \
  libfreetype-dev libfontconfig1-dev libx11-dev libxcomposite-dev libxcursor-dev \
  libxext-dev libxinerama-dev libxrandr-dev libxrender-dev libglu1-mesa-dev mesa-common-dev

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(nproc)
```
O VST3 cópiase a `~/.vst3/`.

### Windows
Necesitas Visual Studio 2022 (carga de traballo «Desenvolvemento para escritorio con C++») e CMake.
```powershell
cmake -B build -DMIGRANA_COPY_AFTER_BUILD=OFF
cmake --build build --config Release --parallel
```
O plugin queda en `build\DoctorMigranaReverb_artefacts\Release\VST3\`. Copia
`Doctor Migrana - Reverb.vst3` a `C:\Program Files\Common Files\VST3\`.
(Se queres que o copie a compilación, executa a terminal como administrador e quita `-DMIGRANA_COPY_AFTER_BUILD=OFF`.)

### macOS
Necesitas Xcode (ou as *Command Line Tools*) e CMake.
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build --parallel
```
O VST3 cópiase a `~/Library/Audio/Plug-Ins/VST3/`. Os binarios descargados de Actions non
van asinados: se macOS os bloquea, executa `xattr -dr com.apple.quarantine "Doctor Migrana - Reverb.vst3"`.

Despois, no teu DAW (en Reaper: *Preferences > Plug-ins > VST > Clear cache and re-scan*).
Para non copiar o plugin ao compilar: `-DMIGRANA_COPY_AFTER_BUILD=OFF`.

## Estrutura
- `Source/SpringReverb.h` – motor DSP (C++ estándar, sen dependencias de JUCE)
- `Source/PluginProcessor.*` – parámetros, estado e procesado de audio
- `Source/PluginEditor.*` – interface (fondo, knobs de filmstrip, interruptor)
- `Assets/` – imaxes incrustadas no plugin (**non** AGPL, ver `ASSETS-LICENSE.md`)
- `.github/workflows/build.yml` – compilación automática en Linux, Windows e macOS

## Licenza
- **Código:** GNU AGPL v3 (`AGPL-3.0-only`). Ver `LICENSE` e `NOTICE.md`.
- **Imaxes:** xeradas con **Meta AI** (Meta Platforms, Inc.) e distribuídas neste proxecto
  non comercial e de código aberto; o logo e a marca «Doctor Migraña» son propiedade do autor.
  Ver `ASSETS-LICENSE.md`.
- JUCE úsase baixo a AGPLv3; VST é marca rexistrada de Steinberg Media Technologies GmbH.
