# Badger Thunder Plugin (C++) — Skeleton

This folder contains a Thunder (WPEFramework) plugin skeleton for the Badger permissions layer, ready to be extended with real logic (e.g., Thor permission registry parsing, check endpoints, and inter-plugin calls).

Key goals:
- Conform to Thunder plugin architecture (IPlugin + JSONRPC, Module headers).
- Provide a minimal, compilable C++ scaffold.
- Offer hooks for JSON-RPC API exposure (e.g., `ping`, `permissions.listMethods`).
- Supply a sample plugin manifest (`plugin.json`) to register in Thunder.

Structure:
- CMakeLists.txt: Build script for a shared plugin library.
- plugin.json: Sample plugin configuration/manifest (callsign, class, autostart, config).
- include/BadgerThunder/
  - Module.h: Central Thunder include aggregator and module naming.
  - BadgerThunderPlugin.h: Core plugin class interface.
- src/
  - Module.cpp: Module declaration boilerplate.
  - BadgerThunderPlugin.cpp: Core implementation with JSON-RPC registration hooks.

Build:
1) Configure:
   - `cmake -S . -B build`
2) Build:
   - `cmake --build build -j`
3) Install (optional):
   - `cmake --install build`

Notes:
- The CMake script tries to locate common Thunder libs (`WPEFrameworkCore`, `WPEFrameworkPlugins`). Adjust names or disable `USE_WPEFRAMEWORK` if building in a different environment.
- This skeleton registers two example JSON‑RPC endpoints:
  - `BadgerThunder.ping` → returns "pong".
  - `BadgerThunder.permissions.listMethods` → enumerates stubbed Badger permission API method names.
- Extend by adding real handlers (e.g., `permissions.check`, `permissions.checkAll`) and wire them to your Thor/YAML registry logic.

Security and configuration:
- `plugin.json` contains example configuration fields (e.g., `registryPath`) that your implementation can read during `Initialize`.
- Do not hardcode secrets. Use platform configuration/Thunder mechanisms to provide paths, toggles, and optional credentials.

License:
- Sample scaffolding; integrate under your project’s license terms.
