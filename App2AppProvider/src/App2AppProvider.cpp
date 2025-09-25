#include "App2AppProvider.h"
#include "App2AppProviderImplementation.h"

#include <core/Portability.h>

namespace WPEFramework {
namespace Plugin {

// Register the service (plugin)
SERVICE_REGISTRATION(App2AppProvider, 1, 0);

// -------------------- App2AppProvider --------------------

App2AppProvider::App2AppProvider()
    : _service(nullptr)
    , _implementation(nullptr)
    , _refCount(1) {
    LOGTRACE("App2AppProvider constructed");
}

App2AppProvider::~App2AppProvider() {
    LOGTRACE("App2AppProvider destructed");
}

// PUBLIC_INTERFACE
const string App2AppProvider::Initialize(PluginHost::IShell* service) {
    LOGTRACE("Initialize enter");
    _service = service;

    const string callsign = (_service != nullptr ? _service->Callsign() : string());
    LOGINFO("Initializing App2AppProvider, callsign=%s", callsign.c_str());

    // Create implementation (it will acquire IAppGateway via COMRPC)
    _implementation = Core::Service<App2AppProviderImplementation>::Create<App2AppProviderImplementation>(_service);
    if (_implementation == nullptr) {
        LOGERR("Failed to create App2AppProviderImplementation instance");
        return string("App2AppProvider: failed to create implementation");
    }

    LOGTRACE("Initialize exit");
    return string();
}

// PUBLIC_INTERFACE
void App2AppProvider::Deinitialize(PluginHost::IShell* /*service*/) {
    LOGTRACE("Deinitialize enter");
    if (_implementation != nullptr) {
        // Release the aggregated implementation
        _implementation->Release();
        _implementation = nullptr;
    }
    _service = nullptr;
    LOGTRACE("Deinitialize exit");
}

// PUBLIC_INTERFACE
string App2AppProvider::Information() const {
    return string("App2AppProvider Thunder plugin: exposes IApp2AppProvider via COMRPC and routes all App2App flows via AppGateway.");
}

// PUBLIC_INTERFACE
bool App2AppProvider::Attach(PluginHost::Channel& channel) {
    LOGTRACE("Attach channelId=%u", channel.Id());
    // Accept channel attach
    return true;
}

// PUBLIC_INTERFACE
void App2AppProvider::Detach(PluginHost::Channel& channel) {
    const uint32_t id = channel.Id();
    LOGTRACE("Detach channelId=%u: cleaning connection-scoped state", id);
    if (_implementation != nullptr) {
        _implementation->CleanupByConnection(id);
    }
}

// IUnknown
uint32_t App2AppProvider::AddRef() const {
    return Core::InterlockedIncrement(_refCount);
}

uint32_t App2AppProvider::Release() const {
    uint32_t result = Core::InterlockedDecrement(_refCount);
    if (result == 0) {
        delete this;
        return Core::ERROR_DESTRUCTION_SUCCEEDED;
    }
    return Core::ERROR_NONE;
}

} // namespace Plugin
} // namespace WPEFramework
