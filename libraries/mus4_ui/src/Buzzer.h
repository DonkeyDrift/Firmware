#pragma once
#include <Arduino.h>

#define BUZZER_MODE_MANUAL 0
#define BUZZER_MODE_SEMI_AUTO 1
#define BUZZER_MODE_FULL_AUTO 2
#define BUZZER_PARK_LOCK 3
#define BUZZER_PARK_UNLOCK 4

#define BUZZER_VOLUME 40
#define BUZZER_SOUND_ENABLED 0

// 音符定义
#define NOTE_REST 0
#define NOTE_C4 262
#define NOTE_E4 330
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 494
#define NOTE_D5 587

// 音符时值定义
#define N8 8
#define N4 4

struct BuzzerNote {
    int pitch;
    int duration;
};

class Buzzer {
private:
    bool _playing = false;
    bool _soundEnabled;
    int _pin;
    int _channel;
    int _volume;
    static int _channelCounter;

    // Non-blocking melody state machine
    const BuzzerNote* _currentMelody = nullptr;
    int _melodyLength = 0;
    int _noteIndex = 0;
    unsigned long _noteStartMs = 0;
    unsigned long _noteDurationMs = 0;
    unsigned long _restDurationMs = 0;
    int _lastPitch = NOTE_REST;

    void startNote(int pitch, unsigned long durationMs);
    void startMelody(const BuzzerNote* melody, int length);
    void stopTone();

public:
    Buzzer(int pin);
    void playModeSound(int mode);
    void playParkLockSound();
    void playParkUnlockSound();
    bool isPlaying() { return _playing; }
    void update();
    void setVolume(int volume);
};