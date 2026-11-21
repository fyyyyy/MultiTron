// XR_APILAYER_MULTITRON_stick - OpenXR API layer that adds MultiTron's output to
// the LEFT THUMBSTICK of the real controller (e.g. Quest 3 via Virtual Desktop).
//
// It sits between the application (e.g. OpenComposite running Skyrim VR) and the
// runtime. It records which actions the app bound to /user/hand/left/input/thumbstick,
// then adds the MultiTron value (shared memory, see shared/MultiTronIpc.h) to those
// actions' state in xrGetActionStateVector2f / xrGetActionStateFloat.
#define XR_NO_PROTOTYPES
#include <openxr/openxr.h>
#include <openxr/openxr_loader_negotiation.h>

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>

#include "MultiTronIpc.h"

namespace {

// ---------------------------------------------------------------- logging
void Log(const char* format, ...) {
    char dir[MAX_PATH]{};
    if (!GetEnvironmentVariableA("LOCALAPPDATA", dir, sizeof(dir))) return;
    std::string folder = std::string(dir) + "\\MultiTron2";
    CreateDirectoryA(folder.c_str(), nullptr);
    FILE* file = nullptr;
    if (fopen_s(&file, (folder + "\\openxr-layer.log").c_str(), "a") != 0 || !file) return;
    SYSTEMTIME t;
    GetLocalTime(&t);
    fprintf(file, "[%02d:%02d:%02d.%03d] ", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
    va_list args;
    va_start(args, format);
    vfprintf(file, format, args);
    va_end(args);
    fputc('\n', file);
    fclose(file);
}

// ---------------------------------------------------------------- state
struct ActionInfo {
    bool leftStick2D = false;   // bound to /user/hand/left/input/thumbstick
    bool leftStickX = false;    // bound to .../thumbstick/x
    bool leftStickY = false;    // bound to .../thumbstick/y
    bool rightStick = false;    // also bound to a right thumbstick path
    bool loggedInjection = false;
};

struct LayerState {
    std::mutex mutex;
    XrInstance instance = XR_NULL_HANDLE;
    PFN_xrGetInstanceProcAddr nextGetInstanceProcAddr = nullptr;
    PFN_xrDestroyInstance nextDestroyInstance = nullptr;
    PFN_xrCreateAction nextCreateAction = nullptr;
    PFN_xrSuggestInteractionProfileBindings nextSuggestBindings = nullptr;
    PFN_xrGetActionStateVector2f nextGetStateVector2f = nullptr;
    PFN_xrGetActionStateFloat nextGetStateFloat = nullptr;
    PFN_xrPathToString nextPathToString = nullptr;
    PFN_xrStringToPath nextStringToPath = nullptr;
    XrPath leftHandPath = XR_NULL_PATH;
    std::unordered_map<uint64_t, ActionInfo> actions;
} g;

uint64_t Key(XrAction action) { return reinterpret_cast<uint64_t>(action); }
float Clamp1(float v) { return v < -1.f ? -1.f : (v > 1.f ? 1.f : v); }

// Latest MultiTron value. Returns true only while MultiTron is live and non-zero.
bool ReadMultiTron(float& x, float& y) {
    static multitron_ipc::Mapping map;
    static std::once_flag once;
    std::call_once(once, [] { map.Open(); });
    auto* state = map.state();
    if (!state) return false;
    const uint64_t now = GetTickCount64();
    state->layerLastSeenMs.store(now, std::memory_order_relaxed);  // lets the app show "connected"
    const auto snapshot = multitron_ipc::ReadSnapshot(*state);
    if (snapshot.stickWriteMs == 0 || now - snapshot.stickWriteMs >= multitron_ipc::kStaleMs) return false;
    x = snapshot.stickX;
    y = snapshot.stickY;
    return x != 0.f || y != 0.f;
}

std::string PathToString(XrPath path) {
    char buffer[256]{};
    uint32_t count = 0;
    if (!g.nextPathToString || XR_FAILED(g.nextPathToString(g.instance, path, sizeof(buffer), &count, buffer))) return {};
    return buffer;
}

// Should an action queried with this subaction path receive the injected value?
bool AppliesToLeft(const ActionInfo& info, XrPath subactionPath) {
    if (subactionPath == g.leftHandPath && g.leftHandPath != XR_NULL_PATH) return true;
    if (subactionPath == XR_NULL_PATH) return !info.rightStick;  // ambiguous when both hands are bound
    return false;
}

// ---------------------------------------------------------------- hooks
XrResult XRAPI_CALL layer_xrCreateAction(XrActionSet actionSet, const XrActionCreateInfo* createInfo, XrAction* action) {
    return g.nextCreateAction(actionSet, createInfo, action);
}

XrResult XRAPI_CALL layer_xrSuggestInteractionProfileBindings(XrInstance instance,
                                                              const XrInteractionProfileSuggestedBinding* suggested) {
    if (suggested && suggested->suggestedBindings) {
        std::lock_guard lock(g.mutex);
        for (uint32_t i = 0; i < suggested->countSuggestedBindings; ++i) {
            const auto& binding = suggested->suggestedBindings[i];
            const std::string path = PathToString(binding.binding);
            ActionInfo& info = g.actions[Key(binding.action)];
            if (path == "/user/hand/left/input/thumbstick") info.leftStick2D = true;
            else if (path == "/user/hand/left/input/thumbstick/x") info.leftStickX = true;
            else if (path == "/user/hand/left/input/thumbstick/y") info.leftStickY = true;
            else if (path.rfind("/user/hand/right/input/thumbstick", 0) == 0) info.rightStick = true;
            else continue;
            Log("binding: action %llu <- %s", static_cast<unsigned long long>(Key(binding.action)), path.c_str());
        }
    }
    return g.nextSuggestBindings(instance, suggested);
}

XrResult XRAPI_CALL layer_xrGetActionStateVector2f(XrSession session, const XrActionStateGetInfo* getInfo,
                                                   XrActionStateVector2f* state) {
    const XrResult result = g.nextGetStateVector2f(session, getInfo, state);
    if (XR_FAILED(result) || !getInfo || !state) return result;
    bool inject = false;
    {
        std::lock_guard lock(g.mutex);
        const auto found = g.actions.find(Key(getInfo->action));
        if (found != g.actions.end() && found->second.leftStick2D && AppliesToLeft(found->second, getInfo->subactionPath)) {
            inject = true;
        }
    }
    float x = 0.f, y = 0.f;
    if (inject && ReadMultiTron(x, y)) {
        state->currentState.x = Clamp1(state->currentState.x + x);
        state->currentState.y = Clamp1(state->currentState.y + y);
        state->isActive = XR_TRUE;
        state->changedSinceLastSync = XR_TRUE;
        std::lock_guard lock(g.mutex);
        auto& info = g.actions[Key(getInfo->action)];
        if (!info.loggedInjection) {
            info.loggedInjection = true;
            Log("injecting into vector2 action %llu", static_cast<unsigned long long>(Key(getInfo->action)));
        }
    } else if (inject) {
        float dummyX, dummyY;
        ReadMultiTron(dummyX, dummyY);  // keeps the app-side "connected" heartbeat alive while idle
    }
    return result;
}

XrResult XRAPI_CALL layer_xrGetActionStateFloat(XrSession session, const XrActionStateGetInfo* getInfo,
                                                XrActionStateFloat* state) {
    const XrResult result = g.nextGetStateFloat(session, getInfo, state);
    if (XR_FAILED(result) || !getInfo || !state) return result;
    bool useX = false, useY = false;
    {
        std::lock_guard lock(g.mutex);
        const auto found = g.actions.find(Key(getInfo->action));
        if (found != g.actions.end() && AppliesToLeft(found->second, getInfo->subactionPath)) {
            useX = found->second.leftStickX;
            useY = found->second.leftStickY;
        }
    }
    float x = 0.f, y = 0.f;
    if ((useX || useY) && ReadMultiTron(x, y)) {
        const float add = useX ? x : y;
        state->currentState = Clamp1(state->currentState + add);
        state->isActive = XR_TRUE;
        state->changedSinceLastSync = XR_TRUE;
    }
    return result;
}

XrResult XRAPI_CALL layer_xrDestroyInstance(XrInstance instance) {
    PFN_xrDestroyInstance next = nullptr;
    {
        std::lock_guard lock(g.mutex);
        next = g.nextDestroyInstance;
        g.actions.clear();
        g.instance = XR_NULL_HANDLE;
    }
    Log("instance destroyed");
    return next ? next(instance) : XR_ERROR_HANDLE_INVALID;
}

XrResult XRAPI_CALL layer_xrGetInstanceProcAddr(XrInstance instance, const char* name, PFN_xrVoidFunction* function) {
    if (!function || !name) return XR_ERROR_VALIDATION_FAILURE;
    if (!g.nextGetInstanceProcAddr) return XR_ERROR_HANDLE_INVALID;
    const auto hook = [&](const char* wanted, PFN_xrVoidFunction replacement) {
        if (std::strcmp(name, wanted) != 0) return false;
        // Still resolve through the chain so the call fails properly if unsupported.
        PFN_xrVoidFunction ignored = nullptr;
        const XrResult chained = g.nextGetInstanceProcAddr(instance, name, &ignored);
        if (XR_FAILED(chained)) return false;
        *function = replacement;
        return true;
    };
    if (hook("xrSuggestInteractionProfileBindings", reinterpret_cast<PFN_xrVoidFunction>(layer_xrSuggestInteractionProfileBindings))
        || hook("xrGetActionStateVector2f", reinterpret_cast<PFN_xrVoidFunction>(layer_xrGetActionStateVector2f))
        || hook("xrGetActionStateFloat", reinterpret_cast<PFN_xrVoidFunction>(layer_xrGetActionStateFloat))
        || hook("xrDestroyInstance", reinterpret_cast<PFN_xrVoidFunction>(layer_xrDestroyInstance))
        || hook("xrCreateAction", reinterpret_cast<PFN_xrVoidFunction>(layer_xrCreateAction))) {
        return XR_SUCCESS;
    }
    return g.nextGetInstanceProcAddr(instance, name, function);
}

XrResult XRAPI_CALL layer_xrCreateApiLayerInstance(const XrInstanceCreateInfo* info, const XrApiLayerCreateInfo* layerInfo,
                                                   XrInstance* instance) {
    if (!layerInfo || layerInfo->structType != XR_LOADER_INTERFACE_STRUCT_API_LAYER_CREATE_INFO
        || layerInfo->structVersion != XR_API_LAYER_CREATE_INFO_STRUCT_VERSION
        || layerInfo->structSize != sizeof(XrApiLayerCreateInfo) || !layerInfo->nextInfo
        || layerInfo->nextInfo->structType != XR_LOADER_INTERFACE_STRUCT_API_LAYER_NEXT_INFO
        || layerInfo->nextInfo->structVersion != XR_API_LAYER_NEXT_INFO_STRUCT_VERSION
        || layerInfo->nextInfo->structSize != sizeof(XrApiLayerNextInfo)) {
        return XR_ERROR_INITIALIZATION_FAILED;
    }
    const auto getProcAddr = layerInfo->nextInfo->nextGetInstanceProcAddr;
    XrApiLayerCreateInfo chained = *layerInfo;
    chained.nextInfo = layerInfo->nextInfo->next;
    const XrResult result = layerInfo->nextInfo->nextCreateApiLayerInstance(info, &chained, instance);
    if (XR_FAILED(result)) return result;

    std::lock_guard lock(g.mutex);
    g.instance = *instance;
    g.nextGetInstanceProcAddr = getProcAddr;
    const auto load = [&](const char* name, auto& target) {
        getProcAddr(*instance, name, reinterpret_cast<PFN_xrVoidFunction*>(&target));
    };
    load("xrDestroyInstance", g.nextDestroyInstance);
    load("xrCreateAction", g.nextCreateAction);
    load("xrSuggestInteractionProfileBindings", g.nextSuggestBindings);
    load("xrGetActionStateVector2f", g.nextGetStateVector2f);
    load("xrGetActionStateFloat", g.nextGetStateFloat);
    load("xrPathToString", g.nextPathToString);
    load("xrStringToPath", g.nextStringToPath);
    g.leftHandPath = XR_NULL_PATH;
    if (g.nextStringToPath) g.nextStringToPath(*instance, "/user/hand/left", &g.leftHandPath);
    g.actions.clear();
    Log("layer active for application '%s'", info ? info->applicationInfo.applicationName : "?");
    return XR_SUCCESS;
}

}  // namespace

extern "C" __declspec(dllexport) XrResult XRAPI_CALL xrNegotiateLoaderApiLayerInterface(
    const XrNegotiateLoaderInfo* loaderInfo, const char* /*layerName*/, XrNegotiateApiLayerRequest* request) {
    if (!loaderInfo || !request || loaderInfo->structType != XR_LOADER_INTERFACE_STRUCT_LOADER_INFO
        || loaderInfo->structVersion != XR_LOADER_INFO_STRUCT_VERSION
        || loaderInfo->structSize != sizeof(XrNegotiateLoaderInfo)
        || request->structType != XR_LOADER_INTERFACE_STRUCT_API_LAYER_REQUEST
        || request->structVersion != XR_API_LAYER_INFO_STRUCT_VERSION
        || request->structSize != sizeof(XrNegotiateApiLayerRequest)
        || loaderInfo->minInterfaceVersion > XR_CURRENT_LOADER_API_LAYER_VERSION
        || loaderInfo->maxInterfaceVersion < XR_CURRENT_LOADER_API_LAYER_VERSION) {
        return XR_ERROR_INITIALIZATION_FAILED;
    }
    request->layerInterfaceVersion = XR_CURRENT_LOADER_API_LAYER_VERSION;
    request->layerApiVersion = XR_CURRENT_API_VERSION;
    request->getInstanceProcAddr = layer_xrGetInstanceProcAddr;
    request->createApiLayerInstance = layer_xrCreateApiLayerInstance;
    return XR_SUCCESS;
}
