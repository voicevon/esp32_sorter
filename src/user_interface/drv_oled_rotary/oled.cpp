#include "oled.h"

OLED* OLED::instance = nullptr;

OLED::OLED() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1), isDisplayAvailable(false), lastUpdateTime(0) {}

OLED* OLED::getInstance() {
    if (instance == nullptr) {
        instance = new OLED();
    }
    return instance;
}

void OLED::initialize() {
    Serial.printf("[OLED] Initializing Wire on SDA=%d, SCL=%d...\n", PIN_OLED_SDA, PIN_OLED_SCL);
    Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
    if (display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
        isDisplayAvailable = true;
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("OLED Init Success");
        display.display();
        Serial.println("[OLED] SSD1306 Init SUCCESS");
    } else {
        isDisplayAvailable = false;
        Serial.println("[OLED] SSD1306 Init FAILED!");
    }
}

void OLED::clearDisplay() {
    if (!isDisplayAvailable) return;
    display.clearDisplay();
    display.display();
}

void OLED::renderMenu(MenuNode* node, int cursorIndex, int scrollOffset) {
    if (!isDisplayAvailable) return;
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("=== MENU SYSTEM ===");
    display.display();
}

void OLED::refresh(const DisplaySnapshot& snapshot) {
    if (!isDisplayAvailable) return;
    
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    
    // 渲染页眉
    display.printf("[%s]\n", snapshot.activePage);
    display.drawFastHLine(0, 9, 128, SSD1306_WHITE);
    display.setCursor(0, 12);
    
    switch (snapshot.currentMode) {
        case APP_PRODUCTION:
            display.printf("Spd: %.1f/s\n", snapshot.data.dashboard.sortingSpeedPerSecond);
            display.printf("Ident: %d\n", snapshot.data.dashboard.identifiedCount);
            display.printf("Trays: %d\n", snapshot.data.dashboard.transportedTrayCount);
            display.printf("LastDia: %d mm\n", snapshot.data.dashboard.latestDiameter);
            break;
            
        case APP_DIAG_ENCODER:
        case APP_DIAG_HMI:
            display.printf("Raw: %d\n", snapshot.data.encoder.raw);
            display.printf("Logic: %d\n", snapshot.data.encoder.logic);
            display.printf("Z Correct: %d\n", snapshot.data.encoder.zeroCount);
            display.printf("Z Error: %d\n", snapshot.data.encoder.zeroTotal);
            display.printf("Offset: %d\n", snapshot.data.encoder.offset);
            break;
            
        case APP_DIAG_SCANNER:
            display.printf("Laser States: 0x%02X\n", snapshot.data.scanner.states);
            display.printf("Samples: %d\n\n", snapshot.data.scanner.sampleCount);
            for (int i = 0; i < 4; i++) {
                display.printf("L%d:", i+1);
                for (int bit = 0; bit < 20; bit++) {
                    int byteIdx = bit / 8;
                    int bitIdx = bit % 8;
                    bool active = snapshot.data.scanner.history[i][byteIdx] & (1 << (7 - bitIdx));
                    display.print(active ? "|" : ".");
                }
                display.print("\n");
            }
            break;
            
        case APP_DIAG_OUTLET:
        case APP_CONFIG_DIAMETER:
            display.printf("Active Outlet: O%d\n", snapshot.data.outlet.activeOutletIndex + 1);
            display.printf("SubMode: %d\n", snapshot.data.outlet.subMode);
            display.printf("Cycles: %u\n", snapshot.data.outlet.cycleCount);
            display.print("Outlets: ");
            for (int i = 0; i < 8; i++) {
                display.print(snapshot.data.outlet.outlets[i].isOpen ? "O" : ".");
            }
            display.print("\n");
            break;
            
        default:
            display.println("Unknown Screen State");
            break;
    }
    display.display();
}
