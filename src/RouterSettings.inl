std::filesystem::path settingsPath() {
    const wchar_t* localAppData = _wgetenv(L"LOCALAPPDATA");
    const auto directory = localAppData ? std::filesystem::path(localAppData) / "MultiTron2"
                                        : std::filesystem::path("MultiTron2");
    return directory / "autosave.yaml";
}

std::filesystem::path sessionsPath() {
    return settingsPath().parent_path() / "sessions.yaml";
}

double loadTotalSessionDistanceMetres() {
    std::wifstream file(sessionsPath());
    if (!file) return 0.0;
    double totalMetres = 0.0;
    for (std::wstring line; std::getline(file, line);) {
        const auto key = line.find(L"distance_km:");
        if (key == std::wstring::npos) continue;
        wchar_t* end = nullptr;
        const double kilometres = std::wcstod(line.c_str() + key + std::wcslen(L"distance_km:"), &end);
        if (end != line.c_str() + key + std::wcslen(L"distance_km:")) totalMetres += std::max(0.0, kilometres) * 1000.0;
    }
    return totalMetres;
}

bool appendSessionMetrics(const App& app) {
    if (app.sessionDistanceMetres <= 0.0) return true;
    try {
        std::filesystem::create_directories(sessionsPath().parent_path());
        std::wofstream file(sessionsPath(), std::ios::app);
        if (!file) return false;
        const double seconds = app.sessionStartedTick == 0 ? 0.0
            : static_cast<double>(GetTickCount64() - app.sessionStartedTick) / 1000.0;
        const double averageKmh = seconds > 0.0 ? app.sessionDistanceMetres / seconds * 3.6 : 0.0;
        file << L"- {distance_km: " << app.sessionDistanceMetres / 1000.0
             << L", duration_seconds: " << seconds
             << L", average_kmh: " << averageKmh << L"}\n";
        return static_cast<bool>(file);
    } catch (const std::filesystem::filesystem_error&) { return false; }
}

namespace settings {
std::wstring trim(std::wstring value) {
    const auto first = value.find_first_not_of(L" \t\r");
    const auto last = value.find_last_not_of(L" \t\r");
    return first == std::wstring::npos ? L"" : value.substr(first, last - first + 1);
}

long number(const std::unordered_map<std::wstring, std::wstring>& values, const wchar_t* key, long fallback) {
    const auto found = values.find(key); if (found == values.end()) return fallback;
    wchar_t* end = nullptr; const long value = std::wcstol(found->second.c_str(), &end, 10);
    return end != found->second.c_str() && *end == L'\0' ? value : fallback;
}

double decimal(const std::unordered_map<std::wstring, std::wstring>& values, const wchar_t* key, double fallback) {
    const auto found = values.find(key); if (found == values.end()) return fallback;
    wchar_t* end = nullptr; const double value = std::wcstod(found->second.c_str(), &end);
    return end != found->second.c_str() && *end == L'\0' ? value : fallback;
}

bool flag(const std::unordered_map<std::wstring, std::wstring>& values, const wchar_t* key, bool fallback) {
    return number(values, key, fallback ? 1 : 0) != 0;
}
}

bool saveSettings(const App& app, const std::filesystem::path& path = settingsPath()) {
    try {
        std::filesystem::create_directories(path.parent_path());
        std::wofstream file(path, std::ios::trunc);
        if (!file) return false;
        file << L"# Mouse vJoy Router settings\nformat: mouse-vjoy-router-settings\nversion: 1\n";
        const auto value = [&file](const wchar_t* key, const auto& setting) { file << key << L": " << setting << L'\n'; };
        value(L"strength", app.strength.load()); value(L"xbox_strength", app.xboxStrength.load()); value(L"smoothing", app.smoothing.load());
        value(L"source_axis", app.sourceAxis.load()); value(L"target_axis", app.targetAxis.load()); value(L"reverse_axis", app.reverseAxis.load());
        value(L"keyboard_enabled", app.keyboardMappingsEnabled.load()); value(L"keyboard_repeat_interval_ms", app.keyboardRepeatIntervalMs.load()); value(L"keyboard_repeat_pause_ms", app.keyboardRepeatPauseMs.load());
        value(L"mouse_enabled", app.mouseInputEnabled.load()); value(L"input_update_interval_ms", app.inputUpdateIntervalMs.load()); value(L"mouse_rotation_delta", app.mouseRotationDelta.load()); value(L"mouse_distance_per_rotation", app.mouseDistancePerRotation.load()); value(L"mouse_distance_unit", app.mouseDistanceUnit.load());
        value(L"audio_enabled", app.audioCadenceEnabled.load()); value(L"audio_beep_enabled", app.audioBeepEnabled.load()); value(L"audio_pulse_amplitude", app.audioCadence.pulseAmplitude());
        value(L"wheel_distance", app.wheelRadius.load()); value(L"wheel_unit", app.wheelRadiusUnit.load()); value(L"map_rpm", app.mapRpm.load());
        value(L"vjoy_enabled", app.vjoyEnabled.load()); value(L"xbox_enabled", app.xboxEnabled.load()); value(L"xbox_axis", app.xboxAxis.load()); value(L"xbox_triggers_flipped", app.xboxTriggersFlipped.load());
        value(L"xbox_left_key", static_cast<unsigned int>(app.xboxXLeftKey.load())); value(L"xbox_right_key", static_cast<unsigned int>(app.xboxXRightKey.load()));
        value(L"loudness_enabled", app.loudnessInputEnabled.load()); value(L"loudness_reverse", app.loudnessReverse.load()); value(L"loudness_clicks", app.loudnessClicksPerRotation.load()); value(L"loudness_smoothing", app.loudnessRpmSmoothing.load()); value(L"loudness_wheel_distance", app.loudnessWheelRadius.load());
        value(L"stats_overlay_enabled", app.statsOverlayEnabled.load()); value(L"stats_overlay_position", app.statsOverlayPosition.load());
        value(L"mouse_mix", std::to_wstring(app.mouseMixMinimum.load()) + L"," + std::to_wstring(app.mouseMixMaximum.load()) + L"," + std::to_wstring(app.mouseMixPriority.load()));
        value(L"audio_mix", std::to_wstring(app.audioMixMinimum.load()) + L"," + std::to_wstring(app.audioMixMaximum.load()) + L"," + std::to_wstring(app.audioMixPriority.load()));
        value(L"loudness_mix", std::to_wstring(app.loudnessMixMinimum.load()) + L"," + std::to_wstring(app.loudnessMixMaximum.load()) + L"," + std::to_wstring(app.loudnessMixPriority.load()));
        file << L"selected_hardware_id: " << app.selectedHardwareId << L"\nmappings:\n";
        for (const auto& mapping : app.mappings) file << L"  - " << mapping.minimum << L"," << mapping.maximum << L"," << mapping.modifier << L"," << static_cast<unsigned int>(mapping.key) << L"," << mapping.modifierToggle << L"," << mapping.keyToggle << L'\n';
        return static_cast<bool>(file);
    } catch (const std::filesystem::filesystem_error&) { return false; }
}

bool loadSettings(App& app, const std::filesystem::path& path = settingsPath()) {
    std::wifstream file(path);
    if (!file) return false;
    std::unordered_map<std::wstring, std::wstring> values;
    std::vector<std::wstring> mappingRows;
    bool mappings = false;
    for (std::wstring line; std::getline(file, line);) {
        line = settings::trim(line); if (line.empty() || line.starts_with(L"#")) continue;
        if (line == L"mappings:") { mappings = true; continue; }
        if (mappings && line.starts_with(L"- ")) { mappingRows.push_back(settings::trim(line.substr(2))); continue; }
        const auto separator = line.find(L':'); if (separator == std::wstring::npos) continue;
        values[settings::trim(line.substr(0, separator))] = settings::trim(line.substr(separator + 1));
    }
    if (values[L"format"] != L"mouse-vjoy-router-settings" || settings::number(values, L"version", 0) != 1) return false;
    const auto bounded = [&values](const wchar_t* key, long fallback, long low, long high) { return std::clamp(settings::number(values, key, fallback), low, high); };
    app.strength = bounded(L"strength", kDefaultMovementStrength, kMinimumMovementStrength, kMaximumVJoyStrength); app.xboxStrength = bounded(L"xbox_strength", kDefaultMovementStrength, kMinimumMovementStrength, kMaximumXboxStrength); app.smoothing = bounded(L"smoothing", kDefaultSmoothing, 1, 200);
    app.sourceAxis = bounded(L"source_axis", 1, 0, 1); app.targetAxis = bounded(L"target_axis", 1, 0, 1); app.reverseAxis = settings::flag(values, L"reverse_axis", false);
    app.keyboardMappingsEnabled = settings::flag(values, L"keyboard_enabled", true); app.keyboardRepeatIntervalMs = bounded(L"keyboard_repeat_interval_ms", 1000, 1, 5000); app.keyboardRepeatPauseMs = bounded(L"keyboard_repeat_pause_ms", 5, 1, 1000);
    app.mouseInputEnabled = settings::flag(values, L"mouse_enabled", true); app.inputUpdateIntervalMs = bounded(L"input_update_interval_ms", 15, 5, 30); app.mouseRotationDelta = bounded(L"mouse_rotation_delta", 0, 0, 100000000); app.mouseDistancePerRotation = std::clamp(settings::decimal(values, L"mouse_distance_per_rotation", 1.0), 0.001, 10000.0); app.mouseDistanceUnit = bounded(L"mouse_distance_unit", 0, 0, 1);
    app.audioCadenceEnabled = settings::flag(values, L"audio_enabled", false); app.audioBeepEnabled = settings::flag(values, L"audio_beep_enabled", true); app.audioCadence.setPulseAmplitude(static_cast<int>(bounded(L"audio_pulse_amplitude", 30, 1, 32767)));
    app.wheelRadius = std::clamp(settings::decimal(values, L"wheel_distance", 1000), 1.0, 10000.0); app.wheelRadiusUnit = bounded(L"wheel_unit", 0, 0, 1); app.mapRpm = std::clamp(settings::decimal(values, L"map_rpm", kDefaultMapRpm), 10.0, 500.0);
    app.vjoyEnabled = settings::flag(values, L"vjoy_enabled", true); app.xboxEnabled = settings::flag(values, L"xbox_enabled", false); app.xboxAxis = bounded(L"xbox_axis", 0, 0, 4); app.xboxTriggersFlipped = settings::flag(values, L"xbox_triggers_flipped", false); app.xboxXLeftKey = static_cast<wchar_t>(bounded(L"xbox_left_key", L'A', 1, 0xffff)); app.xboxXRightKey = static_cast<wchar_t>(bounded(L"xbox_right_key", L'D', 1, 0xffff));
    app.loudnessInputEnabled = settings::flag(values, L"loudness_enabled", false); app.loudnessReverse = settings::flag(values, L"loudness_reverse", false); app.loudnessClicksPerRotation = bounded(L"loudness_clicks", 10, 1, 240); app.loudnessRpmSmoothing = bounded(L"loudness_smoothing", 55, 0, 97); app.loudnessWheelRadius = std::clamp(settings::decimal(values, L"loudness_wheel_distance", 1000), 1.0, 10000.0);
    app.statsOverlayEnabled = settings::flag(values, L"stats_overlay_enabled", true); app.statsOverlayPosition = bounded(L"stats_overlay_position", 3, 0, 3);
    const auto loadMix = [&values](const wchar_t* key, std::atomic<long>& minimum, std::atomic<long>& maximum, std::atomic<int>& priority) {
        const auto found = values.find(key); if (found == values.end()) return;
        std::wistringstream stream(found->second); long low = 0, high = 100; int rank = 1; wchar_t comma = 0;
        if (!(stream >> low >> comma >> high >> comma >> rank)) return;
        minimum = std::clamp(low, 0L, 100L); maximum = std::clamp(high, minimum.load(), 100L); priority = std::clamp(rank, 0, 3);
    };
    loadMix(L"mouse_mix", app.mouseMixMinimum, app.mouseMixMaximum, app.mouseMixPriority);
    loadMix(L"audio_mix", app.audioMixMinimum, app.audioMixMaximum, app.audioMixPriority);
    loadMix(L"loudness_mix", app.loudnessMixMinimum, app.loudnessMixMaximum, app.loudnessMixPriority);
    const auto hardware = values.find(L"selected_hardware_id"); if (hardware != values.end()) app.selectedHardwareId = hardware->second;
    app.mappings.clear();
    for (const auto& row : mappingRows) { std::wistringstream stream(row); App::KeyMapping mapping{}; wchar_t comma = 0; unsigned int key = 0; int modifierToggle = 0, keyToggle = 0; if (!(stream >> mapping.minimum >> comma >> mapping.maximum >> comma >> mapping.modifier >> comma >> key >> comma >> modifierToggle >> comma >> keyToggle)) continue; mapping.minimum = std::clamp(mapping.minimum, -100L, 100L); mapping.maximum = std::clamp(mapping.maximum, -100L, 100L); mapping.modifier = std::clamp(mapping.modifier, 0, 7); mapping.key = static_cast<wchar_t>(key); mapping.modifierToggle = modifierToggle != 0; mapping.keyToggle = keyToggle != 0; app.mappings.push_back(mapping); }
    return !app.mappings.empty();
}
