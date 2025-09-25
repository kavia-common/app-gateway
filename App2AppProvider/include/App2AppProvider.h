#pragma once

// Thunder / WPEFramework
#include <plugins/Module.h>
#include <plugins/IPlugin.h>
#include <plugins/IShell.h>
#include <core/Services.h>

// Project interfaces and logging
#include "IApp2AppProvider.h"
#include "IAppGateway.h"
#include "UtilsLogging.h"

// Forward declaration
namespace WPEFramework {
namespace Plugin {
    class App2AppProviderImplementation;
} // namespace Plugin
} // namespace WPEFramework

namespace WPEFramework {
namespace Plugin {

/**
 * PUBLIC_INTERFACE
 * App2AppProvider Thunder plugin entry. Exposes Exchange::IApp2AppProvider via COMRPC.
 * Lifecycle and registration are handled here; all business logic is delegated to App2AppProviderImplementation.
 */
class App2AppProvider final : public PluginHost::IPlugin, public PluginHost::IPluginExtended {
public:
    App2AppProvider(const App2AppProvider&) = delete;
    App2AppProvider& operator=(const App2AppProvider&) = delete;

    App2AppProvider();
    ~App2AppProvider() override;

public:
    // PUBLIC_INTERFACE
    /**
     * Initialize the plugin. Creates the implementation and wires AppGateway COMRPC.
     * @param service IShell pointer provided by Thunder.
     * @return Empty string on success, otherwise non-empty error message.
     */
    const string Initialize(PluginHost::IShell* service) override;

    // PUBLIC_INTERFACE
    /**
     * Deinitialize the plugin and release resources.
     * @param service IShell pointer provided by Thunder.
     */
    void Deinitialize(PluginHost::IShell* service) override;

    // PUBLIC_INTERFACE
    /**
     * Human-readable information about the plugin.
     * @return Descriptive information string.
     */
    string Information() const override;

    // PUBLIC_INTERFACE
    /**
     * WebSocket channel attached notification (used to track connection-scoped cleanup).
     * @return true to accept, false to reject.
     */
    bool Attach(PluginHost::Channel& channel) override;

    // PUBLIC_INTERFACE
    /**
     * WebSocket channel detached notification - used to cleanup connection-scoped registrations/correlations.
     */
    void Detach(PluginHost::Channel& channel) override;

public:
    // Core::IUnknown implementation using interface map macros
    BEGIN_INTERFACE_MAP(App2AppProvider)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IPluginExtended)
        // Expose IApp2AppProvider aggregated from the implementation object
        INTERFACE_AGGREGATE(WPEFramework::Exchange::IApp2AppProvider, _implementation)
    END_INTERFACE_MAP

    // AddRef/Release explicitly for COM
    uint32_t AddRef() const override;
    uint32_t Release() const override;

private:
    PluginHost::IShell* _service;
    App2AppProviderImplementation* _implementation;
    mutable uint32_t _refCount;
};

} // namespace Plugin
} // namespace WPEFramework
