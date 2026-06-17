// mus4_i2c.h - aggregate header for the mus4_i2c library.
//
// Re-exports the public headers of the mus4_i2c library. The main sketch
// (MUS4_FW.ino) and any other mus4_* library that needs this functionality
// can simply include <mus4_i2c.h>.
//
// Depends on: mus4_core.
//
// Individual headers remain addressable as <mus4_i2c/<Header>.h>.

#pragma once

#include "I2CBusTools.h"
#include "Sensors.h"
