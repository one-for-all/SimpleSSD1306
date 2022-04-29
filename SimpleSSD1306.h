#ifndef _SimpleSSD1306_H_
#define _SimpleSSD1306_H_

#include <inttypes.h>
#include <Wire.h>
#include "Print.h"

#define SSD1306_BLACK 0 ///< Draw 'off' pixels
#define SSD1306_WHITE 1 ///< Draw 'on' pixels

#define SSD1306_MEMORYMODE 0x20          ///< See datasheet
#define SSD1306_COLUMNADDR 0x21          ///< See datasheet
#define SSD1306_PAGEADDR 0x22            ///< See datasheet
#define SSD1306_SEGREMAP 0xA0            ///< See datasheet
#define SSD1306_SETCONTRAST 0x81         ///< See datasheet
#define SSD1306_DISPLAYALLON_RESUME 0xA4 ///< See datasheet
#define SSD1306_NORMALDISPLAY 0xA6       ///< See datasheet
#define SSD1306_COMSCANDEC 0xC8          ///< See datasheet
#define SSD1306_DISPLAYOFF 0xAE          ///< See datasheet
#define SSD1306_DISPLAYON 0xAF           ///< See datasheet
#define SSD1306_SETSTARTLINE 0x40        ///< See datasheet
#define SSD1306_CHARGEPUMP 0x8D          ///< See datasheet
#define SSD1306_SETDISPLAYOFFSET 0xD3    ///< See datasheet
#define SSD1306_SETMULTIPLEX 0xA8        ///< See datasheet
#define SSD1306_SETCOMPINS 0xDA          ///< See datasheet
#define SSD1306_SETPRECHARGE 0xD9        ///< See datasheet
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5  ///< See datasheet
#define SSD1306_SETVCOMDETECT 0xDB       ///< See datasheet

#define SSD1306_EXTERNALVCC 0x01  ///< External display voltage source
#define SSD1306_SWITCHCAPVCC 0x02 ///< Gen. display voltage from 3.3V

#define SSD1306_DEACTIVATE_SCROLL 0x2E ///< Stop scroll

class SimpleSSD1306 : public Print
{
public:
    SimpleSSD1306(int16_t width, int16_t height);
    bool begin(uint8_t vcs, uint8_t i2caddr);
    void clearDisplay(void);
    void display(void);

    using Print::write;
    virtual size_t write(uint8_t);

protected:
    void ssd1306_command1(uint8_t c);
    void ssd1306_commandList(const uint8_t *c, uint8_t n);
    void drawChar(int16_t x, int16_t y, unsigned char c);
    void drawPixel(int16_t x, int16_t y);

    int16_t WIDTH;                        ///< This is the 'raw' display width - never changes
    int16_t HEIGHT;                       ///< This is the 'raw' display height - never changes
    int16_t cursor_x = 0;                 ///< x location to start print()ing text
    int16_t cursor_y = 0;                 ///< y location to start print()ing text
    uint16_t textcolor = SSD1306_WHITE;   ///< 16-bit background color for print()
    uint16_t textbgcolor = SSD1306_BLACK; ///< 16-bit text color for print()
    uint8_t textsize_x = 1;               ///< Desired magnification in X-axis of text to print()
    uint8_t textsize_y = 1;               ///< Desired magnification in Y-axis of text to print()
    TwoWire *wire = &Wire;

    uint8_t *buffer;  ///< Buffer data used for display buffer. Allocated when
                      ///< begin method is called.
    int8_t vccstate;  ///< VCC selection, set by begin method.
    int8_t i2caddr;   ///< I2C address initialized when begin method is called.
    uint8_t contrast; ///< normal contrast setting for this device
};

#endif // _SimpleSSD1306_H_