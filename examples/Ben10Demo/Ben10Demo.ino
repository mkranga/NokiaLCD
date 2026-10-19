#include <NokiaLCD.h>

constexpr int TFT_SID = 13;
constexpr int TFT_SCLK = 14;
constexpr int TFT_RST = 12;
constexpr int TFT_CS = 15;

constexpr uint16_t BLACK = 0x0000;
constexpr uint16_t WHITE = 0xFFFF;
constexpr uint16_t OMNITRIX_GREEN = 0x07E0;

NokiaLCD display(TFT_SID, TFT_SCLK, TFT_RST, TFT_CS);

void setup() {
  display.begin();
  display.fillScreen(BLACK);

  display.setTextColor(OMNITRIX_GREEN);
  display.setTextSize(2);
  display.setCursor(24, 12);
  display.print("BEN 10");

  display.drawRoundRect(28, 48, 72, 94, 12, WHITE);
  display.drawCircle(64, 94, 27, OMNITRIX_GREEN);
  display.fillCircle(64, 94, 20, OMNITRIX_GREEN);
  display.fillCircle(64, 94, 12, BLACK);
  display.drawFastHLine(42, 94, 44, WHITE);
  display.drawFastVLine(64, 72, 44, WHITE);
}

void loop() {}