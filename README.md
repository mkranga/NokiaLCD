# NokiaLCD SPFD54124B

Arduino graphics driver for 128 x 160 Nokia LCD modules using the SPFD54124B controller. The API is named `NokiaLCD` and is built on Adafruit_GFX.

This is a community rebrand and update of the original library, not an official Nokia product or an upstream release.

## Features

- Adafruit_GFX text, fonts, shapes, and drawing primitives.
- RGB565 bitmap drawing with one address-window setup and a direct pixel stream.
- Fast horizontal and vertical lines, rectangles, and screen fills.
- Four-pin software serial transport for the controller's 9-bit command/data protocol.
- No custom SPI implementation; only Arduino GPIO APIs and Adafruit_GFX are required.

The controller embeds the command/data indicator as a ninth serial bit. Standard Arduino SPI APIs send 8-bit words and do not provide a portable 9-bit transfer across AVR, ESP8266, and ESP32, so this driver uses software GPIO serial rather than claiming hardware-SPI support.

## Install

Install Adafruit GFX Library using the Arduino Library Manager, then copy this library into the Arduino `libraries` directory. Include `NokiaLCD.h` in your sketch.

## Wiring

| Display signal | Constructor argument |
| --- | --- |
| SDA / MOSI | `SID` |
| SCK | `SCLK` |
| RESET | `RST` |
| CS | `CS` |

Connect display power and ground according to the module's electrical requirements. The constructor order is `NokiaLCD(SID, SCLK, RST, CS)`. There is no separate D/C pin because command/data selection is sent as the ninth serial bit.

## Quick Start

```cpp
#include <NokiaLCD.h>

constexpr int TFT_SID = 13;
constexpr int TFT_SCLK = 14;
constexpr int TFT_RST = 12;
constexpr int TFT_CS = 15;

NokiaLCD display(TFT_SID, TFT_SCLK, TFT_RST, TFT_CS);

void setup() {
  display.begin();
  display.fillScreen(0x0000);
  display.setTextColor(0xFFFF);
  display.setCursor(10, 20);
  display.print("Hello, NokiaLCD");
  display.drawFastHLine(8, 40, 112, 0x07E0);
}

void loop() {}
```

The pin numbers above are examples for a Wemos D1 mini. Select pins that match your board and wiring. `init()` is also available as an alias for `begin()`.

## Ben 10 Example

Open `examples/Ben10Demo/Ben10Demo.ino` for a self-contained green-and-black Omnitrix-inspired graphics demo. It uses only Adafruit_GFX primitives and does not require a large bitmap in RAM or flash.

## Bitmap Performance

Use `drawRGBBitmap()` for images. The optimized unmasked RGB565 overload clips the image, sets the address window once, and streams pixels directly. Horizontal-line drawing is useful for geometric lines, but is inefficient for raster images when each color run requires a new display window.

On one ESP8266 test with the included 120 x 159 images, direct bitmap drawing measured about 130ms(7x speed boost).

For bitmaps stored in `PROGMEM`, the driver reads pixels using the platform's program-memory access helper.

## Attribution

The original SPFD54124B Arduino library was published by [Amirhm-e](https://github.com/Amirhm-e). Original source comments also credit Abhishek Tiwari. This rebrand retains the original source attribution and copyright notices.

- [Original upstream repository](https://github.com/Amirhm-e/SPFD54124B-library-for-Arduino-esp8266-esp32)
- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)


## License Status

The original source headers state GNU LGPL version 3 or later. The upstream README claims MIT, but the upstream repository has no `LICENSE` file. This rebrand preserves the source notices and does not declare a new license. Confirm redistribution terms with the original copyright holders before publishing or creating a release.