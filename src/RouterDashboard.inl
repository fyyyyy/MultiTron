class RouterDashboard final : public ultralight::WindowListener, public ultralight::LoadListener {
public:
    explicit RouterDashboard(App& app) : app_(app) {
        writeLog("dashboard: creating AppCore runtime");
        // The router should start on systems without a usable D3D renderer as
        // well (for example Remote Desktop and older integrated GPUs).
        ultralight::Settings settings;
        settings.force_cpu_renderer = true;
        wchar_t executablePath[MAX_PATH]{};
        GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
        uiAssets_ = std::filesystem::path(executablePath).parent_path() / "assets";
        wchar_t developmentAssets[MAX_PATH]{};
        const DWORD developmentAssetsLength = GetEnvironmentVariableW(
            L"MOUSE_VJOY_ROUTER_UI_PATH", developmentAssets, MAX_PATH);
        if (developmentAssetsLength > 0 && developmentAssetsLength < MAX_PATH
            && std::filesystem::exists(std::filesystem::path(developmentAssets) / "index.html")) {
            developmentUiSource_ = developmentAssets;
            developmentCopyRoot_ = uiAssets_.parent_path() / "hot-ui";
            uiAssets_ = makeDevelopmentAssetCopy();
            watchStylesheets_ = true;
            writeLog("dashboard: development stylesheet reload enabled (shadow assets)");
        }
        settings.file_system_path = "./";
        appCore_ = ultralight::App::Create(settings);
        writeLog("dashboard: creating native window");
        window_ = ultralight::Window::Create(appCore_->main_monitor(), 1240, 890, false,
            ultralight::kWindowFlags_Titled | ultralight::kWindowFlags_Resizable);
        writeLog("dashboard: creating overlay");
        window_->SetTitle("MultiTron2");
        window_->set_listener(this);
        overlay_ = ultralight::Overlay::Create(window_, window_->width(), window_->height(), 0, 0);
        // Native dropdowns require the overlay to have input focus before their
        // first click.  Without this, the first click only focuses the view.
        overlay_->Focus();
        overlay_->view()->set_load_listener(this);
        const std::string uiUrl = "file:///" + (uiAssets_ / "index.html").generic_string();
        overlay_->view()->LoadURL(uiUrl.c_str());
        if (watchStylesheets_) startStylesheetWatcher();
        writeLog("dashboard: UI files requested");
        app_.dashboardWindow = static_cast<HWND>(window_->native_handle());
    }

    void setCommandHandler(std::function<void(const std::string&, const std::string&)> handler) {
        commandHandler_ = std::move(handler);
    }

    void sync() {
        if (stylesheetReloadRequested_.exchange(false)) {
            ready_ = false;
            loggedFirstSync_ = false;
            writeLog("dashboard: reloading after stylesheet change");
            uiAssets_ = makeDevelopmentAssetCopy();
            const std::string uiUrl = "file:///" + (uiAssets_ / "index.html").generic_string();
            overlay_->view()->LoadURL(uiUrl.c_str());
            return;
        }
        if (!ready_) return;
        const ULONGLONG now = GetTickCount64();
        const bool loudnessEnabled = app_.loudnessInputEnabled.load();
        if (loudnessEnabled && now - app_.lastLoudnessDeviceCheck >= 1000) {
            app_.lastLoudnessDeviceCheck = now;
            app_.loudnessConnected = loudnessControllerIsConnected();
        } else if (!loudnessEnabled) {
            app_.loudnessConnected = false;
        }
        if (!loggedFirstSync_) {
            writeLog("dashboard: publishing first live state");
            loggedFirstSync_ = true;
        }
        if (lastMappingRevision_ != app_.mappingRevision) publishMappings();
        const double centre = static_cast<double>(VJoyOutput::centrePosition());
        const double range = static_cast<double>(VJoyOutput::maximum() - VJoyOutput::centrePosition());
        const auto axis = app_.targetAxis.load() == 0 ? app_.xPosition.load() : app_.yPosition.load();
        const bool mouseEnabled = app_.mouseInputEnabled.load();
        const auto mouse = mouseEnabled ? (app_.targetAxis.load() == 0 ? app_.mouseXPosition.load() : app_.mouseYPosition.load())
                                        : VJoyOutput::centrePosition();
        const auto audio = app_.targetAxis.load() == 0 ? app_.audioXPosition.load() : app_.audioYPosition.load();
        const double speedKmh = app_.sessionCurrentSpeedKmh.load();
        const double elapsedHours = app_.sessionStartedTick == 0 ? 0.0
            : static_cast<double>(GetTickCount64() - app_.sessionStartedTick) / 3600000.0;
        const double averageKmh = elapsedHours > 0.0 ? app_.sessionDistanceMetres / 1000.0 / elapsedHours : 0.0;
        const auto percent = [](long value, double centreValue, double rangeValue) {
            return (std::max)(-100.0, (std::min)(100.0, (value - centreValue) * 100.0 / rangeValue));
        };
        const long xboxAxis = VJoyOutput::centrePosition() + static_cast<long>(std::lround(
            (axis - VJoyOutput::centrePosition()) * app_.xboxStrength.load() / 100.0));
        const long vjoyUnscaledAxis = VJoyOutput::centrePosition() + static_cast<long>(std::lround(
            static_cast<double>(axis - VJoyOutput::centrePosition()) / app_.strength.load()));
        const double loudnessOutput = loudnessEnabled && app_.sessionUsingLoudness.load()
            ? std::clamp(app_.loudnessRpm.load() * 100.0 / app_.mapRpm.load(), 0.0, 100.0)
                * app_.loudnessLastDirection.load()
            : 0.0;
        const bool audioEnabled = app_.audioCadenceEnabled.load();
        const std::string script = "window.RouterState({output:" + std::to_string(percent(axis, centre, range))
            + ",combined:" + std::to_string(percent(axis, centre, range))
            + ",vjoyOutput:" + std::to_string(percent(axis, centre, range))
            + ",vjoyUnscaledOutput:" + std::to_string(percent(vjoyUnscaledAxis, centre, range))
            + ",xboxOutput:" + std::to_string(percent(xboxAxis, centre, range))
            + ",xboxUnscaledOutput:" + std::to_string(percent(axis, centre, range))
            + ",mouse:" + std::to_string(percent(mouse, centre, range))
            + ",mouseRaw:" + std::to_string(mouseEnabled ? app_.rawMouseDelta.load() : 0)
            + ",mouseSmoothed:" + std::to_string(mouseEnabled ? app_.averagedMouseDelta.load() : 0)
            + ",audio:" + std::to_string(percent(audio, centre, range))
            + ",loudness:" + std::to_string(loudnessOutput)
            + ",targetAxis:" + std::to_string(app_.targetAxis.load())
            + ",audioRpm:" + std::to_string(audioEnabled ? measuredAudioRpm(app_) : 0.0)
            + ",audioLevel:" + std::to_string(audioEnabled ? app_.audioCadence.peakAmplitude() : 0)
            + ",audioPulseSequence:" + std::to_string(audioEnabled ? app_.audioCadence.pulseSequence() : 0)
            + ",loudnessDirection:" + std::to_string(loudnessEnabled ? app_.loudnessLastDirection.load() : 0)
            + ",loudnessTurns:" + std::to_string(loudnessEnabled ? app_.loudnessTurnSequence.load() : 0)
            + ",loudnessRpm:" + std::to_string(loudnessEnabled ? app_.loudnessRpm.load() : 0.0)
            + ",loudnessRawRpm:" + std::to_string(loudnessEnabled ? app_.loudnessRawRpm.load() : 0.0)
            + ",loudnessMaximumRpm:" + std::to_string(loudnessEnabled ? app_.loudnessMaximumRpm.load() : 0.0)
            + ",loudnessActive:" + (loudnessEnabled && now - app_.loudnessLastTurnTick.load() < 400 ? "true" : "false")
            + ",loudnessConnected:" + (loudnessEnabled && app_.loudnessConnected.load() ? "true" : "false")
            + ",speedKmh:" + std::to_string(speedKmh)
            + ",averageKmh:" + std::to_string(averageKmh)
            + ",distanceKm:" + std::to_string(app_.sessionDistanceMetres / 1000.0)
            + ",distanceMiles:" + std::to_string(app_.sessionDistanceMetres / 1609.344)
            + ",totalDistanceKm:" + std::to_string((app_.totalDistanceMetres + app_.sessionDistanceMetres) / 1000.0)
            + ",totalDistanceMiles:" + std::to_string((app_.totalDistanceMetres + app_.sessionDistanceMetres) / 1609.344)
            + ",vjoy:" + (app_.vjoyEnabled.load() ? "true" : "false")
            + ",keyboard:" + (app_.keyboardMappingsEnabled.load() ? "true" : "false")
            + ",xbox:" + (app_.xbox.active() ? "true" : "false")
            + ",xboxStatus:" + quote(app_.xbox.active() ? "Xbox 360 controller active"
                : (app_.xboxEnabled.load() ? "Xbox unavailable (install ViGEmBus)" : "Disabled"))
            + ",status:" + quote("vJoy routing active") + "});";
        evaluate(script);
        evaluate("window.RouterMouseRotationMeasurement(" + std::to_string(app_.mouseRotationMeasurementDelta.load()) + ");");
    }

    void Run() { appCore_->Run(); }
    void Quit() { appCore_->Quit(); }

    void publishMice() {
        std::string devices{"["};
        for (size_t index = 0; index < app_.mice.size(); ++index) {
            if (index) devices += ',';
            const auto& mouse = app_.mice[index];
            devices += "{id:" + std::to_string(mouse.id)
                + ",label:" + quote("Mouse " + std::to_string(index + 1)
                    + " (device " + std::to_string(mouse.id) + ")") + "}";
        }
        devices += ']';
        int selectedId = 0;
        const auto selected = std::find_if(app_.mice.begin(), app_.mice.end(), [this](const auto& mouse) {
            return !app_.selectedHardwareId.empty() && mouse.hardwareId == app_.selectedHardwareId;
        });
        if (selected != app_.mice.end()) selectedId = selected->id;
        evaluate("window.RouterMice(" + devices + "," + std::to_string(selectedId) + ");");
    }

    void publishConfiguration() {
        if (!ready_) return;
        evaluate(configurationScript());
        evaluate("window.RouterInputUpdateInterval(" + std::to_string(app_.inputUpdateIntervalMs.load()) + ");");
        publishMappings();
        publishMice();
    }

    void OnDOMReady(ultralight::View* view, uint64_t, bool, const ultralight::String&) override {
        writeLog("dashboard: DOM ready");
        // Loading a document can clear the focus assigned while the overlay was
        // being created. Restore it once controls exist so the first select
        // click opens the list instead of merely focusing the dashboard.
        focusDashboard();
        auto context = view->LockJSContext();
        ultralight::SetJSContext(context->ctx());
        ultralight::JSCallback callback = std::bind(&RouterDashboard::onAction, this,
            std::placeholders::_1, std::placeholders::_2);
        ultralight::JSGlobalObject()["NativeRouterAction"] = callback;
        view->EvaluateScript(configurationScript().c_str());
        view->EvaluateScript(("window.RouterInputUpdateInterval(" + std::to_string(app_.inputUpdateIntervalMs.load()) + ");").c_str());
        ready_ = true;
        publishMice();
    }
    void OnClose(ultralight::Window*) override {
        if (app_.window) DestroyWindow(app_.window);
        appCore_->Quit();
    }

    void OnResize(ultralight::Window*, uint32_t width, uint32_t height) override {
        overlay_->Resize(width, height);
    }

    bool OnMouseEvent(const ultralight::MouseEvent&) override {
        // The native window can regain activation before its embedded view.
        // Restore both levels of focus before forwarding the first click so
        // selects open immediately instead of consuming that click as focus.
        focusDashboard();
        return true;
    }

private:
    std::string configurationScript() const {
        return "window.RouterConfig({source:" + std::to_string(app_.sourceAxis.load())
            + ",target:" + std::to_string(app_.targetAxis.load())
            + ",vjoyStrength:" + std::to_string(app_.strength.load())
            + ",vjoy:" + (app_.vjoyEnabled.load() ? "true" : "false")
            + ",mouse:" + (app_.mouseInputEnabled.load() ? "true" : "false")
            + ",loudness:" + (app_.loudnessInputEnabled.load() ? "true" : "false")
            + ",loudnessReverse:" + (app_.loudnessReverse.load() ? "true" : "false")
            + ",loudnessClicks:" + std::to_string(app_.loudnessClicksPerRotation.load())
            + ",loudnessSmoothing:" + std::to_string(app_.loudnessRpmSmoothing.load())
            + ",loudnessRadius:" + std::to_string(app_.loudnessWheelRadius.load())
            + ",xboxStrength:" + std::to_string(app_.xboxStrength.load())
            + ",smoothing:" + std::to_string(app_.smoothing.load())
            + ",inputUpdateInterval:" + std::to_string(app_.inputUpdateIntervalMs.load())
            + ",mouseRotationDelta:" + std::to_string(app_.mouseRotationDelta.load())
            + ",mouseDistancePerRotation:" + std::to_string(app_.mouseDistancePerRotation.load())
            + ",mouseDistanceUnit:" + std::to_string(app_.mouseDistanceUnit.load())
            + ",mouseMixMinimum:" + std::to_string(app_.mouseMixMinimum.load())
            + ",mouseMixMaximum:" + std::to_string(app_.mouseMixMaximum.load())
            + ",mouseMixPriority:" + std::to_string(app_.mouseMixPriority.load())
            + ",audioMixMinimum:" + std::to_string(app_.audioMixMinimum.load())
            + ",audioMixMaximum:" + std::to_string(app_.audioMixMaximum.load())
            + ",audioMixPriority:" + std::to_string(app_.audioMixPriority.load())
            + ",loudnessMixMinimum:" + std::to_string(app_.loudnessMixMinimum.load())
            + ",loudnessMixMaximum:" + std::to_string(app_.loudnessMixMaximum.load())
            + ",loudnessMixPriority:" + std::to_string(app_.loudnessMixPriority.load())
             + ",reverse:" + (app_.reverseAxis.load() ? "true" : "false")
             + ",keyboard:" + (app_.keyboardMappingsEnabled.load() ? "true" : "false")
             + ",keyboardRepeatInterval:" + std::to_string(app_.keyboardRepeatIntervalMs.load())
             + ",keyboardRepeatPause:" + std::to_string(app_.keyboardRepeatPauseMs.load())
             + ",overlay:" + (app_.statsOverlayEnabled.load() ? "true" : "false")
             + ",overlayPosition:" + std::to_string(app_.statsOverlayPosition.load())
             + ",audio:" + (app_.audioCadenceEnabled.load() ? "true" : "false")
            + ",beep:" + (app_.audioBeepEnabled.load() ? "true" : "false")
            + ",radius:" + std::to_string(app_.wheelRadius.load())
            + ",unit:" + std::to_string(app_.wheelRadiusUnit.load())
            + ",rpm:" + std::to_string(app_.mapRpm.load())
            + ",threshold:" + std::to_string(app_.audioCadence.pulseAmplitude())
            + ",xbox:" + (app_.xboxEnabled.load() ? "true" : "false")
            + ",xboxAxis:" + std::to_string(app_.xboxAxis.load())
            + ",xboxTriggersFlipped:" + (app_.xboxTriggersFlipped.load() ? "true" : "false")
            + ",xboxXLeftKey:" + quote(std::string(1, static_cast<char>(app_.xboxXLeftKey.load())))
            + ",xboxXRightKey:" + quote(std::string(1, static_cast<char>(app_.xboxXRightKey.load())))
            + ",settingsFile:" + quote(utf8Text(app_.loadedSettingsFilename)) + "});";
    }

    void focusDashboard() {
        if (!overlay_->has_focus()) overlay_->Focus();
        if (!overlay_->view()->HasFocus()) overlay_->view()->Focus();
        const auto nativeWindow = static_cast<HWND>(window_->native_handle());
        if (GetForegroundWindow() == nativeWindow && GetFocus() != nativeWindow) {
            SetFocus(nativeWindow);
        }
    }

    std::filesystem::file_time_type newestStylesheetWriteTime() const {
        std::filesystem::file_time_type newest{};
        std::error_code error;
        for (std::filesystem::recursive_directory_iterator entry(developmentUiSource_, error), end;
             !error && entry != end; entry.increment(error)) {
            if (!entry->is_regular_file(error) || entry->path().extension() != ".css") continue;
            newest = (std::max)(newest, entry->last_write_time(error));
        }
        return newest;
    }

    std::filesystem::path makeDevelopmentAssetCopy() const {
        const auto destination = developmentCopyRoot_ / std::to_string(GetTickCount64());
        std::error_code error;
        std::filesystem::create_directories(destination, error);
        std::filesystem::copy(developmentUiSource_, destination,
            std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing, error);
        if (error) writeLog("dashboard: could not copy development UI assets");
        return destination;
    }

    void startStylesheetWatcher() {
        stylesheetWatcher_ = std::jthread([this](std::stop_token stop) {
            auto newest = newestStylesheetWriteTime();
            while (!stop.stop_requested()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
                const auto current = newestStylesheetWriteTime();
                if (current > newest) {
                    newest = current;
                    stylesheetReloadRequested_ = true;
                }
            }
        });
    }

    void evaluate(const std::string& script) {
        ultralight::String exception;
        overlay_->view()->EvaluateScript(script.c_str(), &exception);
        if (!exception.empty()) writeLog("dashboard JavaScript error: " + std::string(exception.utf8().data()));
    }

    static std::string utf8Text(const std::wstring& text) {
        if (text.empty()) return "W";
        const int length = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
        std::string result(static_cast<size_t>(length), '\0');
        if (length > 0) WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), length, nullptr, nullptr);
        return result;
    }

    void publishMappings() {
        std::string rows{"["};
        for (size_t index = 0; index < app_.mappings.size(); ++index) {
            const auto& mapping = app_.mappings[index];
            if (index) rows += ',';
                rows += "{minimum:" + std::to_string(mapping.minimum)
                + ",maximum:" + std::to_string(mapping.maximum)
                + ",modifier:" + std::to_string(mapping.modifier)
                + ",key:" + quote(utf8Text(mappingKeyLabel(mapping.key)))
                + ",modifierToggle:" + (mapping.modifierToggle ? "true" : "false")
                + ",keyToggle:" + (mapping.keyToggle ? "true" : "false") + "}";
        }
        rows += ']';
        overlay_->view()->EvaluateScript(("window.RouterMappings(" + rows + ");").c_str());
        lastMappingRevision_ = app_.mappingRevision;
    }

    static std::string quote(const std::string& text) {
        std::string result{"\""};
        for (const char ch : text) {
            if (ch == '\\' || ch == '\"') result += '\\';
            if (ch == '\n' || ch == '\r') result += ' ';
            else result += ch;
        }
        return result + "\"";
    }

    void onAction(const ultralight::JSObject&, const ultralight::JSArgs& args) {
        if (!commandHandler_ || args.size() < 1) return;
        const ultralight::String commandString = args[0].ToString();
        const std::string command = commandString.utf8().data();
        const ultralight::String valueString = args.size() > 1 ? static_cast<ultralight::String>(args[1].ToString())
                                                                : ultralight::String();
        const std::string value = valueString.utf8().data();
        commandHandler_(command, value);
    }

    App& app_;
    ultralight::RefPtr<ultralight::App> appCore_;
    ultralight::RefPtr<ultralight::Window> window_;
    ultralight::RefPtr<ultralight::Overlay> overlay_;
    std::function<void(const std::string&, const std::string&)> commandHandler_;
    bool ready_ = false;
    bool loggedFirstSync_ = false;
    unsigned int lastMappingRevision_ = std::numeric_limits<unsigned int>::max();
    std::filesystem::path uiAssets_;
    std::filesystem::path developmentUiSource_;
    std::filesystem::path developmentCopyRoot_;
    bool watchStylesheets_ = false;
    std::atomic_bool stylesheetReloadRequested_ = false;
    std::jthread stylesheetWatcher_;
};

