VJoyAxis selectedAxis(const std::atomic<int>& axis) {
    return axis.load() == 0 ? VJoyAxis::X : VJoyAxis::Y;
}

void setPreviewAxis(App& app, VJoyAxis axis, long position) {
    if (axis == VJoyAxis::X) app.xPosition = position;
    else app.yPosition = position;
}

void setMousePreviewAxis(App& app, VJoyAxis axis, long position) {
    if (axis == VJoyAxis::X) app.mouseXPosition = position;
    else app.mouseYPosition = position;
}

void setAudioPreviewAxis(App& app, VJoyAxis axis, long position) {
    if (axis == VJoyAxis::X) app.audioXPosition = position;
    else app.audioYPosition = position;
}

void setStatus(App& app, const std::wstring& text) {
    SetWindowTextW(app.status, text.c_str());
}

void updateStrengthLabel(App& app) {
    SetWindowTextW(app.strengthLabel, (L"Strength: " + std::to_wstring(app.strength.load())).c_str());
}

void updateSmoothingLabel(App& app) {
    SetWindowTextW(app.smoothingLabel, (L"Smoothing: " + std::to_wstring(app.smoothing.load())).c_str());
}

double wheelRadiusMillimetres(const App& app) {
    // Kept as a helper name for settings compatibility; this is now direct
    // distance per rotation rather than a geometric radius.
    return app.wheelRadius.load() * (app.wheelRadiusUnit.load() == 1 ? 25.4 : 1.0);
}

double mouseReferenceRpm(const App& app) {
    return app.mapRpm.load() * 0.5;
}

double mouseDistancePerRotationMetres(const App& app) {
    return app.mouseDistancePerRotation.load() * (app.mouseDistanceUnit.load() == 1 ? 0.3048 : 1.0);
}

// This is the cadence reported to the dashboard. It must not use the
// mouse/audio hand-off threshold: a real, slow pedal pulse is still useful
// information and turning it into zero makes the cadence graph look broken.
double measuredAudioRpm(const App& app) {
    const double rpm = app.audioCadence.rpm();
    // Map RPM is also the physically plausible ceiling for this setup. A
    // short false interval must not briefly report or drive a higher cadence.
    return rpm > 0.0 ? std::min(rpm, app.mapRpm.load()) : 0.0;
}

void updateAudioLabel(App& app, bool audioFresh) {
    if (!app.audioCadenceEnabled.load()) {
        SetWindowTextW(app.audioLabel, L"Audio RPM: disabled");
        return;
    }
    if (!app.audioCadence.active()) {
        SetWindowTextW(app.audioLabel, L"Audio RPM: unavailable (check Windows recording input)");
        return;
    }
    if (!audioFresh) {
        const std::wstring text = L"Audio level: " + std::to_wstring(app.audioCadence.peakAmplitude())
            + L" / 32767  (waiting for pulse)";
        SetWindowTextW(app.audioLabel, text.c_str());
        return;
    }
    const double rpm = measuredAudioRpm(app);
    const double speedKmh = rpm * wheelRadiusMillimetres(app) * 60.0 / 1000000.0;
    const std::wstring text = L"Audio: " + std::to_wstring(static_cast<int>(std::lround(rpm)))
        + L" RPM  /  " + std::to_wstring(speedKmh).substr(0, 4) + L" km/h";
    SetWindowTextW(app.audioLabel, text.c_str());
}

void readWheelRadius(App& app) {
    wchar_t text[32]{};
    GetWindowTextW(app.wheelRadiusEdit, text, static_cast<int>(std::size(text)));
    wchar_t* end = nullptr;
    const double radius = std::wcstod(text, &end);
    if (end != text) app.wheelRadius = std::clamp(radius, 1.0, 10000.0);
    app.wheelRadiusUnit = SendMessageW(app.wheelUnitCombo, CB_GETCURSEL, 0, 0) == 1 ? 1 : 0;
}

void readMapRpm(App& app) {
    wchar_t text[32]{};
    GetWindowTextW(app.mapRpmEdit, text, static_cast<int>(std::size(text)));
    wchar_t* end = nullptr;
    const double value = std::wcstod(text, &end);
    if (end != text) app.mapRpm = std::clamp(value, 10.0, 500.0);
}

const wchar_t* modifierLabel(int modifier) {
    static constexpr const wchar_t* labels[] = {
        L"None", L"Left Ctrl", L"Right Ctrl", L"Left Shift", L"Right Shift", L"Left Alt", L"Right Alt", L"Caps Lock"
    };
    return labels[std::clamp(modifier, 0, 7)];
}

WORD modifierVirtualKey(int modifier) {
    static constexpr WORD keys[] = {0, VK_LCONTROL, VK_RCONTROL, VK_LSHIFT, VK_RSHIFT, VK_LMENU, VK_RMENU, VK_CAPITAL};
    return keys[std::clamp(modifier, 0, 7)];
}

void sendKey(WORD virtualKey, bool down) {
    if (virtualKey == 0) return;
    INPUT input{};
    input.type = INPUT_KEYBOARD;
    const UINT scanCode = MapVirtualKeyW(virtualKey, MAPVK_VK_TO_VSC);
    if (scanCode != 0) {
        input.ki.wScan = static_cast<WORD>(scanCode);
        input.ki.dwFlags = KEYEVENTF_SCANCODE;
    } else {
        input.ki.wVk = virtualKey;
    }
    if (!down) input.ki.dwFlags |= KEYEVENTF_KEYUP;
    if (virtualKey == VK_RCONTROL || virtualKey == VK_RMENU) input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    SendInput(1, &input, sizeof(input));
}

void tapKey(WORD virtualKey) {
    sendKey(virtualKey, true);
    sendKey(virtualKey, false);
}

// Toggle keys change state on key-down, so activate and release each send a
// full key press. Caps Lock modifiers are reference-counted to keep overlap
// between mappings from leaving the lock state inverted.
void activateMappingModifier(App& app, App::KeyMapping& mapping) {
    const WORD virtualKey = modifierVirtualKey(mapping.modifier);
    if (mapping.modifierToggle) {
        if (virtualKey != VK_CAPITAL) {
            tapKey(virtualKey);
            return;
        }
        mapping.capsLockModifierHeld = true;
        if (app.capsLockModifierUsers++ != 0) return;
        tapKey(virtualKey);
        app.capsLockModifierToggled = true;
        return;
    }
    if (virtualKey != VK_CAPITAL) {
        sendKey(virtualKey, true);
        return;
    }
    mapping.capsLockModifierHeld = true;
    if (app.capsLockModifierUsers++ != 0) return;
    sendKey(virtualKey, true);
    sendKey(virtualKey, false);
    app.capsLockModifierToggled = true;
}

void releaseMappingModifier(App& app, App::KeyMapping& mapping) {
    const WORD virtualKey = modifierVirtualKey(mapping.modifier);
    if (mapping.modifierToggle) {
        if (virtualKey != VK_CAPITAL) {
            tapKey(virtualKey);
            return;
        }
        if (!mapping.capsLockModifierHeld) return;
        mapping.capsLockModifierHeld = false;
        if (app.capsLockModifierUsers <= 0 || --app.capsLockModifierUsers != 0) return;
        if (!app.capsLockModifierToggled) return;
        tapKey(virtualKey);
        app.capsLockModifierToggled = false;
        return;
    }
    if (virtualKey != VK_CAPITAL) {
        sendKey(virtualKey, false);
        return;
    }
    if (!mapping.capsLockModifierHeld) return;
    mapping.capsLockModifierHeld = false;
    if (app.capsLockModifierUsers <= 0 || --app.capsLockModifierUsers != 0) return;
    if (!app.capsLockModifierToggled) return;
    sendKey(virtualKey, true);
    sendKey(virtualKey, false);
    app.capsLockModifierToggled = false;
}

bool mappingIsFunctionKey(wchar_t key) {
    return key >= kFunctionKeyMarker && key <= kLastFunctionKeyMarker;
}

std::wstring mappingKeyLabel(wchar_t key) {
    if (mappingIsFunctionKey(key)) return L"F" + std::to_wstring(key - kFunctionKeyMarker + 1);
    return std::wstring(1, key);
}

bool mappingUsesUnicodeTextInput(wchar_t key) {
    // Punctuation can be a dead key (notably ` on several European layouts).
    // Inject it literally so its output is independent of the active layout.
    return !mappingIsFunctionKey(key) && (key > 0x7f || !std::iswalnum(key));
}

WORD mappingVirtualKey(const App::KeyMapping& mapping) {
    if (mappingIsFunctionKey(mapping.key)) return static_cast<WORD>(VK_F1 + mapping.key - kFunctionKeyMarker);
    // Unicode-text mappings have no virtual key but are still valid mappings.
    if (mappingUsesUnicodeTextInput(mapping.key)) return 1;
    const SHORT value = VkKeyScanW(mapping.key);
    return value == -1 ? 0 : LOBYTE(value);
}

// VkKeyScanW also returns the keyboard-layout modifiers required to produce a
// character.  Keeping only its low byte made punctuation such as !, @ and ?
// arrive as 1, 2 and / respectively.
void sendMappingKey(const App::KeyMapping& mapping, bool down) {
    if (mappingIsFunctionKey(mapping.key)) {
        sendKey(mappingVirtualKey(mapping), down);
        return;
    }
    if (mappingUsesUnicodeTextInput(mapping.key)) {
        INPUT input{};
        input.type = INPUT_KEYBOARD;
        input.ki.wScan = mapping.key;
        input.ki.dwFlags = KEYEVENTF_UNICODE | (down ? 0 : KEYEVENTF_KEYUP);
        SendInput(1, &input, sizeof(input));
        return;
    }
    const SHORT value = VkKeyScanW(mapping.key);
    if (value == -1) return;
    const WORD key = LOBYTE(value);
    const BYTE modifiers = HIBYTE(value);
    const auto sendLayoutModifiers = [modifiers](bool pressed) {
        // Release in the reverse order so nested modifiers remain well-formed.
        constexpr WORD keys[] = {VK_SHIFT, VK_CONTROL, VK_MENU};
        if (pressed) {
            for (size_t index = 0; index < std::size(keys); ++index) {
                if ((modifiers & (1 << index)) != 0) sendKey(keys[index], true);
            }
        } else {
            for (size_t index = std::size(keys); index-- > 0;) {
                if ((modifiers & (1 << index)) != 0) sendKey(keys[index], false);
            }
        }
    };
    if (down) {
        sendLayoutModifiers(true);
        sendKey(key, true);
    } else {
        sendKey(key, false);
        sendLayoutModifiers(false);
    }
}

void tapMappingKey(const App::KeyMapping& mapping) {
    sendMappingKey(mapping, true);
    sendMappingKey(mapping, false);
}

bool mappingMatches(const App::KeyMapping& mapping, long signedPercent) {
    return signedPercent != 0 && mapping.minimum <= mapping.maximum
            && signedPercent >= mapping.minimum
            && (signedPercent < mapping.maximum || mapping.maximum == 100 || mapping.maximum == -100);
}

bool mappingCrossed(const App::KeyMapping& mapping, long previousPercent, long currentPercent) {
    if (mapping.minimum != mapping.maximum) return false;
    const long threshold = mapping.minimum;
    return (previousPercent < threshold && currentPercent >= threshold)
        || (previousPercent > threshold && currentPercent <= threshold);
}

// Keyboard mappings use the mouse's smoothed movement directly.  vJoy's
// output interpolation is deliberately excluded: it makes analogue output
// pleasant, but delays the point at which a key mapping crosses its threshold.
long keyboardPercentForMouseDelta(const App& app, long delta) {
    const double centre = static_cast<double>(VJoyOutput::centrePosition());
    const double halfRange = static_cast<double>(VJoyOutput::maximum() - VJoyOutput::centrePosition());
    const double mouseTarget = std::clamp(centre
        + static_cast<double>(delta) * app.strength.load() * kAxisUnitsPerStrengthStep,
        static_cast<double>(VJoyOutput::minimum()), static_cast<double>(VJoyOutput::maximum()));
    return std::clamp(static_cast<long>(std::lround((mouseTarget - centre) * 100.0 / halfRange)), -100L, 100L);
}

std::wstring keyboardOutputText(const App& app, long signedPercent) {
    std::wstring output;
    for (const auto& mapping : app.mappings) {
        if (!mappingMatches(mapping, signedPercent)) continue;
        if (!output.empty()) output += L"  |  ";
        if (mapping.modifier != 0) output += std::wstring(modifierLabel(mapping.modifier)) + L" + ";
        output += mappingKeyLabel(mapping.key);
    }
    return output.empty() ? L"Keyboard output: --" : L"Keyboard output: " + output;
}

void updateKeyboardMappings(App& app, long signedPercent, bool allowInjection) {
    const auto shouldBeActive = [signedPercent, allowInjection](const App::KeyMapping& mapping) {
        return allowInjection && mappingMatches(mapping, signedPercent);
    };

    // Release first.  Two adjacent mappings can use the same main key (for
    // example Shift+W followed by W); pressing W before releasing Shift+W
    // would otherwise immediately release the newly pressed W.
    for (auto& mapping : app.mappings) {
        const WORD key = mappingVirtualKey(mapping);
        if (!mapping.active || (shouldBeActive(mapping) && key != 0)) continue;
        if (mapping.keyToggle) tapMappingKey(mapping);
        else sendMappingKey(mapping, false);
        releaseMappingModifier(app, mapping);
        mapping.active = false;
    }
    for (auto& mapping : app.mappings) {
        const WORD key = mappingVirtualKey(mapping);
        if (mapping.active || !shouldBeActive(mapping) || key == 0) continue;
        activateMappingModifier(app, mapping);
        if (mapping.keyToggle) tapMappingKey(mapping);
        else sendMappingKey(mapping, true);
        mapping.active = true;
    }
}

void triggerThresholdMappings(App& app, long signedPercent, bool allowInjection) {
    if (!allowInjection) return;
    // Equal ranges are one-shot thresholds. Run these separately from the held
    // key refresh cycle so an overlapping held mapping cannot swallow a pulse.
    for (auto& mapping : app.mappings) {
        if (!mappingCrossed(mapping, app.previousKeyboardPercent, signedPercent)
            || mappingVirtualKey(mapping) == 0) continue;
        activateMappingModifier(app, mapping);
        tapMappingKey(mapping);
        releaseMappingModifier(app, mapping);
    }
}

void releaseKeyboardMappings(App& app) {
    for (auto& mapping : app.mappings) {
        if (!mapping.active) continue;
        if (mapping.keyToggle) tapMappingKey(mapping);
        else sendMappingKey(mapping, false);
        releaseMappingModifier(app, mapping);
        mapping.active = false;
    }
}

long readMappingNumber(HWND edit) {
    wchar_t text[32]{};
    GetWindowTextW(edit, text, static_cast<int>(std::size(text)));
    return std::clamp(_wtol(text), -100L, 100L);
}

void readMappingControls(App& app, size_t index) {
    auto& mapping = app.mappings[index];
    mapping.minimum = readMappingNumber(mapping.minimumEdit);
    mapping.maximum = readMappingNumber(mapping.maximumEdit);
    mapping.modifier = std::clamp(static_cast<int>(SendMessageW(mapping.modifierCombo, CB_GETCURSEL, 0, 0)), 0, 7);
    wchar_t key[8]{};
    GetWindowTextW(mapping.keyEdit, key, static_cast<int>(std::size(key)));
    if (key[0] != L'\0') mapping.key = key[0];
}

std::wstring deviceLabel(const InterceptionMouseDevice& device, size_t number) {
    constexpr size_t kMaxShown = 65;
    const auto shown = device.hardwareId.size() > kMaxShown
        ? device.hardwareId.substr(0, kMaxShown - 3) + L"..." : device.hardwareId;
    return L"Mouse " + std::to_wstring(number + 1) + L" (device " + std::to_wstring(device.id) + L"): " + shown;
}

void refreshMice(App& app) {
    app.mice = app.interception.mice();
    writeLog("Interception mice detected=" + std::to_string(app.mice.size()));
    SendMessageW(app.mouseCombo, CB_RESETCONTENT, 0, 0);
    for (size_t index = 0; index < app.mice.size(); ++index) {
        SendMessageW(app.mouseCombo, CB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(deviceLabel(app.mice[index], index).c_str()));
    }
    const int logDeviceId = app.mice.size() >= 2 ? app.mice[1].id : 0;
    app.rawMouseLogDeviceId = logDeviceId;
    app.rawMouseLogPacketsRemaining = logDeviceId == 0 ? 0 : 5000;
    app.rawMouseLogWindowStart = GetTickCount64();
    app.rawMouseLogWindowPackets = 0;
    if (logDeviceId != 0) {
        writeLog("raw mouse logging enabled for Mouse 2 (Interception device " + std::to_string(logDeviceId)
            + "); logging up to 5000 packets");
    }
    if (app.mice.empty()) {
        app.interception.selectMouse(0);
        setStatus(app, L"No Interception mouse devices found. Reboot Windows if the driver was just installed.");
        return;
    }
    const auto saved = std::find_if(app.mice.begin(), app.mice.end(), [&app](const auto& mouse) {
        return !app.selectedHardwareId.empty() && mouse.hardwareId == app.selectedHardwareId;
    });
    if (saved != app.mice.end()) {
        const int index = static_cast<int>(std::distance(app.mice.begin(), saved));
        SendMessageW(app.mouseCombo, CB_SETCURSEL, index, 0);
        app.interception.selectMouse(app.mouseInputEnabled.load() ? saved->id : 0);
        setStatus(app, L"Restored saved bike mouse and settings.");
    } else {
        SendMessageW(app.mouseCombo, CB_SETCURSEL, static_cast<WPARAM>(-1), 0);
        app.interception.selectMouse(0);
        setStatus(app, L"Choose the bike mouse. No mouse is suppressed until you select one.");
    }
}

void onBikeMovement(App& app, int x, int y, bool mouseInput = true) {
    if (mouseInput && !app.mouseInputEnabled.load()) return;
    long delta = selectedAxis(app.sourceAxis) == VJoyAxis::X ? x : y;
    if (app.reverseAxis.load()) delta = -delta;
    // Keep the diagnostic raw trace faithful to every reported movement packet.
    // The output path below still rejects optical-sensor jitter at rest.
    app.rawInputDelta.fetch_add(delta);
    if (mouseInput && app.mouseRotationMeasuring.load()) app.mouseRotationMeasurementDelta.fetch_add(std::abs(delta));
    // Optical sensors produce a little jitter at rest.  Do not let that keep
    // the output alive or prevent the fast-stop timeout from firing.
    if (delta == 0) return;
    const int direction = delta > 0 ? 1 : -1;
    if (direction != app.lastDirection.load()) app.audioCadence.resetRpm();
    app.lastDirection = direction;
    app.lastMousePacketDelta = std::abs(delta);
    // Individual mouse packets are capped (often at +/-79). Accumulating them
    // until the next output tick preserves speed: faster motion produces more
    // packets instead of flattening at the packet ceiling.
    app.pendingDelta.fetch_add(delta);
    app.lastMovementTick = GetTickCount64();
}

COLORREF keyColourFor(const App& app, wchar_t key) {
    static constexpr COLORREF colours[] = {
        RGB(89, 217, 117), RGB(55, 189, 232), RGB(189, 116, 245), RGB(240, 170, 77), RGB(238, 107, 141),
        RGB(89, 213, 200), RGB(240, 107, 77), RGB(124, 156, 255), RGB(214, 200, 90), RGB(232, 120, 196),
    };
    const wchar_t target = static_cast<wchar_t>(std::towupper(key));
    std::vector<wchar_t> keys;
    for (const auto& mapping : app.mappings) {
        const wchar_t mappedKey = static_cast<wchar_t>(std::towupper(mapping.key));
        if (std::find(keys.begin(), keys.end(), mappedKey) != keys.end()) continue;
        if (mappedKey == target) return colours[keys.size() % std::size(colours)];
        keys.push_back(mappedKey);
    }
    return colours[0];
}

COLORREF modifierColourFor(int modifier) {
    static constexpr COLORREF colours[] = {
        0, RGB(75, 159, 255), RGB(118, 184, 255), RGB(169, 124, 255),
        RGB(208, 161, 255), RGB(240, 163, 77), RGB(244, 198, 106), RGB(231, 110, 165),
    };
    return colours[std::clamp(modifier, 0, 7)];
}

COLORREF overlayPropertyColour(HWND window, const wchar_t* name, COLORREF fallback) {
    const auto value = reinterpret_cast<ULONG_PTR>(GetPropW(window, name));
    return value ? static_cast<COLORREF>(value - 1) : fallback;
}

LRESULT CALLBACK statsOverlayProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) return TRUE;
    if (message == WM_NCHITTEST) return HTCAPTION;
    if (message == WM_PAINT) {
        PAINTSTRUCT paint{};
        HDC paintDc = BeginPaint(window, &paint);
        RECT rect{};
        GetClientRect(window, &rect);
        const int width = rect.right - rect.left;
        const int height = rect.bottom - rect.top;
        HDC screenDc = GetDC(nullptr);
        HDC dc = CreateCompatibleDC(screenDc);
        BITMAPINFO bitmapInfo{};
        bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmapInfo.bmiHeader.biWidth = width;
        bitmapInfo.bmiHeader.biHeight = -height;
        bitmapInfo.bmiHeader.biPlanes = 1;
        bitmapInfo.bmiHeader.biBitCount = 32;
        bitmapInfo.bmiHeader.biCompression = BI_RGB;
        void* bitmapPixels = nullptr;
        HBITMAP bitmap = CreateDIBSection(screenDc, &bitmapInfo, DIB_RGB_COLORS, &bitmapPixels, nullptr, 0);
        HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
        // Start with a fully transparent canvas. GDI supplies RGB but not useful
        // alpha, so alpha is assigned explicitly after drawing below.
        ZeroMemory(bitmapPixels, static_cast<SIZE_T>(width) * height * sizeof(DWORD));
        HBRUSH accent = CreateSolidBrush(RGB(38, 190, 225));
        RECT accentRect{0, 0, 3, rect.bottom};
        FillRect(dc, &accentRect, accent);
        DeleteObject(accent);
        SetBkMode(dc, TRANSPARENT);
        wchar_t text[512]{};
        GetWindowTextW(window, text, static_cast<int>(std::size(text)));
        std::wistringstream lines(text);
        std::wstring outputLine, keyLine, vjoyLine, xboxLine;
        std::getline(lines, outputLine);
        std::getline(lines, keyLine);
        std::getline(lines, vjoyLine);
        std::getline(lines, xboxLine);

        const auto font = [](int height, int weight) {
            return CreateFontW(height, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        };
        const auto draw = [&](const std::wstring& value, RECT area, COLORREF colour, HFONT selectedFont, UINT flags = DT_LEFT | DT_VCENTER | DT_SINGLELINE) {
            HGDIOBJ old = SelectObject(dc, selectedFont);
            SetTextColor(dc, colour);
            DrawTextW(dc, value.c_str(), -1, &area, flags);
            SelectObject(dc, old);
        };
        HFONT labelFont = font(12, FW_SEMIBOLD);
        HFONT outputFont = font(30, FW_BOLD);
        HFONT keyFont = font(36, FW_BOLD);
        HFONT axisFont = font(17, FW_BOLD);
        const COLORREF muted = RGB(145, 169, 190);
        const COLORREF green = RGB(89, 217, 117);
        const COLORREF white = RGB(237, 244, 250);

        draw(L"OUTPUT", RECT{16, 12, 105, 34}, muted, labelFont);
        const std::wstring outputValue = outputLine.rfind(L"OUTPUT  ", 0) == 0 ? outputLine.substr(8) : L"0%";
        draw(outputValue, RECT{110, 5, rect.right - 16, 45}, green, outputFont);

        const std::wstring keys = keyLine.rfind(L"KEY  ", 0) == 0 ? keyLine.substr(5) : L"--";
        const COLORREF keyColour = overlayPropertyColour(window, L"MultiTron2KeyColour", RGB(27, 46, 64));
        const COLORREF modifierColour = overlayPropertyColour(window, L"MultiTron2ModifierColour", 0);
        HBRUSH keyBackground = CreateSolidBrush(keys == L"--" ? RGB(27, 46, 64) : keyColour);
        RECT keyBox{16, 56, rect.right - 16, 126};
        FillRect(dc, &keyBox, keyBackground);
        DeleteObject(keyBackground);
        if (keys != L"--" && modifierColour != 0) {
            HBRUSH modifierBorder = CreateSolidBrush(modifierColour);
            const int keyBoxMidpoint = keyBox.top + (keyBox.bottom - keyBox.top) / 2;
            RECT modifierRect{keyBox.left, keyBoxMidpoint, keyBox.right, keyBox.bottom};
            FillRect(dc, &modifierRect, modifierBorder);
            DeleteObject(modifierBorder);
        }
        draw(keys, RECT{27, 56, rect.right - 27, 118}, white, keyFont, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        int axisY = 137;
        const auto drawAxis = [&](const std::wstring& line, const std::wstring& name, COLORREF colour) {
            if (line.empty()) return;
            draw(name, RECT{16, axisY, 92, axisY + 26}, muted, labelFont);
            const std::wstring value = line.size() > name.size() + 2 ? line.substr(name.size() + 2) : L"0%";
            draw(value, RECT{92, axisY - 2, 160, axisY + 28}, colour, axisFont);
            axisY += 29;
        };
        drawAxis(vjoyLine, L"vJoy", RGB(104, 207, 255));
        drawAxis(xboxLine, L"Xbox", RGB(137, 219, 86));

        DeleteObject(labelFont);
        DeleteObject(outputFont);
        DeleteObject(keyFont);
        DeleteObject(axisFont);
        auto* pixels = static_cast<DWORD*>(bitmapPixels);
        for (int index = 0; index < width * height; ++index) {
            if ((pixels[index] & 0x00FFFFFF) != 0) pixels[index] |= 0xFF000000;
        }
        POINT target{};
        RECT windowRect{};
        GetWindowRect(window, &windowRect);
        target.x = windowRect.left;
        target.y = windowRect.top;
        SIZE size{width, height};
        POINT source{};
        BLENDFUNCTION blend{AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
        UpdateLayeredWindow(window, screenDc, &target, &size, dc, &source, 0, &blend, ULW_ALPHA);
        SelectObject(dc, oldBitmap);
        DeleteObject(bitmap);
        DeleteDC(dc);
        ReleaseDC(nullptr, screenDc);
        EndPaint(window, &paint);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

void positionStatsOverlay(App& app) {
    if (!app.statsOverlayWindow) return;
    RECT workArea{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
    constexpr int width = 320, height = 220, margin = 20;
    const int position = std::clamp(app.statsOverlayPosition.load(), 0, 3);
    const int x = (position == 1 || position == 3) ? workArea.right - width - margin : workArea.left + margin;
    const int y = (position == 2 || position == 3) ? workArea.bottom - height - margin : workArea.top + margin;
    ShowWindow(app.statsOverlayWindow, SW_SHOWNOACTIVATE);
    SetWindowPos(app.statsOverlayWindow, HWND_TOPMOST, x, y, width, height,
        SWP_NOACTIVATE);
}

void setStatsOverlayVisible(App& app, bool visible) {
    app.statsOverlayEnabled = visible;
    if (!app.statsOverlayWindow) return;
    if (visible) {
        SetWindowTextW(app.statsOverlayWindow, L"OUTPUT  0%\nKEY  --");
        positionStatsOverlay(app);
        writeLog("stats overlay shown");
    } else {
        ShowWindow(app.statsOverlayWindow, SW_HIDE);
        writeLog("stats overlay hidden");
    }
}

void updateStatsOverlay(App& app) {
    if (!app.statsOverlayEnabled.load() || !app.statsOverlayWindow) return;
    const auto percent = [](long value) {
        const double range = static_cast<double>(VJoyOutput::maximum() - VJoyOutput::centrePosition());
        return static_cast<long>(std::lround((value - VJoyOutput::centrePosition()) * 100.0 / range));
    };
    std::wstring activeKey = L"--";
    const App::KeyMapping* firstActiveMapping = nullptr;
    const bool mappingsEnabled = app.keyboardMappingsEnabled.load();
    const long keyboardPercent = app.lastKeyboardPercent.load();
    for (const auto& mapping : app.mappings) {
        if (!mappingsEnabled || !mappingMatches(mapping, keyboardPercent)) continue;
        if (!firstActiveMapping) firstActiveMapping = &mapping;
        if (activeKey == L"--") activeKey.clear();
        if (!activeKey.empty()) activeKey += L" | ";
        if (mapping.modifier != 0) activeKey += std::wstring(modifierLabel(mapping.modifier)) + L"+";
        activeKey += mappingKeyLabel(mapping.key);
    }
    const long outputPercent = percent(app.axisPosition.load());
    std::wstring text = L"OUTPUT  " + std::to_wstring(outputPercent) + L"%\n"
        + L"KEY  " + activeKey;
    if (app.vjoyEnabled.load() && app.vjoy.active()) {
        text += L"\nvJoy  " + std::to_wstring(outputPercent) + L"%";
    }
    if (app.xbox.active()) {
        const long xboxAxis = VJoyOutput::centrePosition() + static_cast<long>(std::lround(
            (app.axisPosition.load() - VJoyOutput::centrePosition()) * app.xboxStrength.load() / 100.0));
        text += L"\nXbox  " + std::to_wstring(percent(xboxAxis)) + L"%";
    }
    if (firstActiveMapping) {
        SetPropW(app.statsOverlayWindow, L"MultiTron2KeyColour", reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(keyColourFor(app, firstActiveMapping->key)) + 1));
        const COLORREF modifierColour = modifierColourFor(firstActiveMapping->modifier);
        if (modifierColour != 0) SetPropW(app.statsOverlayWindow, L"MultiTron2ModifierColour", reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(modifierColour) + 1));
        else RemovePropW(app.statsOverlayWindow, L"MultiTron2ModifierColour");
    } else {
        RemovePropW(app.statsOverlayWindow, L"MultiTron2KeyColour");
        RemovePropW(app.statsOverlayWindow, L"MultiTron2ModifierColour");
    }
    SetWindowTextW(app.statsOverlayWindow, text.c_str());
    InvalidateRect(app.statsOverlayWindow, nullptr, FALSE);
}

std::string hexadecimal(unsigned int value) {
    std::ostringstream text;
    text << std::hex << value;
    return text.str();
}

std::string utf8(const std::wstring& value) {
    if (value.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    std::string result(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), size, nullptr, nullptr);
    return result;
}

std::string rawInputDeviceName(HANDLE device) {
    UINT length = 0;
    if (GetRawInputDeviceInfoW(device, RIDI_DEVICENAME, nullptr, &length) != 0 || length == 0) return "unknown";
    std::wstring name(length, L'\0');
    if (GetRawInputDeviceInfoW(device, RIDI_DEVICENAME, name.data(), &length) == static_cast<UINT>(-1)) return "unknown";
    if (!name.empty() && name.back() == L'\0') name.pop_back();
    return utf8(name);
}

std::string rawReportBytes(const BYTE* data, size_t size) {
    constexpr char kHex[] = "0123456789abcdef";
    std::string result;
    result.reserve(size * 2);
    for (size_t index = 0; index < size; ++index) {
        result.push_back(kHex[data[index] >> 4]);
        result.push_back(kHex[data[index] & 0x0f]);
    }
    return result;
}

bool registerJoystickInputLogging(HWND window) {
    // Receive Generic Desktop joystick, gamepad, and multi-axis reports from
    // any connected controller. This is diagnostic-only and disabled by
    // kJoystickInputLoggingEnabled unless explicitly needed.
    const std::array<RAWINPUTDEVICE, 3> devices{{
        {0x01, 0x04, RIDEV_INPUTSINK, window},
        {0x01, 0x05, RIDEV_INPUTSINK, window},
        {0x01, 0x08, RIDEV_INPUTSINK, window},
    }};
    if (!RegisterRawInputDevices(devices.data(), static_cast<UINT>(devices.size()), sizeof(RAWINPUTDEVICE))) {
        writeLog("joystick raw HID registration failed error=" + std::to_string(GetLastError()));
        return false;
    }
    writeLog("joystick raw HID logging active; operate a controller to record reports");
    return true;
}

long long highResolutionTimestampMicroseconds();

void logJoystickInput(App& app, HRAWINPUT rawInput) {
    UINT size = 0;
    if (GetRawInputData(rawInput, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) != 0 || size == 0) return;
    std::vector<BYTE> buffer(size);
    if (GetRawInputData(rawInput, RID_INPUT, buffer.data(), &size, sizeof(RAWINPUTHEADER)) == static_cast<UINT>(-1)) return;
    const auto* input = reinterpret_cast<const RAWINPUT*>(buffer.data());
    if (input->header.dwType != RIM_TYPEHID) return;

    RID_DEVICE_INFO info{};
    info.cbSize = sizeof(info);
    UINT infoSize = sizeof(info);
    if (GetRawInputDeviceInfoW(input->header.hDevice, RIDI_DEVICEINFO, &info, &infoSize) == static_cast<UINT>(-1)) return;
    const USHORT page = info.hid.usUsagePage;
    const USHORT usage = info.hid.usUsage;
    if (page != 0x01 || (usage != 0x04 && usage != 0x05 && usage != 0x08)) return;

    if (app.loggedJoystickDevices.insert(input->header.hDevice).second) {
        writeLog("joystick HID device=" + rawInputDeviceName(input->header.hDevice)
            + " usage_page=0x" + hexadecimal(page) + " usage=0x" + hexadecimal(usage));
    }
    const size_t reportSize = input->data.hid.dwSizeHid;
    const size_t reportCount = input->data.hid.dwCount;
    for (size_t index = 0; index < reportCount; ++index) {
        writeLog("joystick_hid timestamp_us=" + std::to_string(highResolutionTimestampMicroseconds())
            + " report=" + std::to_string(index + 1) + "/" + std::to_string(reportCount)
            + " bytes=" + rawReportBytes(input->data.hid.bRawData + index * reportSize, reportSize));
    }
}

long long highResolutionTimestampMicroseconds() {
    static const long long frequency = [] {
        LARGE_INTEGER value{};
        QueryPerformanceFrequency(&value);
        return value.QuadPart;
    }();
    LARGE_INTEGER counter{};
    QueryPerformanceCounter(&counter);
    return counter.QuadPart * 1'000'000 / frequency;
}

void logRawMousePacket(App& app, int deviceId, const InterceptionMouseStroke& stroke) {
    if (deviceId != app.rawMouseLogDeviceId.load()) return;
    const int remaining = app.rawMouseLogPacketsRemaining.fetch_sub(1);
    if (remaining <= 0) return;

    const ULONGLONG now = GetTickCount64();
    const unsigned long packets = app.rawMouseLogWindowPackets.fetch_add(1) + 1;
    const ULONGLONG started = app.rawMouseLogWindowStart.load();
    if (now - started >= 1000) {
        const ULONGLONG elapsed = now - started;
        writeLog("mouse2_raw_rate window_ms=" + std::to_string(elapsed)
            + " packets=" + std::to_string(packets)
            + " hz=" + std::to_string(packets * 1000 / elapsed));
        app.rawMouseLogWindowStart = now;
        app.rawMouseLogWindowPackets = 0;
    }

    writeLog("mouse2_raw timestamp_us=" + std::to_string(highResolutionTimestampMicroseconds())
        + " tick=" + std::to_string(now)
        + " device=" + std::to_string(deviceId)
        + " x=" + std::to_string(stroke.x) + " y=" + std::to_string(stroke.y)
        + " state=0x" + hexadecimal(stroke.state)
        + " flags=0x" + hexadecimal(stroke.flags)
        + " wheel=" + std::to_string(stroke.rolling)
        + " info=0x" + hexadecimal(stroke.information));
    if (remaining == 1) writeLog("mouse2_raw logging limit reached; press Refresh to capture another 5000 packets");
}

void setMouseInputEnabled(App& app, bool enabled) {
    app.mouseInputEnabled = enabled;
    if (!enabled) {
        app.pendingDelta = 0;
        app.rawInputDelta = 0;
        app.rawMouseDelta = 0;
        app.averagedMouseDelta = 0;
        app.lastMousePacketDelta = 0;
        app.mouseTargetPosition = VJoyOutput::centrePosition();
    }
    const auto selected = std::find_if(app.mice.begin(), app.mice.end(), [&app](const auto& mouse) {
        return !app.selectedHardwareId.empty() && mouse.hardwareId == app.selectedHardwareId;
    });
    app.interception.selectMouse(enabled && selected != app.mice.end() ? selected->id : 0);
    saveSettings(app);
}

void onLoudnessControllerKey(App& app, unsigned short code, unsigned short state) {
    // Consumer-control volume keys are extended scan codes 0x30 (up) and
    // 0x2e (down).  Only key-down carries a turn; key-up is still consumed.
    constexpr unsigned short kKeyUp = 0x30;
    constexpr unsigned short kKeyDown = 0x2e;
    constexpr unsigned short kKeyRelease = 0x01;
    if (!app.loudnessInputEnabled.load() || (state & kKeyRelease) != 0) return;
    const int direction = code == kKeyUp ? 1 : code == kKeyDown ? -1 : 0;
    if (direction == 0) return;
    // Populate both dimensions, so the knob works with either configured
    // mouse source axis and joins the same smoothing/output pipeline.
    onBikeMovement(app, direction * kLoudnessControllerStep, direction * kLoudnessControllerStep, false);
}

void selectLoudnessController(App& app) {
    app.keyboards = app.interception.keyboards();
    const auto controller = std::find_if(app.keyboards.begin(), app.keyboards.end(), [](const auto& keyboard) {
        return keyboard.hardwareId.find(kLoudnessControllerHardwareId) != std::wstring::npos;
    });
    const int id = controller == app.keyboards.end() ? 0 : controller->id;
    app.loudnessKeyboardDeviceId = id;
    app.interception.selectKeyboard(app.loudnessInputEnabled.load() ? id : 0);
    if (controller != app.keyboards.end()) {
        writeLog("loudness controller selected as interception keyboard device " + std::to_string(controller->id));
        setStatus(app, L"Loudness controller selected: volume keys are routed to vJoy.");
    } else {
        writeLog("loudness controller keyboard interface was not found");
    }
}

// Consumer Control devices do not pass through Interception's keyboard
// device list. Windows still surfaces their volume usages as low-level volume
// keys, which lets us consume them before the shell changes loudness.
App* gLoudnessHookApp = nullptr;
HHOOK gLoudnessHook = nullptr;

LRESULT CALLBACK loudnessKeyboardHook(int code, WPARAM message, LPARAM data) {
    if (code != HC_ACTION || !gLoudnessHookApp || !gLoudnessHookApp->loudnessInputEnabled.load()) {
        return CallNextHookEx(gLoudnessHook, code, message, data);
    }
    const auto* key = reinterpret_cast<const KBDLLHOOKSTRUCT*>(data);
    if (key->vkCode != VK_VOLUME_UP && key->vkCode != VK_VOLUME_DOWN) {
        return CallNextHookEx(gLoudnessHook, code, message, data);
    }
    if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN) {
        int direction = key->vkCode == VK_VOLUME_UP ? 1 : -1;
        if (gLoudnessHookApp->loudnessReverse.load()) direction = -direction;
        onBikeMovement(*gLoudnessHookApp, direction * kLoudnessControllerStep, direction * kLoudnessControllerStep, false);
        const bool directionChanged = gLoudnessHookApp->loudnessLastDirection.load() != 0
            && direction != gLoudnessHookApp->loudnessLastDirection.load();
        gLoudnessHookApp->loudnessLastDirection = direction;
        ++gLoudnessHookApp->loudnessTurnSequence;
        const ULONGLONG now = GetTickCount64();
        const ULONGLONG previous = gLoudnessHookApp->loudnessLastTurnTick.exchange(now);
        if (directionChanged) {
            gLoudnessHookApp->loudnessRawRpm = 0.0;
            gLoudnessHookApp->loudnessRpm = 0.0;
        } else if (previous != 0 && now > previous) {
            const double instantRpm = 60000.0 / (static_cast<double>(now - previous)
                * static_cast<double>(gLoudnessHookApp->loudnessClicksPerRotation.load()));
            gLoudnessHookApp->loudnessRawRpm = direction * instantRpm;
            const double oldRpm = std::abs(gLoudnessHookApp->loudnessRpm.load());
            const double smoothing = gLoudnessHookApp->loudnessRpmSmoothing.load() / 100.0;
            gLoudnessHookApp->loudnessRpm = direction * (oldRpm == 0.0 ? instantRpm : oldRpm * smoothing + instantRpm * (1.0 - smoothing));
            gLoudnessHookApp->loudnessMaximumRpm = std::max(gLoudnessHookApp->loudnessMaximumRpm.load(),
                std::abs(gLoudnessHookApp->loudnessRpm.load()));
        }
    }
    return 1; // Consume both down and up: no system loudness change.
}

bool installLoudnessHook(App& app) {
    gLoudnessHookApp = &app;
    gLoudnessHook = SetWindowsHookExW(WH_KEYBOARD_LL, loudnessKeyboardHook, GetModuleHandleW(nullptr), 0);
    if (!gLoudnessHook) {
        gLoudnessHookApp = nullptr;
        writeLog("unable to install loudness keyboard hook");
        return false;
    }
    writeLog("loudness Consumer Control hook active");
    return true;
}

void removeLoudnessHook() {
    if (gLoudnessHook) UnhookWindowsHookEx(gLoudnessHook);
    gLoudnessHook = nullptr;
    gLoudnessHookApp = nullptr;
}

bool loudnessControllerIsConnected() {
    UINT count = 0;
    if (GetRawInputDeviceList(nullptr, &count, sizeof(RAWINPUTDEVICELIST)) != 0 || count == 0) return false;
    std::vector<RAWINPUTDEVICELIST> devices(count);
    if (GetRawInputDeviceList(devices.data(), &count, sizeof(RAWINPUTDEVICELIST)) == static_cast<UINT>(-1)) return false;
    for (const auto& device : devices) {
        UINT length = 0;
        if (GetRawInputDeviceInfoW(device.hDevice, RIDI_DEVICENAME, nullptr, &length) != 0 || length == 0) continue;
        std::wstring name(length, L'\0');
        if (GetRawInputDeviceInfoW(device.hDevice, RIDI_DEVICENAME, name.data(), &length) == static_cast<UINT>(-1)) continue;
        if (name.find(L"VID_1189&PID_8890") != std::wstring::npos) return true;
    }
    return false;
}

bool keyIsHeld(wchar_t key) {
    const SHORT scan = VkKeyScanW(std::towupper(key));
    return scan != -1 && (GetAsyncKeyState(LOBYTE(scan)) & 0x8000) != 0;
}

// The dashboard owns presentation only.  Input and output remain in the native
// router, so Interception, vJoy and the audio capture path stay low-latency.
