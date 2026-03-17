#include "TUI.h"

// ANSI Colors
#define ANSI_RESET "\033[0m"
#define ANSI_RED "\033[31m"
#define ANSI_GREEN "\033[32m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_BLUE "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN "\033[36m"
#define ANSI_WHITE "\033[37m"

const int ROW_HEADER = 1;
const int ROW_MODE = 3;
const int ROW_PARK = 4;
const int ROW_LOG = 5;
const int ROW_RC = 7;
const int ROW_OUTPUT = 8;
const int ROW_WAVE_START = 10;

TUI::TUI(Print& out) : _out(out) {
    _refreshRate = 16;
    _forceRedraw = true;
    _ansiEnabled = true;
    _waveformEnabled = true;
    _initialized = false;
    _outputStateInitialized = false;
    _lastUpdate = 0;
    _lastWaveUpdate = 0;
    _lastRenderDuration = 0;
    
    memset(_logBuffer, 0, sizeof(_logBuffer));
    _logTime = 0;

    // Initialize state
    memset(&_state, 0, sizeof(_state));
    memset(&_lastState, 0, sizeof(_lastState));
    
    // Set lastState to invalid values to ensure initial draw
    _lastState.output.mode = -1;  // Invalid mode to force initial draw
    _lastState.output.park = true; // Force initial draw (toggle from false)
}

void TUI::setAnsiEnabled(bool enabled) {
    _ansiEnabled = enabled;
}

void TUI::setWaveformEnabled(bool enabled) {
    if (_waveformEnabled == enabled) return;
    _waveformEnabled = enabled;
    forceRedraw();
}

void TUI::setRefreshRate(unsigned long ms) {
    _refreshRate = ms;
}

void TUI::setRC(int ch1, int ch2, int ch3, int ch4) {
    _state.ch1 = ch1;
    _state.ch2 = ch2;
    _state.ch3 = ch3;
    _state.ch4 = ch4;
}

void TUI::setOutput(int throttle, int steering, int mode, bool park) {
    _state.output.throttle = throttle;
    _state.output.steering = steering;
    _state.output.mode = mode;
    _state.output.park = park;

    // Ensure Mode/Park are shown on first valid output push even if value equals defaults.
    if (!_outputStateInitialized) {
        _lastState.output.mode = -1;
        _lastState.output.park = !park;
        _outputStateInitialized = true;
    }

    updateWaveformData();
}

void TUI::setSensors(const SensorData& data) {
    _state.sensors = data;
}

void TUI::update(unsigned long currentTime) {
    if (currentTime - _lastUpdate > _refreshRate) {
        render();
        _lastUpdate = currentTime;
    }
}

void TUI::forceRedraw() {
    _forceRedraw = true;
    _initialized = false; // Trigger full clear
}

void TUI::log(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(_logBuffer, sizeof(_logBuffer), format, args);
    va_end(args);
    _logTime = millis();
    _forceRedraw = true; // Ensure log is drawn immediately
}

void TUI::cursorTo(int row, int col) {
    if (_ansiEnabled) {
        _out.printf("\033[%d;%dH", row, col);
    }
}

void TUI::updateWaveformData() {
    // Shift data
    for (int i = 1; i < WAVE_WIDTH; i++) {
        _state.throttleWave[i-1] = _state.throttleWave[i];
        _state.steeringWave[i-1] = _state.steeringWave[i];
    }
    // Add new data
    _state.throttleWave[WAVE_WIDTH-1] = _state.output.throttle;
    _state.steeringWave[WAVE_WIDTH-1] = _state.output.steering;
}

void TUI::render() {
    unsigned long start = millis();
    if (!_initialized) {
        if (_ansiEnabled) {
            _out.print("\033[2J\033[H\033[?25l"); // Clear, Home, Hide Cursor
        }
        drawHeader();
        _initialized = true;
        _forceRedraw = true; // Ensure all components draw
    }

    if (_ansiEnabled) {
        _out.print("\033[?25l"); // Ensure cursor hidden
    }

    drawMode();
    drawPark();
    drawLog();
    drawRC();
    drawOutput();
    if (_waveformEnabled) drawWaveforms();
    drawSensors();

    // Save state for next diff
    _lastState = _state;
    _forceRedraw = false;
    _lastRenderDuration = millis() - start;
}

void TUI::drawHeader() {
    cursorTo(ROW_HEADER, 1);
    if (_ansiEnabled) _out.print(ANSI_CYAN);
    _out.println("DonkeyCar Control System - v1.0");
    _out.println("===================================");
    if (_ansiEnabled) _out.print(ANSI_RESET);
}

void TUI::drawMode() {
    if (!_forceRedraw && _state.output.mode == _lastState.output.mode) return;
    
    cursorTo(ROW_MODE, 1);
    _out.print("Mode: ");
    
    if (_ansiEnabled) {
        switch(_state.output.mode) {
            case CAR_MODE_MANUAL: 
                _out.print(ANSI_GREEN "Manual   " ANSI_RESET); 
                break;
            case CAR_MODE_SEMI_AUTO: 
                _out.print(ANSI_YELLOW "Semi-Auto" ANSI_RESET); 
                break;
            case CAR_MODE_FULL_AUTO: 
                _out.print(ANSI_MAGENTA "Full-Auto" ANSI_RESET); 
                break;
            default:
                _out.print("Unknown  ");
        }
    } else {
        _out.print(_state.output.mode);
    }
}

void TUI::drawPark() {
    if (!_forceRedraw && _state.output.park == _lastState.output.park) return;
    
    cursorTo(ROW_PARK, 1);
    _out.print("Park: ");
    if (_ansiEnabled) {
        if (_state.output.park) _out.print(ANSI_RED "LOCKED  " ANSI_RESET);
        else _out.print(ANSI_GREEN "UNLOCKED" ANSI_RESET);
    } else {
        _out.print(_state.output.park ? "LOCKED" : "UNLOCKED");
    }
}

void TUI::drawRC() {
    bool changed = _forceRedraw || 
                   _state.ch1 != _lastState.ch1 || 
                   _state.ch2 != _lastState.ch2 ||
                   _state.ch3 != _lastState.ch3 ||
                   _state.ch4 != _lastState.ch4;
                   
    if (!changed) return;
    
    cursorTo(ROW_RC, 1);
    // Format: RC: [CH1: 1500] [CH2: 1500] [CH3: 1500] [CH4: 1500]
    _out.printf("RC: [CH1:%4d] [CH2:%4d] [CH3:%4d] [CH4:%4d]", 
        _state.ch1, _state.ch2, _state.ch3, _state.ch4);
}

void TUI::drawOutput() {
    bool changed = _forceRedraw ||
                   _state.output.throttle != _lastState.output.throttle ||
                   _state.output.steering != _lastState.output.steering;
                   
    if (!changed) return;
    
    cursorTo(ROW_OUTPUT, 1);
    _out.print("Out: ");

    // Steering Bar
    _out.print("Str ");
    int s = _state.output.steering;
    if (_ansiEnabled) {
        if (s > 0) _out.print(ANSI_CYAN);
        else if (s < 0) _out.print(ANSI_CYAN);
    }
    _out.printf("%4d  ", s);
    if (_ansiEnabled) _out.print(ANSI_RESET);

    // Throttle Bar
    _out.print("Thr ");
    int t = _state.output.throttle; // -100 to 100
    if (_ansiEnabled) {
        if (t > 0) _out.print(ANSI_GREEN);
        else if (t < 0) _out.print(ANSI_RED);
    }
    _out.printf("%4d", t);
    if (_ansiEnabled) _out.print(ANSI_RESET);
}

void TUI::drawWaveforms() {
    // Only redraw if data changed? Waveform always changes if it scrolls
    // But we can optimize by only redrawing if new data arrived
    // Here we assume setOutput calls updateWaveformData
    
    // To match nvtop, we draw a graph.
    // Using Braille characters or blocks is complex for Serial.
    // We stick to simple blocks but optimize rendering.
    
    // Draw Throttle Wave
    int startRow = ROW_WAVE_START;
    
    if (_forceRedraw) {
        cursorTo(startRow, 1);
        _out.println("Throttle History:");
        // Draw Box/Grid if needed
    }
    
    // We only redraw the graph content
    for (int y = 0; y < WAVE_HEIGHT; y++) {
        cursorTo(startRow + 1 + y, 1);
        _out.print("  "); // Margin
        
        // Logical Y from bottom (0) to top (HEIGHT-1)
        // Screen Y is top to bottom
        int logicY = WAVE_HEIGHT - 1 - y;
        
        for (int x = 0; x < WAVE_WIDTH; x++) {
            int val = _state.throttleWave[x];
            int normalized = map(val, -100, 100, 0, WAVE_HEIGHT-1);

            for (int w = 0; w < 2; w++) {
                if (normalized == logicY) {
                    if (_ansiEnabled) _out.print(ANSI_GREEN "#" ANSI_RESET);
                    else _out.print("#");
                } else if (logicY == (WAVE_HEIGHT-1)/2) {
                    _out.print("-"); // Zero line
                } else {
                    _out.print(" ");
                }
            }
        }
    }
    
    // Draw Steering Wave (below Throttle)
    int steeringRow = startRow + 1 + WAVE_HEIGHT + 1;
    if (_forceRedraw) {
        cursorTo(steeringRow, 1);
        _out.println("Steering History:");
    }
    
    for (int y = 0; y < WAVE_HEIGHT; y++) {
        cursorTo(steeringRow + 1 + y, 1);
        _out.print("  ");
        int logicY = WAVE_HEIGHT - 1 - y;
        for (int x = 0; x < WAVE_WIDTH; x++) {
            int val = _state.steeringWave[x];
            int normalized = map(val, -100, 100, 0, WAVE_HEIGHT-1);

            for (int w = 0; w < 2; w++) {
                if (normalized == logicY) {
                    if (_ansiEnabled) _out.print(ANSI_CYAN "#" ANSI_RESET);
                    else _out.print("#");
                } else if (logicY == (WAVE_HEIGHT-1)/2) {
                    _out.print("-");
                } else {
                    _out.print(" ");
                }
            }
        }
    }
}

void TUI::drawLog() {
    int row = (_waveformEnabled ? (ROW_WAVE_START + 1 + WAVE_HEIGHT + 1 + 1 + WAVE_HEIGHT + 4) : ROW_LOG);
    cursorTo(row, 1);
    
    // Clear line
    if (_ansiEnabled) _out.print("\033[K");
    
    if (strlen(_logBuffer) > 0) {
        if (_ansiEnabled) _out.print(ANSI_YELLOW);
        _out.print("LOG: ");
        _out.print(_logBuffer);
        if (_ansiEnabled) _out.print(ANSI_RESET);
    }
    // Ensure we are below log for any external prints
    if (_ansiEnabled) _out.printf("\033[%d;1H", row + 1);
}

void TUI::drawSensors() {
    // Always update sensors? Or check dirty?
    // Sensors update slower usually
    
    int row = 0;
    if (_waveformEnabled) {
        row = ROW_WAVE_START + 1 + WAVE_HEIGHT + 1 + 1 + WAVE_HEIGHT + 2;
    } else {
        row = ROW_OUTPUT + 2;
    }
    cursorTo(row, 1);
    
    if (_state.sensors.valid) {
        _out.printf("INA: %5.2fV %5.1fmA %5.1fmW", 
            _state.sensors.busVoltage, 
            _state.sensors.current_mA, 
            _state.sensors.power_mW);
    } else {
        _out.print("INA: N/A");
    }
    // Clear rest of line
    if (_ansiEnabled) _out.print("\033[K");
    
    cursorTo(row+1, 1);
    if (_state.sensors.valid) { 
        _out.printf("MPU: A[%.1f,%.1f,%.1f] G[%.1f,%.1f,%.1f]",
            _state.sensors.accelX, _state.sensors.accelY, _state.sensors.accelZ,
            _state.sensors.gyroX, _state.sensors.gyroY, _state.sensors.gyroZ);
    } else {
        _out.print("MPU: N/A");
    }

    if (_forceRedraw) {
        cursorTo(row+3, 1);
        if (_ansiEnabled) _out.print("\033[K");
        _out.print("[按下 ESC 退出系统]");
        if (_ansiEnabled) _out.print("\033[K");
    }

    // Clear rest of line
    if (_ansiEnabled) _out.print("\033[K");

}
