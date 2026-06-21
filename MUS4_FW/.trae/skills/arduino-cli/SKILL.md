---
name: arduino-cli
description: Automates Arduino/ESP32 project compilation, upload, and monitoring. Invoke when user wants to build, upload, or monitor Arduino/ESP32 projects.
---

# Arduino CLI Automation

This skill automates the workflow for Arduino/ESP32 projects using the `arduino-cli.py` script.

## Usage

Use the `RunCommand` tool to execute the `arduino-cli.py` script.

### Script Location
**Important**: Do not hardcode the path to `arduino-cli.py`. First, use the `Glob` tool (e.g., `**/arduino-cli.py`) to search for and locate the `arduino-cli.py` script within the current workspace before running any commands. This ensures flexible configuration across different environments.

### Common Commands (Assuming script is found at `<FOUND_PATH>`)

**Compile only:**
```bash
python <FOUND_PATH> -c
```

**Compile and Upload:**
```bash
python <FOUND_PATH> -cu
```

**Compile, Upload, and Monitor (All-in-one):**
```bash
python <FOUND_PATH> -cus
```

### Options

- `-c, --compile`: Compile the sketch
- `-u, --upload`: Upload the firmware
- `-s, --serial`: Open serial monitor
- `-p, --port <PORT>`: Specify the serial port (e.g., COM3, /dev/ttyUSB0)
- `-b, --baud <BAUD>`: Specify baud rate (default: 115200)
- `--fqbn <FQBN>`: Specify Board FQBN (default: esp32:esp32:esp32)
- `--sketch <PATH>`: Specify sketch path (if not defined in config)

## Configuration

The script uses `config.yaml` for default settings. You can override these with command line arguments.

## Troubleshooting

- If `arduino-cli` is not found, ensure it's in the system PATH or configured in `config.yaml`.
- If upload fails, check the port and permissions.