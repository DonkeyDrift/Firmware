#pragma once

#include <Arduino.h>

// Minimal Unit Testing Framework mimicking Google Test API
// This allows the tests to be portable if we ever get a PC compiler

#define TEST(suite, name) \
    void test_##suite##_##name(); \
    bool register_##suite##_##name = TestRegistry::registerTest(#suite, #name, test_##suite##_##name); \
    void test_##suite##_##name()

#define ASSERT_EQ(expected, actual) \
    if ((expected) != (actual)) { \
        TestRegistry::fail(__FILE__, __LINE__, "ASSERT_EQ failed: " #expected " != " #actual); \
        return; \
    }

#define ASSERT_TRUE(condition) \
    if (!(condition)) { \
        TestRegistry::fail(__FILE__, __LINE__, "ASSERT_TRUE failed: " #condition); \
        return; \
    }

#define EXPECT_EQ(expected, actual) \
    if ((expected) != (actual)) { \
        TestRegistry::fail(__FILE__, __LINE__, "EXPECT_EQ failed: " #expected " != " #actual); \
    }

class TestRegistry {
public:
    typedef void (*TestFunc)();
    struct TestInfo {
        const char* suite;
        const char* name;
        TestFunc func;
    };

    static bool registerTest(const char* suite, const char* name, TestFunc func);
    static void fail(const char* file, int line, const char* msg);
    static void runAll();

private:
    static const int MAX_TESTS = 50;
    static TestInfo tests[MAX_TESTS];
    static int count;
    static bool currentTestFailed;
};

// Mock Serial for testing output
class MockPrint : public Print {
public:
    String buffer = "";
    virtual size_t write(uint8_t c) {
        buffer += (char)c;
        return 1;
    }
    virtual size_t write(const uint8_t *buffer, size_t size) {
        for(size_t i=0; i<size; i++) this->buffer += (char)buffer[i];
        return size;
    }
    void clear() { buffer = ""; }
    String getOutput() { return buffer; }
};
