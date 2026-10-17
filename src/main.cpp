#include "InterceptionInput.h"
#include "VJoyOutput.h"
#include "AudioCadenceInput.h"
#include "XboxOutput.h"

#include <AppCore/App.h>
#include <AppCore/JSHelpers.h>
#include <AppCore/Overlay.h>
#include <AppCore/Window.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
#include <mutex>
#include <numbers>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cwchar>
#include <cwctype>
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>

namespace {
constexpr wchar_t kClassName[] = L"MultiTron2Window";
constexpr wchar_t kPreviewClassName[] = L"MultiTron2Preview";
constexpr wchar_t kStatsOverlayClassName[] = L"MultiTron2StatsOverlay";
constexpr bool kJoystickInputLoggingEnabled = false;
constexpr UINT kIdMouseCombo = 1001;
constexpr UINT kIdSourceCombo = 1002;
constexpr UINT kIdTargetCombo = 1003;
constexpr UINT kIdRefresh = 1004;
constexpr UINT kIdStrength = 1005;
constexpr UINT kIdSmoothing = 1006;
constexpr UINT kIdReverse = 1007;
constexpr UINT kIdMappingAdd = 1099;
constexpr UINT kIdMappingBase = 1100;
constexpr UINT kIdAudioCadence = 1200;
constexpr UINT kIdWheelRadius = 1201;
constexpr UINT kIdWheelUnit = 1202;
constexpr UINT kIdMapRpm = 1203;
constexpr UINT kIdAudioBeep = 1204;
constexpr UINT kIdKeyboardMappingsEnabled = 1205;
constexpr UINT_PTR kDecayTimer = 1;
constexpr UINT_PTR kKeyboardRefreshTimer = 2;
constexpr ULONGLONG kMouseAverageWindowMs = 150;
constexpr double kInputSampleIntervalMs = 15.0;
constexpr long kStopNoiseDelta = 10;
constexpr long kMousePacketMaxDelta = 79;
constexpr long kMousePacketAudioThreshold = kMousePacketMaxDelta * 80 / 100;
constexpr long kLoudnessControllerStep = kMousePacketMaxDelta;
constexpr wchar_t kLoudnessControllerHardwareId[] = L"VID_1189&PID_8890&MI_00";
// Keep a valid 10 RPM cadence alive until the next expected six-second pulse.
constexpr ULONGLONG kAudioFreshMs = AudioCadenceInput::kRpmStaleMs;
constexpr double kDefaultMapRpm = 133.0;
constexpr long kDefaultMovementStrength = 1;
constexpr long kMinimumMovementStrength = 1;
constexpr long kMaximumVJoyStrength = 200;
constexpr long kMaximumXboxStrength = 200;
constexpr wchar_t kFunctionKeyMarker = 0xE000;
constexpr wchar_t kLastFunctionKeyMarker = kFunctionKeyMarker + 11;
// High-DPI mice can report large raw deltas even at a slow belt speed. Keep
// strength 1 deliberately gentle; the 1..50 slider provides the rest.
constexpr long kAxisUnitsPerStrengthStep = 1;
constexpr long kDefaultSmoothing = 1;
std::ofstream gLog;
std::mutex gLogMutex;
std::filesystem::path gLogPath;

class RouterDashboard;
std::filesystem::path settingsPath();

void writeLog(const std::string& line) {
    std::scoped_lock lock(gLogMutex);
    if (gLog.is_open()) { gLog << line << '\n'; gLog.flush(); }
}

std::string utf8LogText(const std::wstring& text) {
    if (text.empty()) return {};
    const int bytes = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    std::string result(bytes, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), bytes, nullptr, nullptr);
    return result;
}

std::optional<std::filesystem::path> chooseSettingsFile(HWND owner, bool saving) {
    std::array<wchar_t, MAX_PATH> filename{};
    if (saving) wcscpy_s(filename.data(), filename.size(), L"autosave.yaml");
    const auto defaultDirectory = settingsPath().parent_path().wstring();
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = owner;
    dialog.lpstrFilter = L"Mouse vJoy Router settings (*.yaml)\0*.yaml\0All files (*.*)\0*.*\0";
    dialog.lpstrFile = filename.data();
    dialog.nMaxFile = static_cast<DWORD>(filename.size());
    dialog.lpstrInitialDir = defaultDirectory.c_str();
    dialog.lpstrDefExt = L"yaml";
    dialog.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | (saving ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    if (!(saving ? GetSaveFileNameW(&dialog) : GetOpenFileNameW(&dialog))) return std::nullopt;
    return std::filesystem::path(filename.data());
}

void openLog() {
    try {
        gLogPath = std::filesystem::temp_directory_path() / "MultiTron2.log";
    } catch (const std::filesystem::filesystem_error&) {
        gLogPath = "MultiTron2.log";
    }
    gLog.open(gLogPath, std::ios::app);
    writeLog("--- Mouse vJoy Router session started ---");
}

#include "RouterApp.inl"
#include "RouterSettings.inl"
#include "RouterInput.inl"
#include "RouterDashboard.inl"
#include "RouterNativeControls.inl"

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    openLog();
    timeBeginPeriod(1);
    INITCOMMONCONTROLSEX commonControls{sizeof(commonControls), ICC_BAR_CLASSES};
    InitCommonControlsEx(&commonControls);
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = windowProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = kClassName;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassW(&windowClass);
    WNDCLASSW previewClass{};
    previewClass.lpfnWndProc = previewProc;
    previewClass.hInstance = instance;
    previewClass.lpszClassName = kPreviewClassName;
    previewClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassW(&previewClass);
    WNDCLASSW overlayClass{};
    overlayClass.lpfnWndProc = statsOverlayProc;
    overlayClass.hInstance = instance;
    overlayClass.lpszClassName = kStatsOverlayClassName;
    overlayClass.hCursor = LoadCursor(nullptr, IDC_SIZEALL);
    RegisterClassW(&overlayClass);

    App app;
    const bool restoredSettings = loadSettings(app);
    app.totalDistanceMetres = loadTotalSessionDistanceMetres();
    app.sessionStartedTick = GetTickCount64();
    writeLog(restoredSettings ? "settings restored" : "no saved settings; using defaults");
    app.window = CreateWindowExW(0, kClassName, L"MultiTron2", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                                   CW_USEDEFAULT, CW_USEDEFAULT, 900, 560, nullptr, nullptr, instance, &app);
    if (!app.window) return 1;
    app.statsOverlayWindow = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED,
        kStatsOverlayClassName, L"", WS_POPUP, 0, 0, 320, 220, nullptr, nullptr, instance, nullptr);
    if (app.statsOverlayEnabled.load()) setStatsOverlayVisible(app, true);
    makeLabel(instance, app.window, L"Bike mouse (this one is blocked from Windows)", 22, 22, 300);
    app.mouseCombo = makeCombo(instance, app.window, kIdMouseCombo, 22, 46, 630);
    CreateWindowW(L"BUTTON", L"Refresh mice", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                  22, 90, 120, 28, app.window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdRefresh)), instance, nullptr);
    makeLabel(instance, app.window, L"Read mouse axis", 22, 140, 140);
    app.sourceCombo = makeCombo(instance, app.window, kIdSourceCombo, 170, 136, 120);
    SendMessageW(app.sourceCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"X (left/right)"));
    SendMessageW(app.sourceCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Y (up/down)"));
    SendMessageW(app.sourceCombo, CB_SETCURSEL, app.sourceAxis.load(), 0);
    makeLabel(instance, app.window, L"Map to vJoy axis", 330, 140, 140);
    app.targetCombo = makeCombo(instance, app.window, kIdTargetCombo, 475, 136, 165);
    SendMessageW(app.targetCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"X"));
    SendMessageW(app.targetCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Y"));
    SendMessageW(app.targetCombo, CB_SETCURSEL, app.targetAxis.load(), 0);
    makeLabel(instance, app.window, L"Axis strength", 22, 185, 130);
    app.strengthSlider = CreateWindowW(TRACKBAR_CLASSW, nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_AUTOTICKS,
        170, 180, 330, 34, app.window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdStrength)), instance, nullptr);
    SendMessageW(app.strengthSlider, TBM_SETRANGE, TRUE, MAKELONG(kMinimumMovementStrength, kMaximumVJoyStrength));
    SendMessageW(app.strengthSlider, TBM_SETTICFREQ, 1, 0);
    SendMessageW(app.strengthSlider, TBM_SETPOS, TRUE, app.strength.load());
    app.strengthLabel = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE,
                                      515, 187, 130, 20, app.window, nullptr, instance, nullptr);
    updateStrengthLabel(app);
    makeLabel(instance, app.window, L"Output smoothing", 22, 225, 130);
    app.smoothingSlider = CreateWindowW(TRACKBAR_CLASSW, nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_AUTOTICKS,
        170, 220, 330, 34, app.window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdSmoothing)), instance, nullptr);
    SendMessageW(app.smoothingSlider, TBM_SETRANGE, TRUE, MAKELONG(1, 200));
    SendMessageW(app.smoothingSlider, TBM_SETTICFREQ, 20, 0);
    SendMessageW(app.smoothingSlider, TBM_SETPOS, TRUE, app.smoothing.load());
    app.smoothingLabel = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE,
                                       515, 227, 130, 20, app.window, nullptr, instance, nullptr);
    updateSmoothingLabel(app);
    app.reverseCheck = CreateWindowW(L"BUTTON", L"Reverse direction", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                     22, 265, 170, 24, app.window,
                                     reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdReverse)), instance, nullptr);
    SendMessageW(app.reverseCheck, BM_SETCHECK, app.reverseAxis.load() ? BST_CHECKED : BST_UNCHECKED, 0);
    app.keyboardMappingsCheck = CreateWindowW(L"BUTTON", L"Enable keyboard mappings", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                              210, 265, 185, 24, app.window,
                                              reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdKeyboardMappingsEnabled)), instance, nullptr);
    SendMessageW(app.keyboardMappingsCheck, BM_SETCHECK, app.keyboardMappingsEnabled.load() ? BST_CHECKED : BST_UNCHECKED, 0);
    makeLabel(instance, app.window, L"Keyboard mappings (signed output %)", 22, 305, 240);
    makeLabel(instance, app.window, L"From", 22, 320, 68);
    makeLabel(instance, app.window, L"To", 102, 320, 68);
    makeLabel(instance, app.window, L"Modifier", 182, 320, 155);
    makeLabel(instance, app.window, L"Key", 350, 320, 56);
    app.addMappingButton = CreateWindowW(L"BUTTON", L"+ Add mapping", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        510, 302, 125, 27, app.window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdMappingAdd)), instance, nullptr);
    makeLabel(instance, app.window, L"vJoy X / Y preview", 680, 15, 180);
    app.preview = CreateWindowExW(0, kPreviewClassName, nullptr, WS_CHILD | WS_VISIBLE,
                                  680, 38, 180, 180, app.window, nullptr, instance, &app);
    app.keyboardPreview = CreateWindowW(L"STATIC", L"Keyboard output: --", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                        680, 225, 195, 44, app.window, nullptr, instance, nullptr);
    app.audioCheck = CreateWindowW(L"BUTTON", L"Use 3.5 mm audio cadence RPM", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                   680, 275, 205, 24, app.window,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdAudioCadence)), instance, nullptr);
    SendMessageW(app.audioCheck, BM_SETCHECK, app.audioCadenceEnabled.load() ? BST_CHECKED : BST_UNCHECKED, 0);
    app.audioBeepCheck = CreateWindowW(L"BUTTON", L"Beep on cadence pulse", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                       680, 298, 180, 24, app.window,
                                       reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdAudioBeep)), instance, nullptr);
    SendMessageW(app.audioBeepCheck, BM_SETCHECK, app.audioBeepEnabled.load() ? BST_CHECKED : BST_UNCHECKED, 0);
    makeLabel(instance, app.window, L"Travel distance per rotation", 680, 330, 150);
    const std::wstring radiusText = std::to_wstring(app.wheelRadius.load());
    app.wheelRadiusEdit = CreateWindowW(L"EDIT", radiusText.c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
                                        765, 326, 65, 23, app.window,
                                        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdWheelRadius)), instance, nullptr);
    app.wheelUnitCombo = makeCombo(instance, app.window, kIdWheelUnit, 838, 324, 48);
    SendMessageW(app.wheelUnitCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"mm"));
    SendMessageW(app.wheelUnitCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"in"));
    SendMessageW(app.wheelUnitCombo, CB_SETCURSEL, app.wheelRadiusUnit.load(), 0);
    makeLabel(instance, app.window, L"Map RPM", 680, 361, 65);
    const std::wstring mapRpmText = std::to_wstring(app.mapRpm.load());
    app.mapRpmEdit = CreateWindowW(L"EDIT", mapRpmText.c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
                                   750, 357, 65, 23, app.window,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdMapRpm)), instance, nullptr);
    makeLabel(instance, app.window, L"= full output", 822, 361, 70);
    app.audioLabel = CreateWindowW(L"STATIC", L"Audio RPM: disabled", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                   680, 390, 205, 42, app.window, nullptr, instance, nullptr);
    app.status = CreateWindowW(L"STATIC", L"Starting...", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                210, 265, 440, 42, app.window, nullptr, instance, nullptr);
    relayoutMappingRows(app);
    if (kJoystickInputLoggingEnabled) registerJoystickInputLogging(app.window);

    app.vjoy.initialize();
    writeLog(app.vjoy.active() ? "vJoy Device 1 active" : "vJoy unavailable");
    if (app.xboxEnabled.load()) {
        app.lastXboxInitializeTick = GetTickCount64();
        if (!app.xbox.initialize()) writeLog("Xbox output unavailable");
        saveSettings(app);
    }
    if (!app.interception.start([&app](int x, int y) { onBikeMovement(app, x, y); },
                                [&app](unsigned short code, unsigned short state) { onLoudnessControllerKey(app, code, state); },
                                [&app](int deviceId, const InterceptionMouseStroke& stroke) { logRawMousePacket(app, deviceId, stroke); })) {
        setStatus(app, L"Could not start suppression: " + app.interception.status());
        const auto& status = app.interception.status();
        writeLog("Interception input filter could not start: " + utf8LogText(status));
        MessageBoxW(app.window,
                    (L"Bike mouse input requires the Interception driver.\n\n"
                     L"1. Download Interception from github.com/oblitum/Interception.\n"
                     L"2. Open an Administrator Command Prompt in its folder.\n"
                     L"3. Run: install-interception.exe /install\n"
                     L"4. Restart Windows, then reopen MultiTron2.\n\n"
                     L"Detected problem: " + app.interception.status()).c_str(),
                    L"Install Interception driver", MB_OK | MB_ICONERROR);
    } else {
        refreshMice(app);
        selectLoudnessController(app);
        writeLog("Interception input filter active");
        if (app.mouseInputEnabled.load()) {
            if (app.mice.empty()) {
                MessageBoxW(app.window,
                            L"No bike mouse was detected. Connect the bike mouse, then select Refresh in MultiTron2.",
                            L"MultiTron2 startup error", MB_OK | MB_ICONERROR);
            } else if (app.selectedHardwareId.empty()) {
                MessageBoxW(app.window,
                            L"No bike mouse is configured. Select the mouse mounted on the bike before riding.",
                            L"MultiTron2 startup error", MB_OK | MB_ICONERROR);
            } else if (std::none_of(app.mice.begin(), app.mice.end(), [&app](const auto& mouse) {
                           return mouse.hardwareId == app.selectedHardwareId;
                       })) {
                MessageBoxW(app.window,
                            L"The configured bike mouse is not connected. Connect it, then select Refresh in MultiTron2.",
                            L"MultiTron2 startup error", MB_OK | MB_ICONERROR);
            }
        }
    }
    installLoudnessHook(app);
    if (app.audioCadenceEnabled.load() && !app.audioCadence.start()) {
        app.audioCadenceEnabled = false;
        SendMessageW(app.audioCheck, BM_SETCHECK, BST_UNCHECKED, 0);
        saveSettings(app);
    }
    SetTimer(app.window, kDecayTimer, app.inputUpdateIntervalMs.load(), nullptr);
    // Keep the existing native controls as the low-latency controller layer;
    // the Ultralight window is the visible interface.
    ShowWindow(app.window, SW_HIDE);
    RouterDashboard dashboard(app);
    app.dashboard = &dashboard;
    dashboard.setCommandHandler([&app, &dashboard](const std::string& command, const std::string& value) {
        const auto click = [&app](UINT id, HWND control) {
            SendMessageW(app.window, WM_COMMAND, MAKEWPARAM(id, BN_CLICKED), reinterpret_cast<LPARAM>(control));
        };
        const int numeric = value.empty() ? 0 : std::atoi(value.c_str());
        if (command == "open-external") {
            if (value == "https://discord.gg/XkfF9mu6kx") {
                ShellExecuteW(app.dashboardWindow, L"open", L"https://discord.gg/XkfF9mu6kx", nullptr, nullptr, SW_SHOWNORMAL);
            }
        } else if (command == "open-log") {
            if (!gLog.is_open() || gLogPath.empty()) {
                MessageBoxW(app.dashboardWindow, L"The log file could not be opened.",
                            L"MultiTron2 log", MB_OK | MB_ICONERROR);
                return;
            }
            const std::wstring explorerArguments = L"/select,\"" + gLogPath.wstring() + L"\"";
            const auto result = reinterpret_cast<INT_PTR>(ShellExecuteW(
                app.dashboardWindow, L"open", L"explorer.exe", explorerArguments.c_str(), nullptr, SW_SHOWNORMAL));
            if (result <= 32) {
                MessageBoxW(app.dashboardWindow, L"Could not open the log file in Explorer.",
                            L"MultiTron2 log", MB_OK | MB_ICONERROR);
            }
        } else if (command == "settings-save") {
            const auto path = chooseSettingsFile(app.dashboardWindow, true);
            if (path) writeLog(saveSettings(app, *path) ? "settings saved" : "could not save settings");
        } else if (command == "settings-load") {
            const auto path = chooseSettingsFile(app.dashboardWindow, false);
            if (!path) return;
            releaseKeyboardMappings(app);
            if (!loadSettings(app, *path)) {
                writeLog("could not load settings");
                return;
            }
            app.loadedSettingsFilename = path->filename().wstring();
            SendMessageW(app.sourceCombo, CB_SETCURSEL, app.sourceAxis.load(), 0);
            SendMessageW(app.targetCombo, CB_SETCURSEL, app.targetAxis.load(), 0);
            SendMessageW(app.strengthSlider, TBM_SETPOS, TRUE, app.strength.load());
            SendMessageW(app.smoothingSlider, TBM_SETPOS, TRUE, app.smoothing.load());
            updateStrengthLabel(app); updateSmoothingLabel(app);
            SendMessageW(app.reverseCheck, BM_SETCHECK, app.reverseAxis.load() ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessageW(app.keyboardMappingsCheck, BM_SETCHECK, app.keyboardMappingsEnabled.load() ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessageW(app.audioCheck, BM_SETCHECK, app.audioCadenceEnabled.load() ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessageW(app.audioBeepCheck, BM_SETCHECK, app.audioBeepEnabled.load() ? BST_CHECKED : BST_UNCHECKED, 0);
            SetWindowTextW(app.wheelRadiusEdit, std::to_wstring(app.wheelRadius.load()).c_str());
            SendMessageW(app.wheelUnitCombo, CB_SETCURSEL, app.wheelRadiusUnit.load(), 0);
            SetWindowTextW(app.mapRpmEdit, std::to_wstring(app.mapRpm.load()).c_str());
            relayoutMappingRows(app);
            SetTimer(app.window, kDecayTimer, app.inputUpdateIntervalMs.load(), nullptr);
            refreshMice(app);
            app.interception.selectKeyboard(app.loudnessInputEnabled.load() ? app.loudnessKeyboardDeviceId.load() : 0);
            if (app.audioCadenceEnabled.load()) {
                if (!app.audioCadence.start()) app.audioCadenceEnabled = false;
            } else app.audioCadence.stop();
            SendMessageW(app.audioCheck, BM_SETCHECK, app.audioCadenceEnabled.load() ? BST_CHECKED : BST_UNCHECKED, 0);
            if (!app.vjoyEnabled.load()) {
                std::scoped_lock lock(app.outputMutex);
                app.vjoy.centre(selectedAxis(app.targetAxis));
            }
            if (app.xboxEnabled.load()) {
                app.lastXboxInitializeTick = GetTickCount64();
                if (!app.xbox.initialize()) writeLog("Xbox output unavailable");
            } else app.xbox.shutdown();
            setStatsOverlayVisible(app, app.statsOverlayEnabled.load());
            ++app.mappingRevision;
            dashboard.publishConfiguration();
            writeLog("settings loaded");
        } else if (command == "refresh") {
            refreshMice(app);
            dashboard.publishMice();
        } else if (command == "mouse-device") {
            const auto selected = std::find_if(app.mice.begin(), app.mice.end(), [numeric](const auto& mouse) {
                return mouse.id == numeric;
            });
            if (selected != app.mice.end()) {
                const int index = static_cast<int>(std::distance(app.mice.begin(), selected));
                SendMessageW(app.mouseCombo, CB_SETCURSEL, index, 0);
                SendMessageW(app.window, WM_COMMAND, MAKEWPARAM(kIdMouseCombo, CBN_SELCHANGE),
                             reinterpret_cast<LPARAM>(app.mouseCombo));
            }
        }
        else if (command == "source") {
            SendMessageW(app.sourceCombo, CB_SETCURSEL, numeric == 0 ? 0 : 1, 0);
            SendMessageW(app.window, WM_COMMAND, MAKEWPARAM(kIdSourceCombo, CBN_SELCHANGE), reinterpret_cast<LPARAM>(app.sourceCombo));
        } else if (command == "target") {
            SendMessageW(app.targetCombo, CB_SETCURSEL, numeric == 0 ? 0 : 1, 0);
            SendMessageW(app.window, WM_COMMAND, MAKEWPARAM(kIdTargetCombo, CBN_SELCHANGE), reinterpret_cast<LPARAM>(app.targetCombo));
        } else if (command == "vjoy-strength") {
            SendMessageW(app.strengthSlider, TBM_SETPOS, TRUE,
                std::clamp(static_cast<long>(numeric), kMinimumMovementStrength, kMaximumVJoyStrength));
            SendMessageW(app.window, WM_HSCROLL, 0, reinterpret_cast<LPARAM>(app.strengthSlider));
        } else if (command == "vjoy") {
            app.vjoyEnabled = numeric != 0;
            if (!app.vjoyEnabled.load()) {
                std::scoped_lock lock(app.outputMutex);
                app.vjoy.centre(selectedAxis(app.targetAxis));
            }
            saveSettings(app);
        } else if (command == "mouse") {
            setMouseInputEnabled(app, numeric != 0);
        } else if (command == "loudness") {
            app.loudnessInputEnabled = numeric != 0;
            app.interception.selectKeyboard(app.loudnessInputEnabled.load() ? app.loudnessKeyboardDeviceId.load() : 0);
            if (!app.loudnessInputEnabled.load()) {
                app.loudnessLastDirection = 0;
                app.loudnessTurnSequence = 0;
                app.loudnessLastTurnTick = 0;
                app.loudnessRpm = 0.0;
                app.loudnessRawRpm = 0.0;
                app.loudnessMaximumRpm = 0.0;
                app.loudnessConnected = false;
            }
            saveSettings(app);
        } else if (command == "loudness-reverse") {
            app.loudnessReverse = numeric != 0;
            saveSettings(app);
        } else if (command == "loudness-clicks") {
            app.loudnessClicksPerRotation = std::clamp(static_cast<long>(numeric), 1L, 240L);
            saveSettings(app);
        } else if (command == "loudness-smoothing") {
            app.loudnessRpmSmoothing = std::clamp(static_cast<long>(numeric), 0L, 97L);
            saveSettings(app);
        } else if (command == "loudness-radius") {
            const double distance = std::strtod(value.c_str(), nullptr);
            app.loudnessWheelRadius = std::clamp(distance, 1.0, 10000.0);
            saveSettings(app);
        } else if (command == "xbox-strength") {
            app.xboxStrength = std::clamp(static_cast<long>(numeric), kMinimumMovementStrength, kMaximumXboxStrength);
            saveSettings(app);
        } else if (command == "smoothing") {
            SendMessageW(app.smoothingSlider, TBM_SETPOS, TRUE, std::clamp(numeric, 1, 200));
            SendMessageW(app.window, WM_HSCROLL, 0, reinterpret_cast<LPARAM>(app.smoothingSlider));
        } else if (command == "input-update-interval") {
            app.inputUpdateIntervalMs = std::clamp(numeric, 5, 30);
            SetTimer(app.window, kDecayTimer, app.inputUpdateIntervalMs.load(), nullptr);
            saveSettings(app);
        } else if (command == "mouse-rotation-start") {
            app.mouseRotationMeasurementDelta = 0;
            app.mouseRotationMeasuring = true;
        } else if (command == "mouse-rotation-finish") {
            app.mouseRotationMeasuring = false;
            app.mouseRotationDelta = app.mouseRotationMeasurementDelta.load();
            saveSettings(app);
            dashboard.publishConfiguration();
        } else if (command == "mouse-rotation-cancel") {
            app.mouseRotationMeasuring = false;
            dashboard.publishConfiguration();
        } else if (command == "mouse-distance") {
            app.mouseDistancePerRotation = std::clamp(std::strtod(value.c_str(), nullptr), 0.001, 10000.0);
            saveSettings(app);
        } else if (command == "mouse-distance-unit") {
            app.mouseDistanceUnit = std::clamp(numeric, 0, 1);
            saveSettings(app);
        } else if (command == "mouse-mix-min") {
            app.mouseMixMinimum = std::clamp(static_cast<long>(numeric), 0L, app.mouseMixMaximum.load()); saveSettings(app);
        } else if (command == "mouse-mix-max") {
            app.mouseMixMaximum = std::clamp(static_cast<long>(numeric), app.mouseMixMinimum.load(), 100L); saveSettings(app);
        } else if (command == "mouse-mix-priority") {
            app.mouseMixPriority = std::clamp(numeric, 0, 3); saveSettings(app);
        } else if (command == "audio-mix-min") {
            app.audioMixMinimum = std::clamp(static_cast<long>(numeric), 0L, app.audioMixMaximum.load()); saveSettings(app);
        } else if (command == "audio-mix-max") {
            app.audioMixMaximum = std::clamp(static_cast<long>(numeric), app.audioMixMinimum.load(), 100L); saveSettings(app);
        } else if (command == "audio-mix-priority") {
            app.audioMixPriority = std::clamp(numeric, 0, 3); saveSettings(app);
        } else if (command == "loudness-mix-min") {
            app.loudnessMixMinimum = std::clamp(static_cast<long>(numeric), 0L, app.loudnessMixMaximum.load()); saveSettings(app);
        } else if (command == "loudness-mix-max") {
            app.loudnessMixMaximum = std::clamp(static_cast<long>(numeric), app.loudnessMixMinimum.load(), 100L); saveSettings(app);
        } else if (command == "loudness-mix-priority") {
            app.loudnessMixPriority = std::clamp(numeric, 0, 3); saveSettings(app);
        } else if (command == "reverse") {
            SendMessageW(app.reverseCheck, BM_SETCHECK, numeric ? BST_CHECKED : BST_UNCHECKED, 0);
            click(kIdReverse, app.reverseCheck);
        } else if (command == "keyboard") {
            SendMessageW(app.keyboardMappingsCheck, BM_SETCHECK, numeric ? BST_CHECKED : BST_UNCHECKED, 0);
            click(kIdKeyboardMappingsEnabled, app.keyboardMappingsCheck);
        } else if (command == "keyboard-repeat-interval") {
            app.keyboardRepeatIntervalMs = std::clamp(numeric, 1, 5000);
            saveSettings(app);
        } else if (command == "keyboard-repeat-pause") {
            app.keyboardRepeatPauseMs = std::clamp(numeric, 1, 1000);
            saveSettings(app);
        } else if (command == "overlay") {
            setStatsOverlayVisible(app, numeric != 0);
            saveSettings(app);
        } else if (command == "overlay-position") {
            app.statsOverlayPosition = std::clamp(numeric, 0, 3);
            if (app.statsOverlayEnabled.load()) positionStatsOverlay(app);
            saveSettings(app);
        } else if (command == "audio") {
            SendMessageW(app.audioCheck, BM_SETCHECK, numeric ? BST_CHECKED : BST_UNCHECKED, 0);
            click(kIdAudioCadence, app.audioCheck);
        } else if (command == "beep") {
            SendMessageW(app.audioBeepCheck, BM_SETCHECK, numeric ? BST_CHECKED : BST_UNCHECKED, 0);
            click(kIdAudioBeep, app.audioBeepCheck);
        } else if (command == "radius") {
            SetWindowTextA(app.wheelRadiusEdit, value.c_str());
            SendMessageW(app.window, WM_COMMAND, MAKEWPARAM(kIdWheelRadius, EN_CHANGE), reinterpret_cast<LPARAM>(app.wheelRadiusEdit));
        } else if (command == "unit") {
            SendMessageW(app.wheelUnitCombo, CB_SETCURSEL, numeric == 0 ? 0 : 1, 0);
            SendMessageW(app.window, WM_COMMAND, MAKEWPARAM(kIdWheelUnit, CBN_SELCHANGE), reinterpret_cast<LPARAM>(app.wheelUnitCombo));
        } else if (command == "rpm") {
            SetWindowTextA(app.mapRpmEdit, value.c_str());
            SendMessageW(app.window, WM_COMMAND, MAKEWPARAM(kIdMapRpm, EN_CHANGE), reinterpret_cast<LPARAM>(app.mapRpmEdit));
        } else if (command == "threshold") {
            app.audioCadence.setPulseAmplitude(numeric);
            saveSettings(app);
        } else if (command == "xbox") {
            const bool enable = numeric != 0;
            app.xboxEnabled = enable;
            if (enable) {
                app.lastXboxInitializeTick = GetTickCount64();
                if (!app.xbox.initialize()) writeLog("Xbox output unavailable");
            } else if (!enable) {
                app.xbox.shutdown();
            }
            saveSettings(app);
        } else if (command == "xbox-axis") {
            app.xboxAxis = std::clamp(numeric, 0, 4);
            saveSettings(app);
        } else if (command == "xbox-triggers-flipped") {
            app.xboxTriggersFlipped = numeric != 0;
            saveSettings(app);
        } else if (command == "xbox-x-left-key" && !value.empty()) {
            app.xboxXLeftKey = std::towupper(static_cast<unsigned char>(value.front()));
            saveSettings(app);
        } else if (command == "xbox-x-right-key" && !value.empty()) {
            app.xboxXRightKey = std::towupper(static_cast<unsigned char>(value.front()));
            saveSettings(app);
        } else if (command == "mapping") {
            std::stringstream values(value);
            std::string part;
            std::vector<std::string> parts;
            while (std::getline(values, part, '|')) parts.push_back(part);
            if (parts.size() == 7) {
                const size_t index = static_cast<size_t>(std::strtoul(parts[0].c_str(), nullptr, 10));
                if (index < app.mappings.size()) {
                    releaseKeyboardMappings(app);
                    auto& mapping = app.mappings[index];
                    mapping.minimum = std::clamp(static_cast<long>(std::strtol(parts[1].c_str(), nullptr, 10)), -100L, 100L);
                    mapping.maximum = std::clamp(static_cast<long>(std::strtol(parts[2].c_str(), nullptr, 10)), -100L, 100L);
                    mapping.modifier = std::clamp(std::atoi(parts[3].c_str()), 0, 7);
                    if (!parts[4].empty()) {
                        const auto key = static_cast<wchar_t>(std::strtoul(parts[4].c_str(), nullptr, 16));
                        if (key != L'\0') mapping.key = key;
                    }
                    mapping.modifierToggle = parts[5] == "1";
                    mapping.keyToggle = parts[6] == "1";
                    // Keep the hidden native controls in sync so a later
                    // shutdown cannot overwrite a dashboard edit.
                    relayoutMappingRows(app);
                    saveSettings(app);
                    ++app.mappingRevision;
                }
            }
        } else if (command == "mapping-add") {
            addMapping(app);
            ++app.mappingRevision;
        } else if (command == "mapping-clone") {
            const size_t index = static_cast<size_t>(std::strtoul(value.c_str(), nullptr, 10));
            if (index < app.mappings.size()) {
                releaseKeyboardMappings(app);
                app.mappings.insert(app.mappings.begin() + static_cast<std::ptrdiff_t>(index + 1), app.mappings[index]);
                relayoutMappingRows(app);
                saveSettings(app);
                ++app.mappingRevision;
            }
        } else if (command == "mapping-move-up") {
            const size_t index = static_cast<size_t>(std::strtoul(value.c_str(), nullptr, 10));
            if (index > 0 && index < app.mappings.size()) {
                releaseKeyboardMappings(app);
                std::swap(app.mappings[index], app.mappings[index - 1]);
                relayoutMappingRows(app);
                saveSettings(app);
                ++app.mappingRevision;
            }
        } else if (command == "mapping-move") {
            const auto separator = value.find('|');
            if (separator != std::string::npos) {
                const size_t from = static_cast<size_t>(std::strtoul(value.substr(0, separator).c_str(), nullptr, 10));
                const size_t to = static_cast<size_t>(std::strtoul(value.substr(separator + 1).c_str(), nullptr, 10));
                if (from < app.mappings.size() && to < app.mappings.size() && from != to) {
                    releaseKeyboardMappings(app);
                    auto mapping = std::move(app.mappings[from]);
                    app.mappings.erase(app.mappings.begin() + static_cast<std::ptrdiff_t>(from));
                    app.mappings.insert(app.mappings.begin() + static_cast<std::ptrdiff_t>(to), std::move(mapping));
                    relayoutMappingRows(app);
                    saveSettings(app);
                    ++app.mappingRevision;
                }
            }
        } else if (command == "mapping-remove") {
            const size_t index = static_cast<size_t>(std::strtoul(value.c_str(), nullptr, 10));
            if (index < app.mappings.size()) {
                releaseKeyboardMappings(app);
                app.mappings.erase(app.mappings.begin() + static_cast<std::ptrdiff_t>(index));
                relayoutMappingRows(app);
                saveSettings(app);
                ++app.mappingRevision;
            }
        }
    });
    dashboard.Run();
    app.dashboard = nullptr;
    removeLoudnessHook();
    app.interception.stop();
    if (!appendSessionMetrics(app)) writeLog("could not append session metrics");
    app.xbox.shutdown();
    app.vjoy.shutdown();
    timeEndPeriod(1);
    writeLog("--- Mouse vJoy Router session ended ---");
    return 0;
}
