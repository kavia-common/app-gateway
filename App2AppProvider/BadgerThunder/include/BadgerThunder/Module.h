#pragma once

// Centralized module header to pull in Thunder core/plugin headers and
// define the module name used in logging / tracing.
// Adjust includes to match your Thunder/WPEFramework installation layout.

#ifndef MODULE_NAME
#define MODULE_NAME BadgerThunder
#endif

// Thunder Core and Plugins headers (paths may vary by distribution)
#include <core/JSON.h>
#include <core/core.h>
#include <plugins/JSONRPC.h>
#include <plugins/Plugin.h>

// Standard
#include <cstdint>
#include <string>
#include <vector>

// Logging helpers (no-op here; wire to your logger as needed)
#ifndef BADGER_LOG_INFO
#define BADGER_LOG_INFO(msg) do { /* hook logger */ (void)sizeof(msg); } while(0)
#endif

#ifndef BADGER_LOG_ERROR
#define BADGER_LOG_ERROR(msg) do { /* hook logger */ (void)sizeof(msg); } while(0)
#endif
