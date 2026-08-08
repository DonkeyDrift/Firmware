#include "Buzzer.h"
#include "SharedTypes.h"
#include "MutePreference.h"

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

// Wi-Fi AP 启动提示音 - 上升音阶 C4-E4-G4
BuzzerNote melodyWifiApStart[] = {
    { NOTE_C4, N8 },
    { NOTE_E4, N8 },
    { NOTE_G4, N4 },
    { NOTE_REST, N8 }
};

// Wi-Fi AP 关闭提示音 - 下降音阶 G4-E4-C4
BuzzerNote melodyWifiApStop[] = {
    { NOTE_G4, N8 },
    { NOTE_E4, N8 },
    { NOTE_C4, N4 },
    { NOTE_REST, N8 }
};

// Wi-Fi STA 连接成功提示音 - 双短高音 G4-G4
BuzzerNote melodyWifiStaConnected[] = {
    { NOTE_G4, N8 },
    { NOTE_G4, N8 },
    { NOTE_REST, N8 }
};

// Wi-Fi STA 断开/失败提示音 - 单长低音 C4
BuzzerNote melodyWifiStaDisconnected[] = {
    { NOTE_C4, N4 },
    { NOTE_REST, N8 }
};

Buzzer::Buzzer(int pin) {
    _pin = pin;
    _channel = _channelCounter++;
    _volume = BUZZER_VOLUME;
    _soundEnabled = BUZZER_SOUND_ENABLED;
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

void Buzzer::stopTone() {
#if BUZZER_SOUND_ENABLED
    ledcWriteChannel(_channel, 0);
#else
    digitalWrite(_pin, LOW);
#endif
}

void Buzzer::startNote(int pitch, unsigned long durationMs) {
    _lastPitch = pitch;
    _noteDurationMs = durationMs;
    _noteStartMs = millis();
#if BUZZER_SOUND_ENABLED
    if (pitch == NOTE_REST) {
        ledcWriteChannel(_channel, 0);
    } else {
        ledcChangeFrequency(_pin, pitch, 8);
        int dutyCycle = _volume * 255 / 100;
        ledcWriteChannel(_channel, dutyCycle);
    }
#else
    digitalWrite(_pin, LOW);
#endif
}

void Buzzer::startMelody(const BuzzerNote* melody, int length) {
    if (melody == nullptr || length <= 0) return;
    // System-wide mute gate: suppress every firmware-driven sound (including
    // the boot-time AP start melody) while the user preference is muted.
    if (isSystemMuted()) return;
    // Stop any currently playing melody and start the new one immediately.
    stopTone();
    _currentMelody = melody;
    _melodyLength = length;
    _noteIndex = 0;
    _playing = true;
    int beatMs = 60000 / 120;
    unsigned long durMs = (unsigned long)beatMs * 4 / melody[0].duration;
    _restDurationMs = (unsigned long)(durMs * 0.3f);
    startNote(melody[0].pitch, durMs);
}

void Buzzer::playModeSound(int mode) {
    switch (mode) {
        case CAR_MODE_MANUAL:
            startMelody(melodyManual, sizeof(melodyManual) / sizeof(BuzzerNote));
            break;
        case CAR_MODE_SEMI_AUTO:
            startMelody(melodySemiAuto, sizeof(melodySemiAuto) / sizeof(BuzzerNote));
            break;
        case CAR_MODE_FULL_AUTO:
            startMelody(melodyFullAuto, sizeof(melodyFullAuto) / sizeof(BuzzerNote));
            break;
    }
}

void Buzzer::playParkLockSound() {
    startMelody(melodyParkLock, sizeof(melodyParkLock) / sizeof(BuzzerNote));
}

void Buzzer::playParkUnlockSound() {
    startMelody(melodyParkUnlock, sizeof(melodyParkUnlock) / sizeof(BuzzerNote));
}

void Buzzer::update() {
    if (!_playing || _currentMelody == nullptr) return;

    // Muted mid-melody: stop the active tone immediately and reset the state machine.
    if (isSystemMuted()) {
        stopTone();
        _playing = false;
        _currentMelody = nullptr;
        return;
    }

    unsigned long now = millis();
    unsigned long elapsed = now - _noteStartMs;

    // Still playing the active tone.
    if (elapsed < _noteDurationMs) return;

    // Tone finished; handle inter-note rest.
    unsigned long restElapsed = elapsed - _noteDurationMs;
    if (restElapsed < _restDurationMs) {
        if (_lastPitch != NOTE_REST) {
            stopTone();
            _lastPitch = NOTE_REST;
        }
        return;
    }

    // Move to the next note.
    _noteIndex++;
    if (_noteIndex >= _melodyLength) {
        stopTone();
        _playing = false;
        _currentMelody = nullptr;
        return;
    }

    const BuzzerNote& note = _currentMelody[_noteIndex];
    int beatMs = 60000 / 120;
    unsigned long durMs = (unsigned long)beatMs * 4 / note.duration;
    _restDurationMs = (unsigned long)(durMs * 0.3f);
    startNote(note.pitch, durMs);
}

void Buzzer::playWifiApStartSound() {
    startMelody(melodyWifiApStart, sizeof(melodyWifiApStart) / sizeof(BuzzerNote));
}

void Buzzer::playWifiApStopSound() {
    startMelody(melodyWifiApStop, sizeof(melodyWifiApStop) / sizeof(BuzzerNote));
}

void Buzzer::playWifiStaConnectedSound() {
    startMelody(melodyWifiStaConnected, sizeof(melodyWifiStaConnected) / sizeof(BuzzerNote));
}

void Buzzer::playWifiStaDisconnectedSound() {
    startMelody(melodyWifiStaDisconnected, sizeof(melodyWifiStaDisconnected) / sizeof(BuzzerNote));
}