#include "Buzzer.h"
#include "SharedTypes.h"

int Buzzer::_channelCounter = 2; // 从2开始避免与PWM通道(0,1)冲突

// 手动模式旋律 - 稳定感，大三和弦 C4-E4-G4
BuzzerNote melodyManual[] = {
    { NOTE_C4, N8 },
    { NOTE_E4, N8 },
    { NOTE_G4, N4 },
    { NOTE_REST, N8 }
};

// 半自动模式旋律 - 过渡感，中音跳跃 E4-G4-A4
BuzzerNote melodySemiAuto[] = {
    { NOTE_E4, N8 },
    { NOTE_G4, N8 },
    { NOTE_A4, N4 },
    { NOTE_REST, N8 }
};

// 全自动模式旋律 - 科技感，高音递进 G4-B4-D5
BuzzerNote melodyFullAuto[] = {
    { NOTE_G4, N8 },
    { NOTE_B4, N8 },
    { NOTE_D5, N4 },
    { NOTE_REST, N8 }
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
    ledcAttachChannel(_pin, 2000, 8, _channel);
    setVolume(_volume);
}

void Buzzer::setVolume(int volume) {
    _volume = constrain(volume, 0, 100);
}

void Buzzer::playNoteWithVolume(int pitch, int durationMs) {
    if (pitch == 0) {
        ledcWriteChannel(_channel, 0);
    } else {
        ledcChangeFrequency(_pin, pitch, 8);
        int dutyCycle = _volume * 255 / 100;
        ledcWriteChannel(_channel, dutyCycle);
        delay(durationMs);
        ledcWriteChannel(_channel, 0);
    }
}

void Buzzer::playMelody(const BuzzerNote* melody, int length) {
    _playing = true;
    int beatMs = 60000 / 120;
    for (int i = 0; i < length; i++) {
        int durMs = beatMs * 4 / melody[i].duration;
        playNoteWithVolume(melody[i].pitch, durMs);
        delay(durMs * 0.3);
    }
    _playing = false;
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