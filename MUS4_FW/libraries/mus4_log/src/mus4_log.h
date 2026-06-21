// mus4_log.h - aggregate header for the MUS4 logging library.
//
// Re-exports the public headers of the mus4_log library. The main sketch
// (MUS4_FW.ino) and any other mus4_* library that needs logging or JSON
// utilities can simply include <mus4_log.h>.
//
// Depends on: mus4_core (for FirmwareConfig.h).

#pragma once

#include "Mus4Log.h"
#include "JsonUtil.h"
