#pragma once

/*
 * App2AppProvider Thunder plugin: lifecycle and interface plumbing.
 * This class exposes Exchange::IApp2AppProvider via QueryInterface and delegates
 * business logic to App2AppProviderImplementation.
 */

#include <plugins/IPlugin.h>
#include <plugins/IPlugin.h>
#include <plugins/IShell.h>
#include <plugins/Plugin.h>
#include <memory>
#include <atomic>
#include <string>

#include "App2AppProviderImplementation.h"
#include "UtilsLogging.h"

namespace WPEFramework {
namespace Plugin {

/**
 * PUBLIC_INTERFACE
 * App2AppProvider plugin entry class.
 * - Implements PluginHost::IPlugin and IPluginExtended.
 * - Delegates IApp2AppProvider to App2AppProviderImplementation.
 */
class App2AppProvider : public PluginHost::IPlugin, public PluginHost::IPluginExtended {
public:
    App2AppProvider()
        : _service(nullptr)
        , _refCount(1) {
        LOGTRACE("App2AppProvider constructed");
    }

    ~App2AppProvider() override {
        LOGTRACE("App2AppProvider destructed");
    }

    // IUnknown
    uint32_t AddRef() const override {
        return ++_refCount;
    }
    uint32_t Release() const override {
        uint32_t result = --_refCount;
        if (result == 0) {
            delete const_cast<App2AppProvider*>(this);
        }
        return result;
    }

    void* QueryInterface(const uint32_t id) override {
        if (id == PluginHost::IPlugin::ID) {
            AddRef();
            return static_cast<PluginHost::IPlugin*>(this);
        }
        if (id == PluginHost::IPluginExtended::ID) {
            AddRef();
            return static_cast<PluginHost::IPluginExtended*>(this);
        }
        if (id == Exchange::IApp2AppProvider::ID) {
            if (_impl) {
                _impl->AddRef();
                return static_cast<Exchange::IApp2AppProvider*>(_impl.get());
            }
            return nullptr;
        }
        return nullptr;
    }

    // PUBLIC_INTERFACE
    /**
     * Initialize the plugin and wire the implementation.
     */
    const string Initialize(PluginHost::IShell* service) override {
        LOGTRACE("Initialize called");
        _service = service;
        try {
            _impl = std::make_unique<App2AppProviderImplementation>(_service);
        } catch (const std::exception& e) {
            LOGERR("Failed to create implementation: %s", e.what());
            return string("App2AppProvider: implementation creation failed");
        }
        return string();
    }

    // PUBLIC_INTERFACE
    /**
     * Deinitialize the plugin and release resources.
     */
    void Deinitialize(PluginHost::IShell* /*service*/) override {
        LOGTRACE("Deinitialize called");
        _impl.reset();
        _service = nullptr;
    }

    // PUBLIC_INTERFACE
    /**
     * Provide plugin metadata (optional).
     */
    string Information() const override {
        return string();
    }

    // PUBLIC_INTERFACE
    /**
     * Channel attach hook (not used, return true).
     */
    bool Attach(PluginHost::Channel& /*channel*/) override {
        return true;
    }

    // PUBLIC_INTERFACE
    /**
     * Channel detach hook; cleanup any state bound to the connection.
     */
    void Detach(PluginHost::Channel& channel) override {
        const uint32_t connId = channel.Id();
        LOGINFO("Detach: cleanup for connectionId=%u", connId);
        if (_impl) {
            _impl->CleanupByConnection(connId);
        }
    }

private:
    PluginHost::IShell* _service;
    std::unique_ptr<App2AppProviderImplementation> _impl;
    mutable std::atomic<uint32_t> _refCount;
};

} // namespace Plugin
} // namespace WPEFramework

// Register this plugin with Thunder
SERVICE_REGISTRATION(WPEFramework::Plugin::App2AppProvider, 1, 0)
