# LP-MU-S4 / MUS4 Firmware

[中文文档](README.zh-CN.md)

MUS4 is an ESP32 + Arduino firmware for the LP-MU-S4 remote-control vehicle platform. It reads RC PWM input, accepts Pilot serial commands, blends control sources, applies Park and emergency-stop safety logic, drives steering/throttle PWM outputs, samples I2C sensors, and exposes a Wi-Fi/TCP/Web Console with OTA support.

Firmware version metadata lives in the project header and release notes are kept in `CHANGELOG.md`.

## Current Features

- RC PWM capture for CH1-CH6: steering, throttle, Park, mode, Drift Assist enable, and Drift Assist scale.
- Pilot command input through USB `Serial`, RS232 `Serial1`, TCP Console, and Web Console.
- Manual, semi-auto, and full-auto control blending.
- Park lock and emergency-stop state machines that can override throttle output.
- ESP32 `ledc` PWM output for steering servo and ESC.
- `Serial1` telemetry in `Txx:Sxx` format. DEV mode may keep the OTA window open while telemetry continues; telemetry pauses only during an active OTA transfer.
- INA219 and MPU6050 I2C sampling.
- TUI status output on USB `Serial` when log routing targets serial.
- Wi-Fi AP/STA, TCP Console, Web Console, WebSocket telemetry, ArduinoOTA, and HTTP `/update`.
- BLE Gamepad output only on builds where Wi-Fi Console is disabled.

## Source Layout

The Arduino-facing firmware is intentionally compact:

- `MUS4_FW.ino`: `setup()`, `loop()`, global runtime wiring, and scheduling.
- `MUS4.h`: the single project header. It contains compile-time configuration, build info, shared types, runtime state structures, and public APIs for all firmware modules.
- `MUS4_IO.cpp`: logging, serial line handling, TUI, LED, buzzer, I2C helpers, and sensors.
- `MUS4_Control.cpp`: RC capture/filtering, steering, Drift Assist, control mixing, safety state, and actuator output.
- `MUS4_Command.cpp`: Pilot command parsing, local command dispatch, diagnostics, and steering calibration.
- `MUS4_Wifi.cpp`: wireless command policy, STA/AP management, OTA, Web Console HTTP handlers, web logs, and WebSocket telemetry.
- `WebConsoleAssets.h`: generated PROGMEM Web Console HTML/CSS/JS asset.
- `WirelessSecrets.h`: local Wi-Fi credentials; ignored by git.

This follows a standard Arduino sketch pattern: one `.ino` entry point, one project header, and a small set of implementation files grouped by responsibility.

## Hardware Pins

| Function | GPIO | Notes |
| --- | --- | --- |
| RC CH1 Steering | 36 | Input only |
| RC CH2 Throttle | 39 | Input only |
| RC CH3 Park | 34 | Input only |
| RC CH4 Mode | 26 | PWM input |
| RC CH5 Drift enable | 27 | PWM input |
| RC CH6 Drift scale | 35 | Input only |
| Steering servo | 23 | `ledc` PWM |
| ESC throttle | 25 | `ledc` PWM |
| WS2812B LED | 5 | Status LED |
| UART_SEL | 12 | UART routing |
| Serial1 RX/TX | 16 / 17 | RS232 Pilot I/O |
| I2C SDA/SCL | 21 / 22 | INA219 / MPU6050 |

GPIO 34, 35, 36, and 39 are input-only pins.

## Serial Protocol

Input frames:

```text
Throttle:Steering
Throttle:Steering:Seq
Throttle:Steering*XX
```

`Throttle` and `Steering` are constrained to `-100..100`. `XX` is a two-digit hexadecimal checksum.

Responses:

```text
ACK
NACK
ACK:Seq
NACK:Seq
```

`Serial1` telemetry:

```text
Txx:Sxx
```

## Build and Test

Python dependencies:

```bash
pip install pyyaml pyserial pytest
```

Run tests:

```bash
pytest tests/
```

Native Arduino CLI wrapper:

```bash
python arduino-cli.py -c --sketch MUS4_FW.ino
python arduino-cli.py -u --sketch MUS4_FW.ino --port COM9
python arduino-cli.py -cu --no-progress --sketch MUS4_FW.ino
```

WSL helper:

```powershell
.\arduino-cli-wsl.ps1 -Compile -Sketch MUS4_FW.ino
.\arduino-cli-wsl.ps1 -Compile -Upload -HttpOta -HttpOtaHost <device-ip> -Sketch MUS4_FW.ino
```

## Operating Notes

- Keep the vehicle Park-locked when testing commands, OTA, or steering calibration.
- Treat USB Serial, RS232 Serial1, TCP Console, and Web Console input as untrusted input.
- Wireless command permission rules are mirrored in `wireless_console_policy.py`; update tests when policy changes.
- `provisioning_system/` and `multi_agent_framework/` are separate projects and are not part of the main firmware build path.

## Documentation

- `docs/Guide/MUS4_用户说明书.md`: operator manual.
- `docs/Arch/architecture.md`: deeper firmware architecture notes.
- `docs/Hardware/pin_definitions.md`: hardware pin reference.
- `docs/Tools/ArduinoCLI.md`: Arduino CLI wrapper usage.
- `docs/Plan/ROADMAP.md`: future work only. Items there are not current firmware behavior.
