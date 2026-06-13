#include "MUS4.h"

// This file groups related MUS4 firmware implementation sections so the
// Arduino project stays easy to browse without changing runtime behavior.

// ============================================================================
// Section: Mus4Log.cpp
// ============================================================================
uint8_t mus4LogTarget = MUS4_LOG_TARGET;
static Mus4LogSink mus4WebLogSink = nullptr;

void mus4SetWebLogSink(Mus4LogSink sink)
{
    mus4WebLogSink = sink;
}

void setMus4LogTargetWeb()
{
#if defined(ENABLE_WIFI_CONSOLE)
    mus4LogTarget = MUS4_LOG_TARGET_WEB;
#else
    mus4LogTarget = MUS4_LOG_TARGET_SERIAL;
#endif
}

void mus4LogLine(const char* source, const String& line)
{
#if defined(ENABLE_WIFI_CONSOLE)
    if (mus4LogTarget == MUS4_LOG_TARGET_WEB && mus4WebLogSink != nullptr) {
        mus4WebLogSink(source, line);
        return;
    }
#endif
    Serial.println("[" + String(source) + "] " + line);
}

void mus4Logf(const char* source, const char* fmt, ...)
{
    char buf[192];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    mus4LogLine(source, String(buf));
}

// ============================================================================
// Section: JsonUtil.cpp
// ============================================================================
void appendJsonString(String& out, const char* text)
{
    out += '"';
    while (*text) {
        char c = *text++;
        if (c == '\\' || c == '"') {
            out += '\\';
            out += c;
        } else if (c == '\n') {
            out += "\\n";
        } else if (c == '\r') {
            out += "\\r";
        } else if ((uint8_t)c >= 32) {
            out += c;
        }
    }
    out += '"';
}

// ============================================================================
// Section: I2CBusTools.cpp
// ============================================================================
bool I2CRead(uint8_t Address, uint8_t Register, uint8_t Nbytes, uint8_t *Data)
{
    bool ret = true;

    // Set register address
    Wire.beginTransmission(Address);
    Wire.write(Register);
    if (Wire.endTransmission())
    {
        ret = false;
        // Serial.println("I2C Read Errro"); // Suppress
    }

    // Read Nbytes
    Wire.requestFrom(Address, Nbytes);
    uint8_t index = 0;
    while (Wire.available())
    {
        Data[index++] = Wire.read();
    }

    return ret;
}

uint16_t I2CReadValue(uint8_t addr, uint8_t reg)
{
    uint16_t ret = -1;

    uint8_t data[2];
    if (I2CRead(addr, reg, 2, data))
    {
        ret = (uint16_t)data[0] << 8 | data[1];
    }

    return ret;
}

void I2CWriteValue(uint8_t Address, uint8_t Register, uint16_t Data)
{
    uint8_t *pData = (uint8_t *)&Data;

    // Set register address
    Wire.beginTransmission(Address);
    Wire.write(Register);
    Wire.write(pData[1]);
    Wire.write(pData[0]);
    if (Wire.endTransmission())
    {
        // Serial.println("I2C Write Error"); // Suppress
    }
}

const char *identifyI2CDeviceByAddress(uint8_t address)
{
    switch (address)
    {
    case 0x3C:
    case 0x3D:
        return "SSD1306 OLED / SH1106";
    case 0x40:
        return "INA219 / PCA9685";
    case 0x41:
        return "INA219 (alt address)";
    case 0x48:
    case 0x49:
    case 0x4A:
    case 0x4B:
        return "ADS1115 / TMP102";
    case 0x68:
    case 0x69:
        return "MPU6050 / MPU9250 / DS3231";
    case 0x76:
    case 0x77:
        return "BME280 / BMP280";
    default:
        return "Unknown";
    }
}

bool I2CReadRegister8(uint8_t address, uint8_t reg, uint8_t *value)
{
    Wire.beginTransmission(address);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0)
    {
        return false;
    }

    if (Wire.requestFrom((int)address, 1) != 1)
    {
        return false;
    }

    *value = Wire.read();
    return true;
}

bool probeMPU6050AtAddress(uint8_t address, uint8_t *whoAmI)
{
    const uint8_t MPU6050_WHO_AM_I_REG = 0x75;
    uint8_t id = 0;

    if (!I2CReadRegister8(address, MPU6050_WHO_AM_I_REG, &id))
    {
        return false;
    }

    *whoAmI = id;
    return (id == 0x68 || id == 0x69);
}

// ============================================================================
// Section: Sensors.cpp
// ============================================================================
extern Adafruit_INA219 ina219;
extern Adafruit_MPU6050 mpu;
extern SensorData ina219Data;
extern SensorData mpu6050Data;
extern uint8_t g_mpuCandidateAddress;
extern uint8_t g_mpuWhoAmIValue;
extern uint32_t g_i2cWorkingSpeed;
extern uint8_t g_i2cScanAddresses[16];
extern uint8_t g_i2cScanCount;

void printLastI2CScanSummary()
{
    mus4LogLine("i2c", "Last scan summary:");
    if (g_i2cScanCount == 0)
    {
        mus4LogLine("i2c", "No devices recorded in last scan");
        return;
    }

    for (uint8_t i = 0; i < g_i2cScanCount; i++)
    {
        uint8_t addr = g_i2cScanAddresses[i];
        mus4Logf("i2c", "0x%02X - %s", addr, identifyI2CDeviceByAddress(addr));
    }
}

void read_ina219()
{
    // Read INA219 data
    float shuntvoltage = ina219.getShuntVoltage_mV();
    float busvoltage = ina219.getBusVoltage_V();
    float current_mA = ina219.getCurrent_mA();
    float power_mW = ina219.getPower_mW();
    float loadvoltage = busvoltage + (shuntvoltage / 1000);
    
    // Check data validity
    if (current_mA == 0 && busvoltage == 0 && power_mW == 0)
    {
        ina219Data.valid = false;
        return;
    }
    
    // Store data in global variables
    ina219Data.readCount++;
    ina219Data.lastReadTime = millis();
    ina219Data.busVoltage = busvoltage;
    ina219Data.shuntVoltage = shuntvoltage;
    ina219Data.loadVoltage = loadvoltage;
    ina219Data.current_mA = current_mA;
    ina219Data.power_mW = power_mW;
    ina219Data.valid = true;
}

void setup_ina219()
{
    mus4LogLine("ina219", "Initializing INA219 sensor...");

    if (!ina219.begin())
    {
        mus4LogLine("ina219", "ERROR Failed to find INA219 chip");
        mus4LogLine("ina219", "ERROR Please check I2C connection (SDA: GPIO 21, SCL: GPIO 22)");
        mus4LogLine("ina219", "ERROR Possible causes: address mismatch, wiring issue, power supply issue");
        while (1)
        {
            delay(1000);
            mus4LogLine("ina219", "ERROR Sensor not detected, waiting...");
        }
    }

    mus4LogLine("ina219", "Sensor initialized successfully!");

    // Use default calibration (32V, 2A range)
    // For higher precision, uncomment either line below:
    // ina219.setCalibration_32V_1A();  // 32V, 1A range (higher precision)
    // ina219.setCalibration_16V_400mA(); // 16V, 400mA range (highest precision)

    mus4LogLine("ina219", "Calibration: 32V, 2A range (default)");
    mus4LogLine("ina219", "Setup complete, ready for data acquisition");
}

void read_mpu6050()
{
    /* Get new sensor events with the readings */
    sensors_event_t a, g, temp;
    
    if (!mpu.getEvent(&a, &g, &temp))
    {
        mpu6050Data.valid = false;
        return;
    }
    
    // Store data in global variables
    mpu6050Data.readCount++;
    mpu6050Data.lastReadTime = millis();
    mpu6050Data.accelX = a.acceleration.x;
    mpu6050Data.accelY = a.acceleration.y;
    mpu6050Data.accelZ = a.acceleration.z;
    mpu6050Data.gyroX = g.gyro.x;
    mpu6050Data.gyroY = g.gyro.y;
    mpu6050Data.gyroZ = g.gyro.z;
    mpu6050Data.temperature = temp.temperature;
    mpu6050Data.valid = true;
}

void scanI2CBus()
{
    mus4LogLine("i2c", "Scanning I2C bus...");
    byte error, address;
    int nDevices = 0;
    g_mpuCandidateAddress = 0;
    g_mpuWhoAmIValue = 0;
    g_i2cScanCount = 0;

    for(address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0)
        {
            mus4Logf("i2c", "Found device at 0x%02X - %s", address, identifyI2CDeviceByAddress(address));
            if (g_i2cScanCount < sizeof(g_i2cScanAddresses))
            {
                g_i2cScanAddresses[g_i2cScanCount++] = address;
            }

            if (address == 0x68 || address == 0x69)
            {
                uint8_t whoAmI = 0;
                if (probeMPU6050AtAddress(address, &whoAmI))
                {
                    g_mpuCandidateAddress = address;
                    g_mpuWhoAmIValue = whoAmI;
                    mus4Logf("i2c", "MPU probe OK at 0x%02X (WHO_AM_I=0x%02X)", address, whoAmI);
                }
                else if (I2CReadRegister8(address, 0x75, &whoAmI))
                {
                    mus4Logf("i2c", "MPU-family device at 0x%02X but WHO_AM_I=0x%02X (not MPU6050)", address, whoAmI);
                }
                else
                {
                    mus4Logf("i2c", "Could not read WHO_AM_I at 0x%02X", address);
                }
            }
            nDevices++;
        }
        else if (error == 4)
        {
            mus4Logf("i2c", "Unknown error at 0x%02X", address);
        }
    }

    if (nDevices == 0)
    {
        mus4LogLine("i2c", "No I2C devices found!");
    }
    else
    {
        mus4Logf("i2c", "Found %d device(s)", nDevices);
    }
}

bool tryInitMPU6050OnCurrentBus(uint8_t *activeAddress, int maxRetriesPerAddress)
{
    uint8_t tryAddress[2] = {0x68, 0x69};
    if (g_mpuCandidateAddress == 0x69)
    {
        tryAddress[0] = 0x69;
        tryAddress[1] = 0x68;
    }

    for (int i = 0; i < 2; i++)
    {
        for (int retryCount = 1; retryCount <= maxRetriesPerAddress; retryCount++)
        {
            uint8_t addr = tryAddress[i];
            mus4Logf("mpu6050", "Try addr 0x%02X (attempt %d/%d)", addr, retryCount, maxRetriesPerAddress);

            if (mpu.begin(addr, &Wire))
            {
                *activeAddress = addr;
                return true;
            }

            mus4Logf("mpu6050", "Init failed at 0x%02X", addr);
            delay(300);
        }
    }

    return false;
}

void setup_mpu6050()
{
    mus4LogLine("mpu6050", "Initializing MPU6050 sensor...");
    uint8_t activeAddress = 0;
    const int maxRetriesPerAddress = 2;
    bool initOk = tryInitMPU6050OnCurrentBus(&activeAddress, maxRetriesPerAddress);

    if (!initOk)
    {
        mus4LogLine("mpu6050", "Retry with lower I2C speed: 100kHz");
        Wire.begin(SDA_PIN, SCL_PIN, 100000L);
        g_i2cWorkingSpeed = 100000L;
        delay(50);
        scanI2CBus();
        initOk = tryInitMPU6050OnCurrentBus(&activeAddress, maxRetriesPerAddress);
    }

    if (!initOk)
    {
        mus4LogLine("mpu6050", "Retry with lower I2C speed: 50kHz");
        Wire.begin(SDA_PIN, SCL_PIN, 50000L);
        g_i2cWorkingSpeed = 50000L;
        delay(50);
        scanI2CBus();
        initOk = tryInitMPU6050OnCurrentBus(&activeAddress, maxRetriesPerAddress);
    }

    if (!initOk)
    {
        mus4LogLine("mpu6050", "ERROR Failed to find MPU6050 chip");
        mus4LogLine("mpu6050", "ERROR Please check I2C connection (SDA: GPIO 21, SCL: GPIO 22)");
        mus4LogLine("mpu6050", "ERROR Possible causes:");
        mus4LogLine("mpu6050", "1. I2C address mismatch (try 0x68 or 0x69)");
        mus4LogLine("mpu6050", "2. Wiring issues (SDA/SCL swapped or loose)");
        mus4LogLine("mpu6050", "3. Power supply issue");
        mus4LogLine("mpu6050", "4. I2C bus speed too high");
        printLastI2CScanSummary();

        if (g_mpuCandidateAddress != 0)
        {
            mus4Logf("mpu6050", "ERROR Probe saw candidate at 0x%02X, WHO_AM_I=0x%02X",
                          g_mpuCandidateAddress, g_mpuWhoAmIValue);
        }

        unsigned long lastWaitLogMs = 0;
        unsigned long lastRescanMs = millis();
        while (1)
        {
            unsigned long now = millis();

            if (now - lastWaitLogMs >= 1000UL)
            {
                lastWaitLogMs = now;
                mus4LogLine("mpu6050", "ERROR Sensor not detected, waiting...");
            }

            if (now - lastRescanMs >= 5000UL)
            {
                lastRescanMs = now;
                mus4LogLine("mpu6050", "ERROR Auto re-scan I2C bus (5s interval)...");
                scanI2CBus();
                printLastI2CScanSummary();
            }

            delay(50);
        }
    }

    mus4LogLine("mpu6050", "Sensor initialized successfully!");
    mus4Logf("mpu6050", "Active I2C address: 0x%02X", activeAddress);
    mus4Logf("mpu6050", "Active I2C speed: %lu Hz", g_i2cWorkingSpeed);
    if (g_mpuWhoAmIValue != 0)
    {
        mus4Logf("mpu6050", "WHO_AM_I = 0x%02X", g_mpuWhoAmIValue);
    }

    // Confirm WHO_AM_I once for the initialized address to avoid misidentification caused by bus interference
    uint8_t whoAmI = 0;
    if (I2CReadRegister8(activeAddress, 0x75, &whoAmI))
    {
        mus4Logf("mpu6050", "WHO_AM_I readback: 0x%02X", whoAmI);
        if (whoAmI != 0x68 && whoAmI != 0x69)
        {
            mus4LogLine("mpu6050", "WARNING WHO_AM_I mismatch, device may not be MPU6050");
        }
    }
    else
    {
        mus4LogLine("mpu6050", "WARNING WHO_AM_I readback failed after init");
    }

    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    const char* accelRangeText = "Unknown";
    switch (mpu.getAccelerometerRange())
    {
    case MPU6050_RANGE_2_G:
        accelRangeText = "+-2G";
        break;
    case MPU6050_RANGE_4_G:
        accelRangeText = "+-4G";
        break;
    case MPU6050_RANGE_8_G:
        accelRangeText = "+-8G";
        break;
    case MPU6050_RANGE_16_G:
        accelRangeText = "+-16G";
        break;
    }
    mus4Logf("mpu6050", "Accelerometer range set to: %s", accelRangeText);

    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    const char* gyroRangeText = "Unknown";
    switch (mpu.getGyroRange())
    {
    case MPU6050_RANGE_250_DEG:
        gyroRangeText = "+- 250 deg/s";
        break;
    case MPU6050_RANGE_500_DEG:
        gyroRangeText = "+- 500 deg/s";
        break;
    case MPU6050_RANGE_1000_DEG:
        gyroRangeText = "+- 1000 deg/s";
        break;
    case MPU6050_RANGE_2000_DEG:
        gyroRangeText = "+- 2000 deg/s";
        break;
    }
    mus4Logf("mpu6050", "Gyro range set to: %s", gyroRangeText);

    mpu.setFilterBandwidth(MPU6050_BAND_94_HZ);
    const char* bandwidthText = "Unknown";
    switch (mpu.getFilterBandwidth())
    {
    case MPU6050_BAND_260_HZ:
        bandwidthText = "260 Hz";
        break;
    case MPU6050_BAND_184_HZ:
        bandwidthText = "184 Hz";
        break;
    case MPU6050_BAND_94_HZ:
        bandwidthText = "94 Hz (Sampling rate: ~100Hz)";
        break;
    case MPU6050_BAND_44_HZ:
        bandwidthText = "44 Hz";
        break;
    case MPU6050_BAND_21_HZ:
        bandwidthText = "21 Hz";
        break;
    case MPU6050_BAND_10_HZ:
        bandwidthText = "10 Hz";
        break;
    case MPU6050_BAND_5_HZ:
        bandwidthText = "5 Hz";
        break;
    }
    mus4Logf("mpu6050", "Filter bandwidth set to: %s", bandwidthText);
    mus4LogLine("mpu6050", "Setup complete, ready for data acquisition");
}

// End of MPU6050 functions

// ============================================================================
// Section: SerialLineReader.cpp
// ============================================================================
#ifdef ENABLE_WIFI_CONSOLE
#endif

static const char* serialSourceFor(HardwareSerial& ser)
{
    if (&ser == &Serial1) return "serial1";
    return "serial";
}

void readSerialBuf(HardwareSerial& ser, SerialBuf& sb)
{
    while (ser.available())
    {
        int c = ser.read();
        if (c < 0) break;
        if (c == '\r') continue;
        if (c == '\n')
        {
            sb.buf[sb.len] = 0;
            String line = String(sb.buf);
#ifdef ENABLE_WIFI_CONSOLE
            appendWebLog(serialSourceFor(ser), String("> ") + redactWirelessConsoleLine(line));
#endif
            String response;
            StringPrint out(response);
            dispatchCommandLine(line, out, sb);
            ser.print(response);
#ifdef ENABLE_WIFI_CONSOLE
            appendWebLogLines(serialSourceFor(ser), response);
#endif
            sb.len = 0;
            sb.overflow = false;
        }
        else
        {
            if (sb.len < sizeof(sb.buf)-1)
            {
                sb.buf[sb.len++] = (char)c;
            }
            else
            {
                sb.len = 0;
                sb.overflow = true;
            }
        }
    }
}

// ============================================================================
// Section: TUI.cpp
// ============================================================================
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

void TUI::setRC(int ch1, int ch2, int ch3, int ch4, int ch5, int ch6) {
    _state.ch1 = ch1;
    _state.ch2 = ch2;
    _state.ch3 = ch3;
    _state.ch4 = ch4;
    _state.ch5 = ch5;
    _state.ch6 = ch6;
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
    _out.print("DonkeyCar Control System - ");
    _out.println(MUS4_FIRMWARE_VERSION);
    _out.println("===================================");
    if (_ansiEnabled) _out.print(ANSI_RESET);
}

void TUI::drawMode() {
    if (!_forceRedraw && _state.output.mode == _lastState.output.mode) return;
    
    cursorTo(ROW_MODE, 1);
    _out.print("MODE: ");

    if (_ansiEnabled) {
        switch(_state.output.mode) {
            case CAR_MODE_MANUAL:
                _out.print(ANSI_GREEN "MANUAL   " ANSI_RESET);
                break;
            case CAR_MODE_SEMI_AUTO:
                _out.print(ANSI_YELLOW "SEMI-AUTO" ANSI_RESET);
                break;
            case CAR_MODE_FULL_AUTO:
                _out.print(ANSI_MAGENTA "FULL-AUTO" ANSI_RESET);
                break;
            default:
                _out.print("UNKNOWN  ");
        }
    } else {
        _out.print(_state.output.mode);
    }
}

void TUI::drawPark() {
    extern bool drift_assist_enabled;
    extern bool drift_assist_active;
    extern float drift_compensation;
    extern float gyro_z_filtered;
    extern float drift_assist_scale;

    cursorTo(ROW_PARK, 1);
    _out.print("PARK: ");
    if (_ansiEnabled) {
        if (_state.output.park) _out.print(ANSI_RED "LOCKED  " ANSI_RESET);
        else _out.print(ANSI_GREEN "UNLOCKED" ANSI_RESET);
    } else {
        _out.print(_state.output.park ? "LOCKED  " : "UNLOCKED");
    }

    cursorTo(ROW_PARK, 18);
    if (drift_assist_enabled) {
        if (_ansiEnabled) _out.print(ANSI_GREEN);
        _out.print("DRIFT: ON");
        if (drift_assist_active) {
            if (_ansiEnabled) _out.print(ANSI_YELLOW);
            _out.printf(" ACTIVE  GyroZ: %+5.2f  Comp: %+4.0f  Scale: %.1f", gyro_z_filtered, drift_compensation, drift_assist_scale);
        } else {
            _out.printf(" STANDBY GyroZ: %+5.2f  Scale: %.1f", gyro_z_filtered, drift_assist_scale);
        }
    } else {
        if (_ansiEnabled) _out.print(ANSI_WHITE);
        _out.print("DRIFT: OFF");
    }
    if (_ansiEnabled) _out.print(ANSI_RESET "\033[K");
}

void TUI::drawRC() {
    bool changed = _forceRedraw ||
                   _state.ch1 != _lastState.ch1 ||
                   _state.ch2 != _lastState.ch2 ||
                   _state.ch3 != _lastState.ch3 ||
                   _state.ch4 != _lastState.ch4 ||
                   _state.ch5 != _lastState.ch5 ||
                   _state.ch6 != _lastState.ch6;
                   
    if (!changed) return;
    
    cursorTo(ROW_RC, 1);
    _out.printf("RC: [CH1:%4d] [CH2:%4d] [CH3:%4d] [CH4:%4d] [CH5:%4d] [CH6:%4d]",
        _state.ch1, _state.ch2, _state.ch3, _state.ch4, _state.ch5, _state.ch6);
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

// ============================================================================
// Section: LedStatus.cpp
// ============================================================================
extern CRGB leds[];
extern bool toggleActive;
extern CRGB toggleColor1;
extern CRGB toggleColor2;
extern unsigned long toggleTime;
extern unsigned long toggleInterval;

// Modified setLEDColor function
void setLEDColor(CRGB targetColor)
{
    if (toggleActive)
    {
        toggleActive = false; // Disable toggle mode
        toggleTime = 0;       // Reset toggle time
    }
    if (leds[0] != targetColor)
    {
        leds[0] = targetColor;
        FastLED.show(); // Update display only when the color differs
    }
}

// Added setLEDToggle function
void setLEDToggle(CRGB color1, CRGB color2)
{
    toggleColor1 = color1;
    toggleColor2 = color2;
    toggleActive = true;
    toggleTime = 0; // Ensure the first toggle runs immediately
}

void scanLEDToggle()
{
    if (toggleActive && (millis() >= toggleTime))
    {
        CRGB currentColor = leds[0];
        CRGB nextColor = (currentColor == toggleColor1) ? toggleColor2 : toggleColor1;
        leds[0] = nextColor;
        FastLED.show();
        toggleTime = millis() + toggleInterval;
    }
}

// ============================================================================
// Section: Buzzer.cpp
// ============================================================================
int Buzzer::_channelCounter = 2; // 从2开始避免与PWM通道(0,1)冲突

// 手动模式 - 低音单音调
BuzzerNote melodyManual[] = {
    { NOTE_C4, N4 }
};

// 半自动模式 - 中音单音调
BuzzerNote melodySemiAuto[] = {
    { NOTE_E4, N4 }
};

// 全自动模式 - 高音单音调
BuzzerNote melodyFullAuto[] = {
    { NOTE_G4, N4 }
};

// 锁定提示音 - 下降音阶 G4-E4-C4
BuzzerNote melodyParkLock[] = {
    { NOTE_G4, N8 },
    { NOTE_E4, N8 },
    { NOTE_C4, N4 },
    { NOTE_REST, N8 }
};

// 解锁提示音 - 上升音阶 C4-E4-G4
BuzzerNote melodyParkUnlock[] = {
    { NOTE_C4, N8 },
    { NOTE_E4, N8 },
    { NOTE_G4, N4 },
    { NOTE_REST, N8 }
};

Buzzer::Buzzer(int pin) {
    _pin = pin;
    _channel = _channelCounter++;
    _volume = BUZZER_VOLUME;
#if BUZZER_SOUND_ENABLED
    ledcAttachChannel(_pin, 2000, 8, _channel);
    setVolume(_volume);
#else
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
#endif
}

void Buzzer::setVolume(int volume) {
    _volume = constrain(volume, 0, 100);
}

void Buzzer::playNoteWithVolume(int pitch, int durationMs) {
#if BUZZER_SOUND_ENABLED
    if (pitch == 0) {
        ledcWriteChannel(_channel, 0);
    } else {
        ledcChangeFrequency(_pin, pitch, 8);
        int dutyCycle = _volume * 255 / 100;
        ledcWriteChannel(_channel, dutyCycle);
        delay(durationMs);
        ledcWriteChannel(_channel, 0);
    }
#else
    digitalWrite(_pin, LOW);
#endif
}

void Buzzer::playMelody(const BuzzerNote* melody, int length) {
#if BUZZER_SOUND_ENABLED
    _playing = true;
    int beatMs = 60000 / 120;
    for (int i = 0; i < length; i++) {
        int durMs = beatMs * 4 / melody[i].duration;
        playNoteWithVolume(melody[i].pitch, durMs);
        delay(durMs * 0.3);
    }
    _playing = false;
#else
    _playing = false;
    digitalWrite(_pin, LOW);
#endif
}

void Buzzer::playModeSound(int mode) {
    switch (mode) {
        case CAR_MODE_MANUAL:
            playMelody(melodyManual, sizeof(melodyManual) / sizeof(BuzzerNote));
            break;
        case CAR_MODE_SEMI_AUTO:
            playMelody(melodySemiAuto, sizeof(melodySemiAuto) / sizeof(BuzzerNote));
            break;
        case CAR_MODE_FULL_AUTO:
            playMelody(melodyFullAuto, sizeof(melodyFullAuto) / sizeof(BuzzerNote));
            break;
    }
}

void Buzzer::playParkLockSound() {
    playMelody(melodyParkLock, sizeof(melodyParkLock) / sizeof(BuzzerNote));
}

void Buzzer::playParkUnlockSound() {
    playMelody(melodyParkUnlock, sizeof(melodyParkUnlock) / sizeof(BuzzerNote));
}

void Buzzer::update() {
}

// ============================================================================
// Section: GamepadMode.cpp
// ============================================================================
#ifdef ENABLE_GAMEPAD_MODE

extern BleGamepad bleGamepad;
extern volatile uint16_t pwm_value[];

void sendGamepadPacket() {
    if (bleGamepad.isConnected()) {
        // Map RC Channels (approx 1000-2000) to Gamepad Axes (-32767 to 32767)
        int lx = map(constrain(pwm_value[CH_STEERING], 1000, 2000), 1000, 2000, 0, 32767);
        int ly = map(constrain(pwm_value[CH_THROTTLE], 1300, 1800), 1300, 1800, 32767, 0);
        // int rx = map(constrain(pwm_value[CH_PARK], 1000, 2000), 1000, 2000, -32767, 32767);
        // int ry = map(constrain(pwm_value[CH_MODE], 1000, 2000), 1000, 2000, -32767, 32767);
        int rx = 0;
        int ry = 0;

        bleGamepad.setLeftThumb(0, ly);
        bleGamepad.setRightThumb(lx, 0);
    }
}
#endif

