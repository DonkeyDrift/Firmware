// mus4_rc.h - aggregate header for the mus4_rc library.
//
// Re-exports the public headers of the mus4_rc library. The main sketch
// (MUS4_FW.ino) and any other mus4_* library that needs this functionality
// can simply include <mus4_rc.h>.
//
// Depends on: mus4_core.
//
// Individual headers remain addressable as <mus4_rc/<Header>.h>.

#pragma once

#include "RcPwmCapture.h"
#include "RcFilter.h"
