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


## Build

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\Release\MultiTron2.exe
```

Install and configure vJoy Device 1 with X and Y axes before launching the router. Select the bike mouse, choose its motion axis and vJoy destination axis, then move the mouse. The status line displays the raw deltas being sent.

The router uses the Interception input-filter driver. It forwards every mouse except the selected bike mouse unchanged; strokes from the selected mouse are consumed and mapped to vJoy, so they do not move the Windows cursor or affect the game as a normal mouse.

When the connected `LC_KEY` USB loudness controller (`VID_1189&PID_8890`) is present, the router also claims its keyboard interface. Its Volume Up and Volume Down turns are consumed before Windows can change system loudness and are fed into the same motion/smoothing path as the selected bike mouse. The controller works with either configured mouse source axis.

This is a system-wide input filter. If no mice appear in the dropdown after installation, reboot Windows, then reopen the router.
