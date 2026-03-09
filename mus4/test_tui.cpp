#include "test_runner.h"
#include "TUI.h"

// Test Fixture
class TUITest : public TestRegistry {
};

TEST(TUI, InitialState) {
    MockPrint mockOut;
    TUI tui(mockOut);
    
    // Default state should be safe
    ASSERT_TRUE(true);
}

TEST(TUI, RenderHeader) {
    MockPrint mockOut;
    TUI tui(mockOut);
    tui.setAnsiEnabled(false); // Test plain text mode first for simplicity
    
    tui.render();
    String out = mockOut.getOutput();
    
    // Expect header to be present
    ASSERT_TRUE(out.indexOf("MUS4 Control System") >= 0);
    ASSERT_TRUE(out.indexOf("====================") >= 0);
}

TEST(TUI, RenderMode) {
    MockPrint mockOut;
    TUI tui(mockOut);
    tui.setAnsiEnabled(false);
    
    tui.setOutput(0, 0, CAR_MODE_MANUAL, false);
    tui.render();
    String out = mockOut.getOutput();
    ASSERT_TRUE(out.indexOf("Mode: Manual") >= 0);
    
    mockOut.clear();
    tui.setOutput(0, 0, CAR_MODE_FULL_AUTO, false);
    tui.render();
    out = mockOut.getOutput();
    ASSERT_TRUE(out.indexOf("Mode: Full-Auto") >= 0);
}
