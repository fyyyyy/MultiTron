void relayoutMappingRows(App& app);
void addMapping(App& app);

LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* app = reinterpret_cast<App*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    switch (message) {
    case WM_NCCREATE:
        SetWindowLongPtrW(window, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams));
        return TRUE;
    case WM_INPUT:
        if (app) logJoystickInput(*app, reinterpret_cast<HRAWINPUT>(lParam));
        return 0;
    case WM_TIMER:
        if (app && wParam == kKeyboardRefreshTimer) {
            KillTimer(window, kKeyboardRefreshTimer);
            if (app->keyboardRefreshInProgress) {
                app->keyboardRefreshInProgress = false;
                updateKeyboardMappings(*app, app->lastKeyboardPercent.load(),
                    GetForegroundWindow() != (app->dashboardWindow ? app->dashboardWindow : app->window)
                        && app->keyboardMappingsEnabled.load());
            }
            return 0;
        }
        if (wParam != kDecayTimer) return 0;
        if (app) {
            const ULONGLONG now = GetTickCount64();
            const bool audioEnabled = app->audioCadenceEnabled.load();
            const ULONGLONG audioPulse = audioEnabled ? app->audioCadence.lastPulseTick() : 0;
            const unsigned long audioPulseSequence = audioEnabled ? app->audioCadence.pulseSequence() : 0;
            if (audioEnabled && audioPulseSequence != app->lastAudioPulseSequence) {
                app->lastAudioPulseSequence = audioPulseSequence;
                if (app->audioBeepEnabled.load()) Beep(2000, 12);
            }
            const double measuredRpm = audioEnabled ? measuredAudioRpm(*app) : 0.0;
            const bool audioFresh = audioEnabled && app->audioCadence.active()
                && audioPulse != 0 && now - audioPulse <= kAudioFreshMs && measuredRpm > 0.0;
            updateAudioLabel(*app, audioFresh);
            // The mouse remains the stop sensor. Audio cadence can only update
            // once per revolution, so a previous RPM must never keep the game
            // moving after the belt/mouse has stopped.
            // Spread each mouse packet across a short rolling window. Optical
            // mouse reports can arrive in bursts, so treating one empty 15 ms
            // tick as zero produces false stops and visibly jerky output.
            // Mouse reports are relative deltas. Sum every report, then
            // normalize every sample to the 15 ms reference interval. This
            // keeps the displayed values and output strength consistent when
            // the user chooses a faster or slower input update rate.
            const long long inputSampleTimestampUs = highResolutionTimestampMicroseconds();
            const long long previousInputSample = app->lastInputSampleTimestampUs;
            app->lastInputSampleTimestampUs = inputSampleTimestampUs;
            app->lastInputSampleTick = now;
            const long long targetInputIntervalUs = static_cast<long long>(app->inputUpdateIntervalMs.load()) * 1'000LL;
            const long long referenceInputIntervalUs = static_cast<long long>(kInputSampleIntervalMs * 1'000.0);
            const long long elapsedInputUs = previousInputSample == 0 ? targetInputIntervalUs
                : std::clamp(inputSampleTimestampUs - previousInputSample,
                    (std::max)(1'000LL, targetInputIntervalUs / 2), 60'000LL);
            const double inputRateScale = static_cast<double>(referenceInputIntervalUs) / static_cast<double>(elapsedInputUs);
            const long rawDelta = static_cast<long>(std::lround(app->pendingDelta.exchange(0) * inputRateScale));
            const long rawInputDelta = app->rawInputDelta.exchange(0);
            app->rawMouseDelta = static_cast<long>(std::lround(rawInputDelta * inputRateScale));
            const long previousAverage = app->averagedMouseDelta.load();
            // The dashboard's Smoothed mouse trace and the output path share
            // the Output smoothing control. At minimum smoothing this follows
            // raw packets; at maximum it retains up to 95% of the prior value.
            const double smoothing = (static_cast<double>(app->smoothing.load()) - 1.0) / 199.0;
            // Apply smoothing over elapsed time rather than once per timer
            // tick, so changing the input interval does not change its feel.
            const double inputRetention = std::pow(smoothing * 0.95,
                static_cast<double>(elapsedInputUs) / referenceInputIntervalUs);
            // A brief movement in the opposite direction is an immediate stop
            // signal. The first reverse packet brakes to zero; subsequent
            // packets build motion in the new direction.
            const bool reversingInput = rawDelta != 0 && previousAverage != 0
                && (rawDelta > 0) != (previousAverage > 0);
            const long averagedDelta = reversingInput ? 0 : static_cast<long>(std::lround(
                previousAverage * inputRetention + rawDelta * (1.0 - inputRetention)));
            app->averagedMouseDelta = averagedDelta;
            const bool mouseIdle = app->lastMovementTick.load() != 0
                && now - app->lastMovementTick.load() >= kMouseAverageWindowMs;
            const bool hasStopped = mouseIdle && std::abs(averagedDelta) <= kStopNoiseDelta;
            const long accumulatedDelta = hasStopped ? 0 : averagedDelta;
            // Audio remains a separate monitored input. Mouse alone controls
            // the output and session until a future input mixer combines them.
            const double mouseRpm = hasStopped ? 0.0 : std::clamp(
                static_cast<double>(app->lastMousePacketDelta.load())
                    / static_cast<double>(kMousePacketAudioThreshold) * mouseReferenceRpm(*app),
                0.0, mouseReferenceRpm(*app));
            const bool loudnessFresh = app->loudnessInputEnabled.load()
                && app->loudnessLastTurnTick.load() != 0
                && now - app->loudnessLastTurnTick.load() <= 1500
                && std::abs(app->loudnessRpm.load()) > 0.0;
            const long loudnessInputPercent = loudnessFresh ? static_cast<long>(std::lround(
                std::clamp(std::abs(app->loudnessRpm.load()) * 100.0 / app->mapRpm.load(), 0.0, 100.0)))
                * app->loudnessLastDirection.load() : 0;
            app->sessionUsingLoudness = loudnessFresh;
            app->sessionCurrentRpm = loudnessFresh ? std::abs(app->loudnessRpm.load())
                : mouseRpm;
            if (app->sessionLastTick != 0) {
                const double seconds = static_cast<double>(now - app->sessionLastTick) / 1000.0;
                double travelledMetres = 0.0;
                if (loudnessFresh) {
                    const double distanceMetres = app->loudnessWheelRadius.load() / 1000.0;
                    travelledMetres = app->sessionCurrentRpm * distanceMetres / 60.0 * seconds;
                } else {
                    const long rotationDelta = app->mouseRotationDelta.load();
                    travelledMetres = rotationDelta > 0
                        ? std::abs(rawInputDelta) * mouseDistancePerRotationMetres(*app) / rotationDelta
                        : 0.0;
                }
                app->sessionDistanceMetres += travelledMetres;
                app->sessionSpeedWindowMetres += travelledMetres;
                if (app->sessionSpeedWindowStartedTick == 0) app->sessionSpeedWindowStartedTick = now;
                const double windowSeconds = static_cast<double>(now - app->sessionSpeedWindowStartedTick) / 1000.0;
                if (windowSeconds >= 1.0) {
                    app->sessionCurrentSpeedKmh = app->sessionSpeedWindowMetres / windowSeconds * 3.6;
                    app->sessionSpeedWindowMetres = 0.0;
                    app->sessionSpeedWindowStartedTick = now;
                }
            }
            app->sessionLastTick = now;
            const VJoyAxis outputAxis = selectedAxis(app->targetAxis);
            if (accumulatedDelta != 0) {
                const long mouseTarget = std::clamp(VJoyOutput::centrePosition()
                    + accumulatedDelta * app->strength.load() * kAxisUnitsPerStrengthStep,
                    VJoyOutput::minimum(), VJoyOutput::maximum());
                app->mouseTargetPosition = mouseTarget;
                app->lastDelta = accumulatedDelta;
            } else {
                const long mouseTarget = app->mouseTargetPosition.load();
                app->mouseTargetPosition = VJoyOutput::centrePosition() + static_cast<long>(std::lround(
                    (mouseTarget - VJoyOutput::centrePosition()) * 0.60));
            }
            // The red dashboard marker is a direct RPM preview. Keep it
            // independent from the output hand-off, which additionally needs
            // a saturated mouse packet to take control of vJoy.
            if (audioFresh) {
                const double fraction = std::clamp(measuredRpm / app->mapRpm.load(), 0.0, 1.0);
                app->audioTargetPosition = VJoyOutput::centrePosition() + static_cast<long>(std::lround(
                    fraction * (VJoyOutput::maximum() - VJoyOutput::centrePosition()))) * app->lastDirection.load();
            } else {
                app->audioTargetPosition = VJoyOutput::centrePosition();
            }
            const long mouseInputPosition = app->mouseInputEnabled.load()
                ? app->mouseTargetPosition.load() : VJoyOutput::centrePosition();
            setMousePreviewAxis(*app, outputAxis, mouseInputPosition);
            setAudioPreviewAxis(*app, outputAxis, app->audioTargetPosition.load());
            const auto sourcePercent = [](long position) {
                const double range = static_cast<double>(VJoyOutput::maximum() - VJoyOutput::centrePosition());
                return static_cast<long>(std::lround((position - VJoyOutput::centrePosition()) * 100.0 / range));
            };
            const auto mapSource = [](long percent, long minimum, long maximum) {
                if (percent == 0) return 0L;
                const long magnitude = static_cast<long>(std::lround(minimum
                    + (maximum - minimum) * std::abs(percent) / 100.0));
                return percent < 0 ? -magnitude : magnitude;
            };
            long combinedTotal = 0;
            int combinedCount = 0;
            int combinedPriority = 0;
            const auto combine = [&combinedTotal, &combinedCount, &combinedPriority, &mapSource](long percent, long minimum, long maximum, int priority) {
                if (percent == 0 || priority == 0 || priority < combinedPriority) return;
                const long mappedPercent = mapSource(percent, minimum, maximum);
                if (priority > combinedPriority) {
                    combinedTotal = mappedPercent;
                    combinedCount = 1;
                    combinedPriority = priority;
                } else {
                    combinedTotal += mappedPercent;
                    ++combinedCount;
                }
            };
            combine(sourcePercent(mouseInputPosition), app->mouseMixMinimum.load(), app->mouseMixMaximum.load(), app->mouseMixPriority.load());
            combine(sourcePercent(app->audioTargetPosition.load()), app->audioMixMinimum.load(), app->audioMixMaximum.load(), app->audioMixPriority.load());
            combine(loudnessInputPercent, app->loudnessMixMinimum.load(), app->loudnessMixMaximum.load(), app->loudnessMixPriority.load());
            const long combinedPercent = combinedCount == 0 ? 0 : static_cast<long>(std::lround(
                static_cast<double>(combinedTotal) / combinedCount));
            const long combinedTarget = VJoyOutput::centrePosition() + static_cast<long>(std::lround(
                combinedPercent * (VJoyOutput::maximum() - VJoyOutput::centrePosition()) / 100.0));
            if (combinedPercent == 0 && hasStopped && !audioFresh && !loudnessFresh) {
                app->pendingDelta = 0;
                app->rawMouseDelta = 0;
                app->averagedMouseDelta = 0;
                app->targetPosition = VJoyOutput::centrePosition();
                app->axisPosition = VJoyOutput::centrePosition();
                app->mouseTargetPosition = VJoyOutput::centrePosition();
                setMousePreviewAxis(*app, outputAxis, VJoyOutput::centrePosition());
                setPreviewAxis(*app, outputAxis, VJoyOutput::centrePosition());
                std::scoped_lock lock(app->outputMutex);
                if (app->vjoyEnabled.load() && app->vjoy.active()) app->vjoy.centre(outputAxis);
            } else {
                app->targetPosition = combinedTarget;
            }
            const long oldPosition = app->axisPosition.load();
            const long target = app->targetPosition.load();
            if (oldPosition != VJoyOutput::centrePosition() || target != VJoyOutput::centrePosition()) {
                const double smoothing = (static_cast<double>(app->smoothing.load()) - 1.0) / 199.0;
                const double interpolation = 0.35 - smoothing * 0.315;
                const double targetReturnToCentre = 0.50 + smoothing * 0.48;
                long position = oldPosition + static_cast<long>(std::lround((target - oldPosition) * interpolation));
                if (position == oldPosition && target != oldPosition) position += target > oldPosition ? 1 : -1;
                long nextTarget = VJoyOutput::centrePosition()
                    + static_cast<long>(std::lround((target - VJoyOutput::centrePosition()) * targetReturnToCentre));
                if (std::abs(nextTarget - VJoyOutput::centrePosition()) < 40) nextTarget = VJoyOutput::centrePosition();
                if (std::abs(position - VJoyOutput::centrePosition()) < 2 && nextTarget == VJoyOutput::centrePosition()) {
                    position = VJoyOutput::centrePosition();
                }
                app->axisPosition = position;
                app->targetPosition = nextTarget;
                setPreviewAxis(*app, outputAxis, position);
                std::scoped_lock lock(app->outputMutex);
                if (app->vjoyEnabled.load() && app->vjoy.active()) app->vjoy.setAxis(outputAxis, position);
            }
            if (app->xboxEnabled.load()) {
                if (!app->xbox.active() && now - app->lastXboxInitializeTick >= 2000) {
                    app->lastXboxInitializeTick = now;
                    app->xbox.initialize();
                }
                if (app->xbox.active()) {
                    const long scaledPosition = VJoyOutput::centrePosition() + static_cast<long>(std::lround(
                        (app->axisPosition.load() - VJoyOutput::centrePosition()) * app->xboxStrength.load() / 100.0));
                    app->xbox.setAxis(app->xboxAxis.load(), scaledPosition, app->xboxTriggersFlipped.load());
                    const bool leftHeld = keyIsHeld(app->xboxXLeftKey.load());
                    const bool rightHeld = keyIsHeld(app->xboxXRightKey.load());
                    const long xPosition = leftHeld == rightHeld ? VJoyOutput::centrePosition()
                        : (leftHeld ? VJoyOutput::minimum() : VJoyOutput::maximum());
                    app->xbox.setAxis(0, xPosition);
                }
            }
            const long clampedKeyboardPercent = keyboardPercentForMouseDelta(*app, accumulatedDelta);
            app->lastKeyboardPercent = clampedKeyboardPercent;
            const bool mappingsEnabled = app->keyboardMappingsEnabled.load();
            const bool allowKeyboardInjection = GetForegroundWindow()
                != (app->dashboardWindow ? app->dashboardWindow : app->window) && mappingsEnabled;
            triggerThresholdMappings(*app, clampedKeyboardPercent, allowKeyboardInjection);
            const bool anyKeyHeld = std::any_of(app->mappings.begin(), app->mappings.end(),
                [](const App::KeyMapping& mapping) { return mapping.active; });
            const bool toggleMappingHeld = std::any_of(app->mappings.begin(), app->mappings.end(),
                [](const App::KeyMapping& mapping) {
                    return mapping.active && (mapping.modifierToggle || mapping.keyToggle || mapping.modifier == 7);
                });
            // Periodically lift held keys for a short refresh interval. This
            // guards against games losing a very long synthetic key-down.
            const bool refreshKeys = allowKeyboardInjection && anyKeyHeld && !toggleMappingHeld
                && now - app->lastKeyboardRefreshTick >= app->keyboardRepeatIntervalMs.load();
            if (refreshKeys) {
                app->lastKeyboardRefreshTick = now;
                updateKeyboardMappings(*app, clampedKeyboardPercent, false);
                app->keyboardRefreshInProgress = true;
                SetTimer(window, kKeyboardRefreshTimer, app->keyboardRepeatPauseMs.load(), nullptr);
            } else if (!app->keyboardRefreshInProgress) {
                updateKeyboardMappings(*app, clampedKeyboardPercent, allowKeyboardInjection);
            }
            if (!anyKeyHeld) app->lastKeyboardRefreshTick = now;
            app->previousKeyboardPercent = clampedKeyboardPercent;
            std::wstring keyboardText = mappingsEnabled ? keyboardOutputText(*app, clampedKeyboardPercent)
                                                        : L"Keyboard output: disabled";
            if (mappingsEnabled && !allowKeyboardInjection && keyboardText != L"Keyboard output: --") {
                keyboardText += L" (inactive while this app is focused)";
            }
            SetWindowTextW(app->keyboardPreview, keyboardText.c_str());
            const ULONGLONG tick = GetTickCount64();
            if ((accumulatedDelta != 0 || oldPosition != VJoyOutput::centrePosition())
                && tick - app->lastLogTick >= 100) {
                app->lastLogTick = tick;
                writeLog("input_total=" + std::to_string(accumulatedDelta)
                    + " strength=" + std::to_string(app->strength.load())
                    + " target=" + std::to_string(app->targetPosition.load())
                    + " output=" + std::to_string(app->axisPosition.load()));
            }
            const wchar_t* source = app->sourceAxis.load() == 0 ? L"X" : L"Y";
            const wchar_t* targetName = app->targetAxis.load() == 0 ? L"X" : L"Y";
            setStatus(*app, L"Suppressed bike-mouse " + std::wstring(source) + L" delta "
                       + std::to_wstring(app->lastDelta.load()) + L" -> vJoy " + targetName
                       + L"   " + app->vjoy.status());
            InvalidateRect(app->preview, nullptr, FALSE);
            if (app->dashboard && now - app->lastDashboardSync >= 100) {
                app->lastDashboardSync = now;
                app->dashboard->sync();
            }
        }
        if (app) updateStatsOverlay(*app);
        return 0;
    case WM_COMMAND:
        if (!app) break;
        if (LOWORD(wParam) == kIdRefresh && HIWORD(wParam) == BN_CLICKED) refreshMice(*app);
        if (LOWORD(wParam) == kIdMouseCombo && HIWORD(wParam) == CBN_SELCHANGE) {
            const int index = static_cast<int>(SendMessageW(app->mouseCombo, CB_GETCURSEL, 0, 0));
            app->interception.selectMouse(app->mouseInputEnabled.load() && index >= 0 && index < static_cast<int>(app->mice.size()) ? app->mice[index].id : 0);
            app->selectedHardwareId = index >= 0 && index < static_cast<int>(app->mice.size())
                ? app->mice[index].hardwareId : L"";
            saveSettings(*app);
            writeLog("selected interception mouse device " + std::to_string(
                index >= 0 && index < static_cast<int>(app->mice.size()) ? app->mice[index].id : 0));
            app->lastDelta = 0;
            setStatus(*app, L"Bike mouse selected and suppressed.");
        }
        if (LOWORD(wParam) == kIdSourceCombo && HIWORD(wParam) == CBN_SELCHANGE) {
            app->sourceAxis = SendMessageW(app->sourceCombo, CB_GETCURSEL, 0, 0) == 0 ? 0 : 1;
            saveSettings(*app);
            writeLog("source axis changed");
        }
        if (LOWORD(wParam) == kIdTargetCombo && HIWORD(wParam) == CBN_SELCHANGE) {
            const VJoyAxis oldAxis = selectedAxis(app->targetAxis);
            app->targetAxis = SendMessageW(app->targetCombo, CB_GETCURSEL, 0, 0) == 0 ? 0 : 1;
            app->axisPosition = VJoyOutput::centrePosition();
            app->targetPosition = VJoyOutput::centrePosition();
            app->mouseTargetPosition = VJoyOutput::centrePosition();
            app->audioTargetPosition = VJoyOutput::centrePosition();
            app->xPosition = app->yPosition = VJoyOutput::centrePosition();
            app->mouseXPosition = app->mouseYPosition = VJoyOutput::centrePosition();
            app->audioXPosition = app->audioYPosition = VJoyOutput::centrePosition();
            setPreviewAxis(*app, oldAxis, VJoyOutput::centrePosition());
            std::scoped_lock lock(app->outputMutex);
            app->vjoy.centre(oldAxis);
            saveSettings(*app);
            writeLog("target vJoy axis changed");
        }
        if (LOWORD(wParam) == kIdReverse && HIWORD(wParam) == BN_CLICKED) {
            app->reverseAxis = SendMessageW(app->reverseCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            saveSettings(*app);
            writeLog(std::string("reverse axis=") + (app->reverseAxis.load() ? "true" : "false"));
        }
        if (LOWORD(wParam) == kIdKeyboardMappingsEnabled && HIWORD(wParam) == BN_CLICKED) {
            app->keyboardMappingsEnabled = SendMessageW(app->keyboardMappingsCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            if (!app->keyboardMappingsEnabled.load()) releaseKeyboardMappings(*app);
            saveSettings(*app);
        }
        if (LOWORD(wParam) == kIdAudioCadence && HIWORD(wParam) == BN_CLICKED) {
            const bool enabled = SendMessageW(app->audioCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            app->audioCadenceEnabled = enabled;
            if (enabled && !app->audioCadence.start()) {
                app->audioCadenceEnabled = false;
                SendMessageW(app->audioCheck, BM_SETCHECK, BST_UNCHECKED, 0);
            }
            if (!enabled) app->audioCadence.stop();
            saveSettings(*app);
        }
        if (LOWORD(wParam) == kIdAudioBeep && HIWORD(wParam) == BN_CLICKED) {
            app->audioBeepEnabled = SendMessageW(app->audioBeepCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            saveSettings(*app);
        }
        if ((LOWORD(wParam) == kIdWheelRadius && HIWORD(wParam) == EN_CHANGE)
            || (LOWORD(wParam) == kIdWheelUnit && HIWORD(wParam) == CBN_SELCHANGE)) {
            readWheelRadius(*app);
            saveSettings(*app);
        }
        if (LOWORD(wParam) == kIdMapRpm && HIWORD(wParam) == EN_CHANGE) {
            readMapRpm(*app);
            saveSettings(*app);
        }
        if (LOWORD(wParam) == kIdMappingAdd && HIWORD(wParam) == BN_CLICKED) {
            addMapping(*app);
            return 0;
        }
        if (!app->buildingMappingControls
            && LOWORD(wParam) >= kIdMappingBase && LOWORD(wParam) < kIdMappingBase + app->mappings.size() * 5
            && (HIWORD(wParam) == EN_CHANGE || HIWORD(wParam) == CBN_SELCHANGE)) {
            const size_t index = (LOWORD(wParam) - kIdMappingBase) / 5;
            releaseKeyboardMappings(*app);
            readMappingControls(*app, index);
            // Read the selection from the control that raised the event. This
            // avoids losing a modifier selection while adjacent edit controls
            // are also committing their text.
            if ((LOWORD(wParam) - kIdMappingBase) % 5 == 2 && HIWORD(wParam) == CBN_SELCHANGE) {
                app->mappings[index].modifier = std::clamp(
                    static_cast<int>(SendMessageW(reinterpret_cast<HWND>(lParam), CB_GETCURSEL, 0, 0)), 0, 7);
            }
            saveSettings(*app);
        }
        if (LOWORD(wParam) >= kIdMappingBase && LOWORD(wParam) < kIdMappingBase + app->mappings.size() * 5
            && (LOWORD(wParam) - kIdMappingBase) % 5 == 4 && HIWORD(wParam) == BN_CLICKED) {
            const size_t index = (LOWORD(wParam) - kIdMappingBase) / 5;
            releaseKeyboardMappings(*app);
            app->mappings.erase(app->mappings.begin() + static_cast<std::ptrdiff_t>(index));
            relayoutMappingRows(*app);
            saveSettings(*app);
        }
        return 0;
    case WM_HSCROLL:
        if (app && reinterpret_cast<HWND>(lParam) == app->strengthSlider) {
            app->strength = static_cast<long>(SendMessageW(app->strengthSlider, TBM_GETPOS, 0, 0));
            updateStrengthLabel(*app);
            saveSettings(*app);
            writeLog("strength=" + std::to_string(app->strength.load()));
        }
        if (app && reinterpret_cast<HWND>(lParam) == app->smoothingSlider) {
            app->smoothing = static_cast<long>(SendMessageW(app->smoothingSlider, TBM_GETPOS, 0, 0));
            updateSmoothingLabel(*app);
            saveSettings(*app);
            writeLog("smoothing=" + std::to_string(app->smoothing.load()));
        }
        return 0;
    case WM_DESTROY:
        KillTimer(window, kDecayTimer);
        KillTimer(window, kKeyboardRefreshTimer);
        if (app) {
            // Release active modifiers before reading the legacy controls.
            // The dashboard owns the visible mappings, so stale hidden
            // controls must never leave a toggle modifier such as Caps Lock
            // engaged during shutdown.
            releaseKeyboardMappings(*app);
            for (size_t index = 0; index < app->mappings.size(); ++index) readMappingControls(*app, index);
            readWheelRadius(*app);
            readMapRpm(*app);
            saveSettings(*app);
            app->audioCadence.stop();
            if (app->statsOverlayWindow) DestroyWindow(app->statsOverlayWindow);
        }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

LRESULT CALLBACK previewProc(HWND window, UINT message, WPARAM, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        SetWindowLongPtrW(window, GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams));
        return TRUE;
    }
    if (message == WM_PAINT) {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(window, &paint);
        RECT rect{};
        GetClientRect(window, &rect);
        FillRect(dc, &rect, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
        Rectangle(dc, rect.left, rect.top, rect.right, rect.bottom);
        const int centreX = (rect.left + rect.right) / 2;
        const int centreY = (rect.top + rect.bottom) / 2;
        MoveToEx(dc, centreX, rect.top, nullptr); LineTo(dc, centreX, rect.bottom);
        MoveToEx(dc, rect.left, centreY, nullptr); LineTo(dc, rect.right, centreY);
        if (const auto* app = reinterpret_cast<App*>(GetWindowLongPtrW(window, GWLP_USERDATA))) {
            const double range = static_cast<double>(VJoyOutput::maximum() - VJoyOutput::minimum());
            const auto pointFor = [&rect, range](long xValue, long yValue) {
                const double xFraction = (xValue - VJoyOutput::minimum()) / range;
                const double yFraction = (yValue - VJoyOutput::minimum()) / range;
                return POINT{
                    rect.left + static_cast<int>(std::lround(xFraction * (rect.right - rect.left))),
                    rect.bottom - static_cast<int>(std::lround(yFraction * (rect.bottom - rect.top)))
                };
            };
            const POINT mouse = pointFor(app->mouseXPosition.load(), app->mouseYPosition.load());
            const POINT audio = pointFor(app->audioXPosition.load(), app->audioYPosition.load());
            const POINT combined = pointFor(app->xPosition.load(), app->yPosition.load());
            const auto drawRing = [dc](POINT point, COLORREF colour, int radius) {
                HPEN pen = CreatePen(PS_SOLID, 2, colour);
                HGDIOBJ oldPen = SelectObject(dc, pen);
                HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
                Ellipse(dc, point.x - radius, point.y - radius, point.x + radius, point.y + radius);
                SelectObject(dc, oldBrush);
                SelectObject(dc, oldPen);
                DeleteObject(pen);
            };
            drawRing(mouse, RGB(20, 100, 255), 7);   // Mouse source
            drawRing(audio, RGB(230, 40, 40), 10);   // Audio RPM source
            HBRUSH green = CreateSolidBrush(RGB(0, 180, 70));
            HGDIOBJ oldBrush = SelectObject(dc, green);
            Ellipse(dc, combined.x - 5, combined.y - 5, combined.x + 5, combined.y + 5);
            SelectObject(dc, oldBrush);
            DeleteObject(green);
        }
        EndPaint(window, &paint);
        return 0;
    }
    return DefWindowProcW(window, message, 0, lParam);
}

HWND makeLabel(HINSTANCE instance, HWND parent, const wchar_t* text, int x, int y, int width) {
    return CreateWindowW(L"STATIC", text, WS_CHILD | WS_VISIBLE, x, y, width, 20, parent, nullptr, instance, nullptr);
}

HWND makeCombo(HINSTANCE instance, HWND parent, int id, int x, int y, int width) {
    return CreateWindowW(L"COMBOBOX", nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
                         x, y, width, 300, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance, nullptr);
}

void destroyMappingControls(App::KeyMapping& mapping) {
    for (HWND control : {mapping.minimumEdit, mapping.maximumEdit, mapping.modifierCombo, mapping.keyEdit, mapping.removeButton}) {
        if (control) DestroyWindow(control);
    }
    mapping.minimumEdit = mapping.maximumEdit = mapping.modifierCombo = mapping.keyEdit = mapping.removeButton = nullptr;
}

void relayoutMappingRows(App& app) {
    app.buildingMappingControls = true;
    for (auto& mapping : app.mappings) destroyMappingControls(mapping);
    const HINSTANCE instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(app.window, GWLP_HINSTANCE));
    for (size_t index = 0; index < app.mappings.size(); ++index) {
        auto& mapping = app.mappings[index];
        const int y = 335 + static_cast<int>(index) * 30;
        const UINT base = kIdMappingBase + static_cast<UINT>(index) * 5;
        mapping.minimumEdit = CreateWindowW(L"EDIT", std::to_wstring(mapping.minimum).c_str(),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, 22, y, 68, 23, app.window,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(base)), instance, nullptr);
        mapping.maximumEdit = CreateWindowW(L"EDIT", std::to_wstring(mapping.maximum).c_str(),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, 102, y, 68, 23, app.window,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(base + 1)), instance, nullptr);
        mapping.modifierCombo = makeCombo(instance, app.window, base + 2, 182, y - 2, 155);
        for (int modifier = 0; modifier <= 7; ++modifier) {
            SendMessageW(mapping.modifierCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(modifierLabel(modifier)));
        }
        SendMessageW(mapping.modifierCombo, CB_SETCURSEL, mapping.modifier, 0);
        wchar_t key[2]{mapping.key, L'\0'};
        mapping.keyEdit = CreateWindowW(L"EDIT", key, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
            350, y, 56, 23, app.window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(base + 3)), instance, nullptr);
        mapping.removeButton = CreateWindowW(L"BUTTON", L"Remove", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            418, y - 1, 75, 25, app.window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(base + 4)), instance, nullptr);
    }
    const int statusY = 345 + static_cast<int>(app.mappings.size()) * 30;
    SetWindowPos(app.status, nullptr, 22, statusY, 630, 42, SWP_NOZORDER | SWP_NOACTIVATE);
    const int windowHeight = std::max(440, statusY + 95);
    SetWindowPos(app.window, nullptr, 0, 0, 900, windowHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    app.buildingMappingControls = false;
}

void addMapping(App& app) {
    // Control IDs are 16-bit on Win32; this still allows far more mappings than
    // can sensibly fit in a single settings screen.
    if (app.mappings.size() >= 1000) {
        setStatus(app, L"Maximum number of keyboard mappings reached.");
        return;
    }
    app.mappings.push_back({0, 100, 0, L'W'});
    relayoutMappingRows(app);
    saveSettings(app);
}
}

