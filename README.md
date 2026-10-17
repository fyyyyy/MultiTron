# MultiTron

Open Source ReWASD alternative.

Windows program to convert excercise bikes or treadmills ( MaraTron ) or other hardware into precision game controllers. Agentic coded, can't be bothered to read all code so use at your own risk.
Download latest build: https://maratron2.sfo3.digitaloceanspaces.com/releases/MultiTron2-Setup-0.1.1.exe

<img width="1864" height="1382" alt="grafik" src="https://github.com/user-attachments/assets/a8675780-cd28-4a38-9ee0-8bcd47192c18" />


Mutliple Inputs, Multiple Outputs

* Use a mouse as input, MaraTron style.

* Also reads RPM sensor input via 3.5mm audio jack soundcard

<img width="300" alt="grafik" src="https://github.com/user-attachments/assets/ced886f3-8d0a-4f14-8401-09c85196329e" />

* Also reads USB loudness controllers as rotary sensors.

<img width="300" alt="grafik" src="https://github.com/user-attachments/assets/7f1e7111-3326-49f5-9bbf-9cbe8d47ca85" />


Outputs:

Keyboard
Virtual Joystick
Xbox Controller


## Technical overview

MultiTron is a native Windows C++20 application with an HTML/CSS/JavaScript dashboard. The executable and internal build target are currently named `MultiTron2`.

| Component | Implementation |
| --- | --- |
| Desktop UI | Ultralight with AppCore, loading local HTML assets; CPU rendering is enabled |
| UI components | Lit and TypeScript, bundled with esbuild |
| UI development server | Vite, serving `src/UI` |
| Mouse and USB volume dial input | Interception input-filter driver |
| Audio cadence input | Windows WinMM audio capture (`waveInOpen`), detecting pulses to calculate RPM |
| Virtual joystick output | vJoy Device 1, using `vJoyInterface.dll` |
| Xbox controller output | Virtual Xbox 360 controller through ViGEmClient and ViGEmBus |
| Native build | CMake 3.21 or newer, C++20 compiler, Windows SDK |
| Installer | Inno Setup 6 |

Ultralight renders the dashboard; input handling, smoothing, mappings, and controller output run in C++. The dashboard sends commands through a JavaScript/native bridge, and the native layer publishes configuration and live input/output values back to the UI.

## Build from source

Use a Windows x64 development environment with Visual Studio C++ build tools and the Windows SDK, CMake 3.21+, Git, and Node.js/npm compatible with the locked dependencies. Run commands from the repository root.

### Third-party SDK files

Download and extract the Windows x64 Ultralight SDK into `third_party/ultralight-sdk`. CMake expects this layout:

```text
third_party/
  ultralight-sdk/
    include/
      AppCore/App.h
    lib/
    bin/
    resources/
  interception/
    x64/
      interception.dll
```

Ultralight headers and libraries are required to compile. The build links `UltralightCore`, `AppCore`, `Ultralight`, and `WebCore`, then copies the SDK's runtime files and resources beside the executable. Third-party binaries are excluded from Git.

Place the Interception x64 DLL at the path above to have CMake copy it into the output directory. Installing the Interception driver is a separate step; copying the DLL alone does not install it.

CMake downloads [ViGEmClient](https://github.com/nefarius/ViGEmClient) through `FetchContent`, so initial configuration requires network access. The dependency currently follows its `master` branch. ViGEmBus must be installed separately for Xbox output.

### Compile and run

```powershell
npm ci
npm run build:ui
cmake -S . -B build -A x64
cmake --build build --config Release
.\build\Release\MultiTron2.exe
```

The commands above use a Visual Studio generator. `npm run build:ui` regenerates `src/UI/components.js` from the TypeScript components. CMake copies `src/UI` into the executable's `assets` directory. Keep the copied Ultralight DLLs, `resources`, and `assets` with the executable when distributing a build.

After configuring CMake, `build-and-run.bat` rebuilds the UI and native application, then launches it. `build-installer.bat` builds the native Release target and packages `build/Release` using Inno Setup 6, writing the installer into `dist`. Rebuild the UI first when its TypeScript components change.

### UI development

```powershell
npm ci
npm run dev
```

Vite serves the UI at `http://127.0.0.1:4173` and loads the TypeScript components directly. Browser preview does not provide the native Windows driver/controller backend.

For development inside the native application, `build-and-run.bat` sets `MOUSE_VJOY_ROUTER_UI_PATH` to this checkout's `src/UI` directory. The dashboard copies these assets into a `hot-ui` directory beside the executable and watches stylesheets for reloads. TypeScript changes still require rebuilding the component bundle.

## Runtime setup

- Install the [Interception driver](https://github.com/oblitum/Interception) for mouse suppression and USB volume dial capture. Run `install-interception.exe /install` from an Administrator Command Prompt, then restart Windows.
- For vJoy output, install vJoy and configure Device 1 with X and Y axes. The application loads `vJoyInterface.dll` from its DLL search path or the standard vJoy x64 installation directories.
- For Xbox output, install [ViGEmBus](https://github.com/nefarius/ViGEmBus/releases). The application creates a virtual Xbox 360 controller through the driver.
- For audio cadence input, connect the sensor to an audio input. Capture currently uses the Windows default recording device (`WAVE_MAPPER`).

Select the bike mouse, choose its motion axis and output mapping, then move the mouse. The dashboard displays input and output values.

The router uses the Interception input-filter driver. It forwards every mouse except the selected bike mouse unchanged; strokes from the selected mouse are consumed and mapped to vJoy, so they do not move the Windows cursor or affect the game as a normal mouse.

When the connected `LC_KEY` USB loudness controller (`VID_1189&PID_8890`) is present, the router also claims its keyboard interface. Its Volume Up and Volume Down turns are consumed before Windows can change system loudness and are fed into the same motion/smoothing path as the selected bike mouse. The controller works with either configured mouse source axis.

This is a system-wide input filter. If no mice appear in the dropdown after installation, reboot Windows, then reopen the router.

## Settings and source layout

Settings are automatically stored in `%LOCALAPPDATA%\MultiTron2\autosave.yaml`; session distance and timing records are appended to `sessions.yaml` in the same directory. The UI also supports saving and loading settings files manually.

| Path | Purpose |
| --- | --- |
| `src/main.cpp` | Startup, native window integration, and UI command dispatch |
| `src/RouterApp.inl` | Shared application state |
| `src/RouterInput.inl` | Input processing, smoothing, and output routing |
| `src/RouterDashboard.inl` | Ultralight dashboard and JavaScript/native bridge |
| `src/RouterNativeControls.inl` | Native controls and event handling |
| `src/RouterSettings.inl` | Settings and session persistence |
| `src/InterceptionInput.*`, `src/AudioCadenceInput.*` | Input backends |
| `src/VJoyOutput.*`, `src/XboxOutput.*` | Virtual controller backends |
| `src/UI/` | Dashboard assets and Lit components |
| `installer/` | Windows icon resources and Inno Setup definition |
