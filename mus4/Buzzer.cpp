#include "Buzzer.h"
#include "SharedTypes.h"

// 手动模式旋律 - 稳定感，大三和弦 C4-E4-G4
Note melodyManual[] = {
    { NOTE_C4, N8 },
    { NOTE_E4, N8 },
    { NOTE_G4, N4 },
    { NOTE_REST, N8 }
};

// 半自动模式旋律 - 过渡感，中音跳跃 E4-G4-A4
Note melodySemiAuto[] = {
    { NOTE_E4, N8 },
    { NOTE_G4, N8 },
    { NOTE_A4, N4 },
    { NOTE_REST, N8 }
};

// 全自动模式旋律 - 科技感，高音递进 G4-B4-D5
Note melodyFullAuto[] = {
    { NOTE_G4, N8 },
    { NOTE_B4, N8 },
    { NOTE_D5, N4 },
    { NOTE_REST, N8 }
};

// 锁定提示音 - 下降音阶 G4-E4-C4
Note melodyParkLock[] = {
    { NOTE_G4, N8 },
    { NOTE_E4, N8 },
    { NOTE_C4, N4 },
    { NOTE_REST, N8 }
};

// 解锁提示音 - 上升音阶 C4-E4-G4
Note melodyParkUnlock[] = {
    { NOTE_C4, N8 },
    { NOTE_E4, N8 },
    { NOTE_G4, N4 },
    { NOTE_REST, N8 }
};

Buzzer::Buzzer(int pin) : _player(pin, 120) {
}

void Buzzer::playModeSound(int mode) {
    _playing = true;
    switch (mode) {
        case CAR_MODE_MANUAL:
            _player.play(melodyManual, sizeof(melodyManual) / sizeof(Note));
            break;
        case CAR_MODE_SEMI_AUTO:
            _player.play(melodySemiAuto, sizeof(melodySemiAuto) / sizeof(Note));
            break;
        case CAR_MODE_FULL_AUTO:
            _player.play(melodyFullAuto, sizeof(melodyFullAuto) / sizeof(Note));
            break;
    }
    _playing = false;
}

void Buzzer::playParkLockSound() {
    _playing = true;
    _player.play(melodyParkLock, sizeof(melodyParkLock) / sizeof(Note));
    _playing = false;
}

void Buzzer::playParkUnlockSound() {
    _playing = true;
    _player.play(melodyParkUnlock, sizeof(melodyParkUnlock) / sizeof(Note));
    _playing = false;
}

void Buzzer::update() {
    // 非阻塞更新接口，当前实现为阻塞播放，后续可扩展
}