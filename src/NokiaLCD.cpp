#if defined(ESP32) || defined(ESP32S2)
#include "driver/gpio.h"
#include "soc/gpio_struct.h"
#include <Arduino.h>
#endif
#include <stdlib.h>
#include <pgmspace.h>

#include "NokiaLCD.h"
#include "Adafruit_GFX.h"

// GPIO register access macros
#if defined(ESP32)

#define SET_PIN(mask)   GPIO.out_w1ts = (mask)
#define CLR_PIN(mask)   GPIO.out_w1tc = (mask)

#elif defined(ESP8266)

#define SET_PIN(mask)   GPOS = (mask)
#define CLR_PIN(mask)   GPOC = (mask)

#endif
#if defined(ESP32) || defined(ESP32S2) || defined(ESP8266)
#define CLK_H           SET_PIN(sclkMask)
#define CLK_L           CLR_PIN(sclkMask)
#define DATA_H          SET_PIN(sidMask)
#define DATA_L          CLR_PIN(sidMask)

// 9-bit SPI macros
#define WRITE_9(data) do {                     \
    uint32_t m = 0x100;                         \
    while(m) {                                   \
        if((data) & m) DATA_H; else DATA_L;      \
        CLK_H; CLK_L;                             \
        m >>= 1;                                   \
    }                                              \
} while(0)

#define WRITE_CMD(c)    WRITE_9(c)
#define WRITE_DATA(d)   WRITE_9(0x100 | (d))
#endif

// Chip select macros
#define CS_CLR          digitalWrite(cs, LOW)
#define CS_SET          digitalWrite(cs, HIGH)

NokiaLCD::NokiaLCD(int8_t SID, int8_t SCLK, int8_t RST, int8_t CS) 
    : Adafruit_GFX(SPFD54124B_LCDWIDTH, SPFD54124B_LCDHEIGHT) {
    sid = SID; sclk = SCLK; rst = RST; cs = CS;
}

void NokiaLCD::writeCommand(uint8_t c) {
#if defined(ESP32) || defined(ESP32S2) || defined(ESP8266)
    uint32_t sidMask = 1UL << sid, sclkMask = 1UL << sclk;
    WRITE_CMD(c);
#else
    write9(c);
#endif
}

void NokiaLCD::writeData(uint8_t d) {
#if defined(ESP32) || defined(ESP32S2) || defined(ESP8266)
    uint32_t sidMask = 1UL << sid, sclkMask = 1UL << sclk;
    WRITE_DATA(d);
#else
    write9(0x100 | d);
#endif
}

void NokiaLCD::writeColorBurst(uint16_t color, uint32_t count) {
    if (count == 0) return;

    const uint8_t hi = color >> 8;
    const uint8_t lo = color & 0xFF;

#if defined(ESP32) || defined(ESP32S2) || defined(ESP8266)
    uint32_t sidMask = 1UL << sid;
    uint32_t sclkMask = 1UL << sclk;
    while (count--) {
        WRITE_9(0x100 | hi);
        WRITE_9(0x100 | lo);
    }
#else
    while (count--) {
        write9(0x100 | hi);
        write9(0x100 | lo);
    }
#endif
}

void NokiaLCD::write9(uint16_t data) {
    for (uint16_t mask = 0x100; mask; mask >>= 1) {
        digitalWrite(sid, (data & mask) ? HIGH : LOW);
        digitalWrite(sclk, HIGH);
        digitalWrite(sclk, LOW);
    }
}

#define DELAY 0x80
PROGMEM static const unsigned char initCmd[] = {
    14,
    SPFD54124B_N_SETEXTCMD, 3, 0xFF, 0x83, 0x40,
    SPFD54124B_N_SPLOUT, DELAY, 150,
    0xCA, 3, 0x70, 0x00, 0xD9,
    0xB0, 2, 0x01, 0x11,
    0xC9, 8 + DELAY, 0x90, 0x49, 0x10, 0x28, 0x28, 0x10, 0x00, 0x06, 20,
    SPFD54124B_N_SETGAMMAP, 9, 0x60, 0x71, 0x01, 0x0E, 0x05, 0x02, 0x09, 0x31, 0x0A,
    SPFD54124B_N_SETGAMMAN, 8 + DELAY, 0x67, 0x30, 0x61, 0x17, 0x48, 0x07, 0x05, 0x33, 10,
    SPFD54124B_N_SETPWCTR5, 3, 0x35, 0x20, 0x45,
    SPFD54124B_N_SETPWCTR4, 3 + DELAY, 0x33, 0x25, 0x4c, 10,
    SPFD54124B_N_COLMOD, 1, 0x05,
    SPFD54124B_N_DISPON, DELAY, 10,
    SPFD54124B_N_CASET, 4, 0x00, 0x00, 0x00, 0xaf,
    SPFD54124B_N_PASET, 4, 0x00, 0x00, 0x00, 0xdb,
    SPFD54124B_N_RAMWR, 0,
};

void NokiaLCD::begin() {
    pinMode(rst, OUTPUT); pinMode(cs, OUTPUT);
    pinMode(sclk, OUTPUT); pinMode(sid, OUTPUT);
    CS_CLR; digitalWrite(rst, HIGH);
    digitalWrite(sclk, LOW); digitalWrite(sid, LOW);

    digitalWrite(rst, HIGH); delay(100);
    digitalWrite(rst, LOW);  delay(50);
    digitalWrite(rst, HIGH); delay(50);

    CS_CLR;
    
    uint8_t numCmds = pgm_read_byte(initCmd);
    const uint8_t* addr = initCmd + 1;
    
    while (numCmds--) {
        writeCommand(pgm_read_byte(addr++));
        uint8_t numArgs = pgm_read_byte(addr++);
        uint8_t delayMs = numArgs & DELAY;
        numArgs &= ~DELAY;
        
        while (numArgs--) writeData(pgm_read_byte(addr++));
        if (delayMs) delay(pgm_read_byte(addr++));
    }
  writeCommand(SPFD54124B_N_MADCTL); 
  writeData(0x00);               // RGB //0x08= BGR
    CS_SET;
}

inline void NokiaLCD::setWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    CS_CLR;
    
    if (rotation) {
        uint8_t w = WIDTH-1, h = HEIGHT-1, t;
        switch (rotation) {
            case 1: t = w - y1; y1 = x1; x1 = w - y0; y0 = x0; x0 = t; break;
            case 2: t = x0; x0 = w - x1; x1 = w - t; t = y0; y0 = h - y1; y1 = h - t; break;
            case 3: t = h - x1; x1 = y1; y1 = h - x0; x0 = y0; y0 = t; break;
        }
    }
    
    writeCommand(SPFD54124B_N_CASET);
    writeData(0); writeData(x0);
    writeData(0); writeData(x1);
    
    writeCommand(SPFD54124B_N_PASET);
    writeData(0); writeData(y0);
    writeData(0); writeData(y1);
    
    writeCommand(SPFD54124B_N_RAMWR);
    CS_SET;
}

void NokiaLCD::fillScreen(uint16_t c) {
    setWindow(0, 0, _width - 1, _height - 1);
    CS_CLR;
    writeColorBurst(c, (uint32_t)_width * (uint32_t)_height);
    CS_SET;
}

void NokiaLCD::pushColor(uint16_t c) {
    CS_CLR;
    writeColorBurst(c, 1);
    CS_SET;
}

void NokiaLCD::drawPixel(int16_t x, int16_t y, uint16_t c) {
    if (x<0 || x>=_width || y<0 || y>=_height) return;
    setWindow(x, y, x, y);
    CS_CLR;
    writeColorBurst(c, 1);
    CS_SET;
}

void NokiaLCD::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t c) {
    if (x<0 || x>=_width || y>=_height) return;
    int16_t y2 = y + h - 1;
    if (y2 < 0) return;
    if (y2 >= _height) h = _height - y;
    if (y < 0) { h += y; y = 0; }

    setWindow(x, y, x, y + h - 1);
    CS_CLR;
    writeColorBurst(c, (uint32_t)h);
    CS_SET;
}

void NokiaLCD::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t c) {
    if (y<0 || y>=_height || x>=_width) return;
    int16_t x2 = x + w - 1;
    if (x2 < 0) return;
    if (x2 >= _width) w = _width - x;
    if (x < 0) { w += x; x = 0; }

    setWindow(x, y, x + w - 1, y);
    CS_CLR;
    writeColorBurst(c, (uint32_t)w);
    CS_SET;
}

void NokiaLCD::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c) {
    if (x>=_width || y>=_height) return;
    int16_t x2 = x + w - 1, y2 = y + h - 1;
    if (x2<0 || y2<0) return;
    if (x2 >= _width) w = _width - x;
    if (x < 0) { w += x; x = 0; }
    if (y2 >= _height) h = _height - y;
    if (y < 0) { h += y; y = 0; }

    if (w <= 0 || h <= 0) return;

    setWindow(x, y, x + w - 1, y + h - 1);
    CS_CLR;
    writeColorBurst(c, (uint32_t)w * (uint32_t)h);
    CS_SET;
}
uint16_t NokiaLCD::Color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

uint16_t NokiaLCD:: pushFastPixel(uint32_t length, const void* color)
{
    const uint16_t* colors = (const uint16_t*)color;

    CS_CLR;
    writePixels(colors, length);
    CS_SET;

    return (uint16_t)length;
}

void NokiaLCD::writePixels(const uint16_t* colors, uint32_t length)
{
    uint32_t sidMask = 1UL << sid;
    uint32_t sclkMask = 1UL << sclk;

    while (length--) {
        uint16_t color = pgm_read_word(colors++);
#if defined(ESP32) || defined(ESP32S2) || defined(ESP8266)
        WRITE_DATA(color >> 8);
        WRITE_DATA(color & 0xFF);
#else
        write9(0x100 | (color >> 8));
        write9(0x100 | (color & 0xFF));
#endif
    }
}

void NokiaLCD::drawRGBBitmap(int16_t x, int16_t y, uint16_t bitmap[], int16_t w, int16_t h)
{
    drawRGBBitmap(x, y, (const uint16_t*)bitmap, w, h);
}

void NokiaLCD::drawRGBBitmap(int16_t x, int16_t y, const uint16_t bitmap[], int16_t w, int16_t h)
{
    if (bitmap == nullptr || w <= 0 || h <= 0) return;

    if (rotation != 0) {
        Adafruit_GFX::drawRGBBitmap(x, y, bitmap, w, h);
        return;
    }

    int32_t left = x < 0 ? -(int32_t)x : 0;
    int32_t top = y < 0 ? -(int32_t)y : 0;
    int32_t right = (int32_t)x + w;
    int32_t bottom = (int32_t)y + h;
    if (right > _width) right = _width;
    if (bottom > _height) bottom = _height;
    if (left >= right || top >= bottom) return;

    int32_t drawX = (int32_t)x + left;
    int32_t drawY = (int32_t)y + top;
    uint32_t drawWidth = (uint32_t)(right - drawX);
    uint32_t drawHeight = (uint32_t)(bottom - drawY);

    setWindow((uint8_t)drawX, (uint8_t)drawY,
              (uint8_t)(right - 1), (uint8_t)(bottom - 1));
    CS_CLR;

    if (left == 0 && drawWidth == (uint32_t)w) {
        writePixels(bitmap + (uint32_t)top * (uint32_t)w, drawWidth * drawHeight);
    } else {
        for (uint32_t row = 0; row < drawHeight; ++row) {
            const uint16_t* rowPixels = bitmap + (uint32_t)(top + row) * (uint32_t)w + (uint32_t)left;
            writePixels(rowPixels, drawWidth);
        }
    }

    CS_SET;
}

void NokiaLCD::pushImage(int16_t x, int16_t y, int16_t w, int16_t h, const void* data) 
{ 
    if (w <= 0 || h <= 0 || data == nullptr) {
        return; // Invalid parameters
    }
    drawRGBBitmap(x, y, (const uint16_t*)data, w, h);
}