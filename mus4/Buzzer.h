#pragma once
#include <Arduino.h>
#include "ESP_Music.h"

#define BUZZER_MODE_MANUAL 0
#define BUZZER_MODE_SEMI_AUTO 1
#define BUZZER_MODE_FULL_AUTO 2
#define BUZZER_PARK_LOCK 3
#define BUZZER_PARK_UNLOCK 4

class Buzzer {
private:
    MusicPlayer _player;
    bool _playing = false;
    
public:
    Buzzer(int pin);
    void playModeSound(int mode);
    void playParkLockSound();
    void playParkUnlockSound();
    bool isPlaying() { return _playing; }
    void update();
};