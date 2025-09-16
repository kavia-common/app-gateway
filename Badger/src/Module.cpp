#include "Badger/Module.h"
#include "Badger/BadgerPlugin.h"

namespace WPEFramework {
namespace Plugin {

// Register the Badger plugin with Thunder (WPEFramework).
// The major/minor version will be visible to the plugin manager.
SERVICE_REGISTRATION(Badger, 1, 0);

} // namespace Plugin
} // namespace WPEFramework
