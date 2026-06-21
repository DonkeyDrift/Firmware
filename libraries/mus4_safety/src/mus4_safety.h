// mus4_safety.h - aggregate header for the mus4_safety library.
//
// Re-exports the public headers of the mus4_safety library. The main sketch
// (MUS4_FW.ino) and any other mus4_* library that needs this functionality
// can simply include <mus4_safety.h>.
//
// Depends on: mus4_core.
//
// Individual headers remain addressable as <mus4_safety/<Header>.h>.

#pragma once

#include "SafetyState.h"
#include "ActuatorOutput.h"
