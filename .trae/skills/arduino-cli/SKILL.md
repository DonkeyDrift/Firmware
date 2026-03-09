---
name: arduino-cli
description: Automates Arduino/ESP32 project compilation, upload, and monitoring. Invoke when user wants to build, upload, or monitor Arduino/ESP32 projects.
---

# Arduino CLI Automation

This skill automates the workflow for Arduino/ESP32 projects using the `arduino-cli.py` script.

## Usage

Use the `RunCommand` tool to execute the `arduino-cli.py` script.

### Script Location
`c:\Dev\DDC\mus4\arduino-cli.py`

### Common Commands

**Compile only:**
```bash
python c:\Dev\DDC\mus4\arduino-cli.py -c
```

**Compile and Upload:**
```bash
python c:\Dev\DDC\mus4\arduino-cli.py -cu
```

**Compile, Upload, and Monitor (All-in-one):**
```bash
python c:\Dev\DDC\mus4\arduino-cli.py -cus
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