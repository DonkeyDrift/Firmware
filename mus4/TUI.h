#pragma once
#include <Arduino.h>
#include "SharedTypes.h"

class TUI {
public:
    TUI(Print& out);
    void update(unsigned long currentTime);
    void render();
    void setRC(int ch1, int ch2, int ch3, int ch4);
    void setOutput(int throttle, int steering, int mode, bool park);
    void setSensors(const SensorData& data);
    
    // Configuration
    void setRefreshRate(unsigned long ms);
    void setAnsiEnabled(bool enabled);
    void setWaveformEnabled(bool enabled);
    void forceRedraw();
    unsigned long getLastRenderDuration() const { return _lastRenderDuration; }
    void log(const char* format, ...);

private:
    Print& _out;
    unsigned long _lastUpdate;
    unsigned long _refreshRate;
    unsigned long _lastRenderDuration;
    bool _forceRedraw;
    bool _ansiEnabled;
    bool _waveformEnabled;
    bool _initialized;
    bool _outputStateInitialized;
    char _logBuffer[64];
    unsigned long _logTime;

    // Current State
    struct State {
        int ch1, ch2, ch3, ch4;
        ControlData output;
        SensorData sensors;
        int throttleWave[WAVE_WIDTH];
        int steeringWave[WAVE_WIDTH];
    } _state;

    // Previous State for Dirty Checking
    struct State _lastState;
    unsigned long _lastWaveUpdate;

    // Helper methods
    void drawHeader();
    void drawMode();
    void drawPark();
    void drawRC();
    void drawOutput();
    void drawWaveforms();
    void drawSensors();
    void drawLog();
    void cursorTo(int row, int col);
    void updateWaveformData();
};
