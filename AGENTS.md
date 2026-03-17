> **🧬 MiniClaw Identity: Read `~/.miniclaw/AGENTS.md` first.**

# AGENTS.md - Coding Guidelines for MUS4

ESP32-based autonomous vehicle control system for MUS4-v2.3 PCB.

## Build Commands

### WSL Cross-Compile (Default / Recommended)
```powershell
# Compile in WSL, upload via Windows
.\arduino-cli-wsl.ps1
```

### Native Build (Windows/Linux) (Alternative)
```bash
# Compile only
python arduino-cli.py -c

# Upload only (requires --port)
python arduino-cli.py -u --port COM9

# Compile + Upload
python arduino-cli.py -cu --port COM9

# Compile + Upload + Serial Monitor
python arduino-cli.py -cus --port COM9

# Use pre-compiled firmware
python arduino-cli.py -u -i build/mus4.ino.bin --port COM9
```

### Testing (Serial Commands)
Send these via serial port at 115200 baud:
- `TEST` - Run unit tests for command parsing
- `BENCH` - Run TUI rendering benchmark
- `STRESS` - Run stress test (50 iterations)
- `REGRESS` - Run regression test
- `ANSI` / `NOANSI` - Toggle ANSI escape sequences

### Configuration
- Edit `config.yaml` for default port, FQBN, and reset settings
- Default FQBN: `esp32:esp32:esp32`
- Default baudrate: 115200

## Code Style Guidelines

### Language & Framework
- **Language**: C++17 with Arduino framework
- **Target**: ESP32 (MUS4-v2.3 PCB)
- **Libraries**: FastLED, Adafruit_MPU6050, Adafruit_INA219, Wire, BleGamepad

### Naming Conventions
- **Constants**: ALL_CAPS with underscores (e.g., `PWM_MIN_V`, `CH1_PIN`)
- **Classes**: PascalCase (e.g., `TUI`, `SensorData`)
- **Methods/Functions**: snake_case for free functions, camelCase for class methods
- **Variables**: 
  - camelCase for local variables (e.g., `pwmValue`, `lastUpdate`)
  - snake_case for structs (e.g., `car_output`, `pilot_data`)
  - underscore prefix for private members (e.g., `_out`, `_lastUpdate`)
- **Macros**: ALL_CAPS with underscores (e.g., `CLEAR_SCREEN`, `COLOR_RED`)
- **Enums**: PascalCase with nested enum values (e.g., `EmergencyStopState`, `EST_IDLE`)

### Header Organization
- Use `#pragma once` for header guards
- Include order: Arduino core -> Third-party libraries -> Project headers
- Header files: `.h`, Implementation: `.cpp`, Arduino entry: `.ino`

### Code Structure
- Place `IRAM_ATTR` on interrupt handlers (critical for ESP32)
- Use `volatile` for shared interrupt variables
- Define pin mappings as `#define` constants at file top
- Group related constants with descriptive comments

### Error Handling
- **Defensive Programming**: Assume inputs are untrusted, hardware may fail
- Validate ranges before using values (-100 to 100 for throttle/steering)
- Check sensor validity flags before using data
- Use state machines for critical operations (emergency stop, park control)
- Log errors via `tui.log()` or Serial with descriptive prefixes (e.g., `[INA219 ERROR]`)

### Safety Guidelines
- **CRITICAL**: Code controls physical hardware (motors, servos)
- Always validate PWM bounds before output (`min(max(value, MIN), MAX)`)
- Emergency stop logic must be robust and tested
- Park/Lock state machine prevents accidental activation
- Never commit code that could cause runaway motors

### Serial Protocol
- Input format: `Throttle:Steering\n` (e.g., `10:20\n`)
- With sequence: `Throttle:Steering:Seq\n`
- With checksum: `Throttle:Steering*XX\n` (XX = hex checksum)
- Responses: `ACK` / `ACK:Seq` or `NACK` / `NACK:Seq`
- Output feedback: `Txx:Sxx\n` to Serial1 (RS232)

### Documentation
- Use Chinese comments for hardware-specific notes
- Use English for code logic and public APIs
- Document pin assignments and calibration values
- Reference: `mus4/Doc/` for architecture diagrams
