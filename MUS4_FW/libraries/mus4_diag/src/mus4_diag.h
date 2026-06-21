// mus4_diag.h - aggregate header for the MUS4 diagnostic library.
//
// Re-exports the public headers of the mus4_diag library. The main
// sketch (MUS4_FW.ino) and any other mus4_* library that needs
// runtime degradation detection, BENCH/STRESS/REGRESS diagnostics, or
// BLE gamepad output can simply include <mus4_diag.h>.
//
// Depends on: mus4_core, mus4_ui.
//
// Individual headers remain addressable as <mus4_diag/<Header>.h>.

#pragma once

#include "Diagnostics.h"
#include "GamepadMode.h"
