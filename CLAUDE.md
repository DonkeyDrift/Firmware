# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**MUS4** is an ESP32-based autonomous vehicle control system (MUS4-v2.3 PCB version). It handles:
- RC receiver PWM signal acquisition (4 channels)
- Serial command processing from upper computer (Pilot)
- Steering servo and motor ESC PWM output
- LED status indication (WS2812B)
- BLE Gamepad mode (RC to Bluetooth gamepad conversion)
- I2C sensors: INA219 (power monitor) and MPU6050 (IMU)

## Build Commands

### Native Build (Windows/Linux)
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

### WSL Cross-Compile (Windows + WSL)
```powershell
# Compile in WSL, upload via Windows
.\build_wsl_fast.ps1
```

### Configuration
- Edit `config.yaml` for default port, FQBN, and reset settings
- Default FQBN: `esp32:esp32:esp32`
- Default baudrate: 115200

## Architecture

### Main Entry Point
`mus4/mus4.ino` - Main Arduino sketch containing:
- Interrupt handlers for RC PWM input (GPIO 36, 39, 34, 26)
- Serial command parser (`T:S` format with optional checksum and sequence)
- Control loop with mode switching
- Emergency stop state machine

### Supporting Files
- `SharedTypes.h` - Common data structures (SensorData, ControlData, mode constants)
- `TUI.h/cpp` - Terminal UI class for ANSI serial display (nvtop-style dashboard)

### Control Modes
| Mode | ID | Steering Source | Throttle Source | LED Color |
|------|----|-----------------|-----------------|-----------|
| Manual | 0 | RC | RC | Green |
| Semi-Auto | 1 | Pilot | RC | Yellow |
| Full-Auto | 2 | Pilot | Pilot | Blue |

### Key Data Structures
```cpp
struct struct_message {
    int throttle;  // -100 to 100
    int steering;  // -100 to 100
    int mode;      // 0=Manual, 1=Semi-Auto, 2=Full-Auto
    bool park;     // Lock/Unlock state
};
```

### Serial Protocol
- **Input**: `Throttle:Steering\n` (e.g., `10:20\n`)
- **Input with sequence**: `Throttle:Steering:Seq\n`
- **Input with checksum**: `Throttle:Steering*XX\n` (XX = hex checksum)
- **Response**: `ACK` or `ACK:Seq` / `NACK` or `NACK:Seq`
- **Output feedback**: `Txx:Sxx\n` to Serial1 (RS232)

### Pin Configuration (MUS4-v2.3 PCB)
| Function | GPIO | Notes |
|----------|------|-------|
| RC CH1 (Steering) | 36 | Input only |
| RC CH2 (Throttle) | 39 | Input only |
| RC CH3 (Park) | 34 | Input only |
| RC CH4 (Mode) | 26 | |
| Steering Servo | 23 | PWM 50Hz |
| Throttle ESC | 25 | PWM 50Hz |
| LED (WS2812B) | 5 | |
| Serial1 RX | 16 | RS232 |
| Serial1 TX | 17 | RS232 |
| I2C SDA | 21 | INA219, MPU6050 |
| I2C SCL | 22 | INA219, MPU6050 |

## BLE Gamepad Mode

Enabled by `#define ENABLE_GAMEPAD_MODE`. Maps RC channels to gamepad axes:
- CH1 (Steering) → Right Thumb X
- CH2 (Throttle) → Left Thumb Y

Device name: "Gamepad MU03"

## Testing Commands (via Serial)

Send these commands to the serial port for testing:
- `TEST` - Run unit tests for command parsing
- `BENCH` - Run TUI rendering benchmark
- `STRESS` - Run stress test
- `REGRESS` - Run regression test
- `ANSI` / `NOANSI` - Toggle ANSI escape sequences

## Documentation

See `mus4/Doc/` for detailed documentation:
- `Arch/architecture.md` - System architecture with diagrams
- `Hardware/pin_definitions.md` - Complete pin configuration
