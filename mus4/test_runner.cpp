#include "test_runner.h"

// Static member definitions
TestRegistry::TestInfo TestRegistry::tests[TestRegistry::MAX_TESTS];
int TestRegistry::count = 0;
bool TestRegistry::currentTestFailed = false;

bool TestRegistry::registerTest(const char* suite, const char* name, TestFunc func) {
    if (count < MAX_TESTS) {
        tests[count++] = {suite, name, func};
        return true;
    }
    return false;
}

void TestRegistry::fail(const char* file, int line, const char* msg) {
    Serial.printf("[FAIL] %s:%d: %s\n", file, line, msg);
    currentTestFailed = true;
}

void TestRegistry::runAll() {
    Serial.println("[TEST] Running all tests...");
    int passed = 0;
    int failed = 0;
    
    for (int i = 0; i < count; i++) {
        currentTestFailed = false;
        Serial.printf("[RUN ] %s.%s\n", tests[i].suite, tests[i].name);
        unsigned long start = millis();
        tests[i].func();
        unsigned long duration = millis() - start;
        
        if (!currentTestFailed) {
            Serial.printf("[PASS] %s.%s (%lums)\n", tests[i].suite, tests[i].name, duration);
            passed++;
        } else {
            // Failure already logged
            failed++;
        }
    }
    Serial.printf("[DONE] Total: %d, Passed: %d, Failed: %d\n", count, passed, failed);
}
