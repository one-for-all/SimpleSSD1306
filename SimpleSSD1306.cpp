#include "SimpleSSD1306.h"
#include "glcdfont.c"
#include <Wire.h>
#include <Arduino.h>

#ifdef __AVR__
#include <avr/pgmspace.h>
#else
#error __AVR__ needs to be defined // This library only works with AVR processor
#endif

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifdef BUFFER_LENGTH
#define WIRE_MAX min(256, BUFFER_LENGTH) ///< AVR or similar Wire lib
#else
#error BUFFER_LENGTH needs to be defined in Wire.h
#endif

#if ARDUINO >= 100
#define WIRE_WRITE wire->write ///< Wire write function in recent Arduino lib
#else
#error ARDUINO needs to be >= 100 // This library only works with recent Aduino lib
#endif

SimpleSSD1306::SimpleSSD1306(int16_t width, int16_t height) : WIDTH(width), HEIGHT(height)
{
}

/** Allocates RAM for image buffer, initialize peripherals and pins.
    @param  vcs
            VCC selection. Pass SSD1306_SWITCHCAPVCC to generate the display
            voltage (step up) from the 3.3V source, or SSD1306_EXTERNALVCC
            otherwise. Most situations with Adafruit SSD1306 breakouts will
            want SSD1306_SWITCHCAPVCC.
    @param  addr
            I2C address of corresponding SSD1306 display (or pass 0 to use
            default of 0x3C for 128x32 display, 0x3D for all others).
    @return true on successful allocation/init, false otherwise.
            Well-behaved code should check the return value before
            proceeding.
    @note   MUST call this function before any drawing or updates!
*/
bool SimpleSSD1306::begin(uint8_t vcs, uint8_t i2caddr)
{
    // Allocate buffer of number of bytes required for the screen, where each
    // bit is one pixel
    if ((!buffer) && !(buffer = (uint8_t *)malloc(WIDTH * ((HEIGHT + 7) / 8))))
        return false;

    vccstate = vcs;
    this->i2caddr = i2caddr;
    wire->begin();

    // Init sequence
    static const uint8_t init1[] PROGMEM = {SSD1306_DISPLAYOFF,         // 0xAE
                                            SSD1306_SETDISPLAYCLOCKDIV, // 0xD5
                                            0x80,                       // the suggested ratio 0x80
                                            SSD1306_SETMULTIPLEX};      // 0xA8
    ssd1306_commandList(init1, sizeof(init1));
    ssd1306_command1(HEIGHT - 1);

    static const uint8_t init2[] PROGMEM = {SSD1306_SETDISPLAYOFFSET,   // 0xD3
                                            0x0,                        // no offset
                                            SSD1306_SETSTARTLINE | 0x0, // line #0
                                            SSD1306_CHARGEPUMP};        // 0x8D
    ssd1306_commandList(init2, sizeof(init2));
    ssd1306_command1((vccstate == SSD1306_EXTERNALVCC) ? 0x10 : 0x14);

    static const uint8_t init3[] PROGMEM = {SSD1306_MEMORYMODE, // 0x20
                                            0x00,               // 0x0 act like ks0108
                                            SSD1306_SEGREMAP | 0x1,
                                            SSD1306_COMSCANDEC};
    ssd1306_commandList(init3, sizeof(init3));

    uint8_t comPins = 0x02;
    contrast = 0x8F;
    if ((WIDTH == 128) && (HEIGHT == 32))
    {
        comPins = 0x02;
        contrast = 0x8F;
    }
    else if ((WIDTH == 128) && (HEIGHT == 64))
    {
        comPins = 0x12;
        contrast = (vccstate == SSD1306_EXTERNALVCC) ? 0x9F : 0xCF;
    }
    else if ((WIDTH == 96) && (HEIGHT == 16))
    {
        comPins = 0x2; // ada x12
        contrast = (vccstate == SSD1306_EXTERNALVCC) ? 0x10 : 0xAF;
    }
    else
    {
        // Other screen varieties -- TBD
    }

    ssd1306_command1(SSD1306_SETCOMPINS);
    ssd1306_command1(comPins);
    ssd1306_command1(SSD1306_SETCONTRAST);
    ssd1306_command1(contrast);

    ssd1306_command1(SSD1306_SETPRECHARGE); // 0xd9
    ssd1306_command1((vccstate == SSD1306_EXTERNALVCC) ? 0x22 : 0xF1);
    static const uint8_t PROGMEM init5[] = {
        SSD1306_SETVCOMDETECT, // 0xDB
        0x40,
        SSD1306_DISPLAYALLON_RESUME, // 0xA4
        SSD1306_NORMALDISPLAY,       // 0xA6
        SSD1306_DEACTIVATE_SCROLL,
        SSD1306_DISPLAYON}; // Main screen turn on
    ssd1306_commandList(init5, sizeof(init5));

    return true; // Success
}

/** Sends a single command to SSD1306. */
void SimpleSSD1306::ssd1306_command1(uint8_t c)
{
    wire->beginTransmission(i2caddr);
    WIRE_WRITE((uint8_t)0x00); // Co = 0, D/C = 0
    WIRE_WRITE(c);
    wire->endTransmission();
}

/** Sends a list of commands to SSD1306. */
void SimpleSSD1306::ssd1306_commandList(const uint8_t *c, uint8_t n)
{
    wire->beginTransmission(i2caddr);
    WIRE_WRITE((uint8_t)0x00); // Co = 0, D/C = 0
    uint16_t bytesOut = 1;
    while (n--)
    {
        if (bytesOut >= WIRE_MAX)
        {
            wire->endTransmission();
            wire->beginTransmission(i2caddr);
            WIRE_WRITE((uint8_t)0x00); // Co = 0, D/C = 0
            bytesOut = 1;
        }
        WIRE_WRITE(pgm_read_byte(c++));
        bytesOut++;
    }
    wire->endTransmission();
}

/** Clears the buffer. */
void SimpleSSD1306::clearDisplay(void)
{
    memset(buffer, 0, WIDTH * ((HEIGHT + 7) / 8));
}

/** Displays the screen with pixels in buffer. */
void SimpleSSD1306::display(void)
{
    static const uint8_t dlist1[] PROGMEM = {
        SSD1306_PAGEADDR,
        0,                      // Page start address
        0xFF,                   // Page end (not really, but works here)
        SSD1306_COLUMNADDR, 0}; // Column start address
    ssd1306_commandList(dlist1, sizeof(dlist1));
    ssd1306_command1(WIDTH - 1); // Column end address

    uint16_t count = WIDTH * ((HEIGHT + 7) / 8);
    uint8_t *ptr = buffer;

    wire->beginTransmission(i2caddr);
    WIRE_WRITE((uint8_t)0x40);
    uint16_t bytesOut = 1;
    while (count--)
    {
        if (bytesOut >= WIRE_MAX)
        {
            wire->endTransmission();
            wire->beginTransmission(i2caddr);
            WIRE_WRITE((uint8_t)0x40);
            bytesOut = 1;
        }
        WIRE_WRITE(*ptr++);
        bytesOut++;
    }
    wire->endTransmission();
}

/** Prints one byte/character of data, used to support print(), where c is the
 *  8-bit ascii character to write.
 */
size_t SimpleSSD1306::write(uint8_t c)
{
    if (c == '\n') // Newline?
    {
        cursor_x = 0;               // Reset x to zero,
        cursor_y += textsize_y * 8; // advance y one line
    }
    else if (c != '\r')
    { // Ignore carriage returns
        if ((cursor_x + textsize_x * 6) > WIDTH)
        {                               // Off right?
            cursor_x = 0;               // Reset x to zero,
            cursor_y += textsize_y * 8; // advance y one line
        }
        drawChar(cursor_x, cursor_y, c);
        cursor_x += textsize_x * 6; // Advance x one char
    }

    return 1;
}

/** Draws a character c with (x, y) as bottom left corner. */
void SimpleSSD1306::drawChar(int16_t x, int16_t y, unsigned char c)
{
    if ((x >= WIDTH) ||                   // Clip right
        (y >= HEIGHT) ||                  // Clip bottom
        ((x + 6 * textsize_x - 1) < 0) || // Clip left
        ((y + 8 * textsize_y - 1) < 0))   // Clip top
        return;

    if (c >= 176)
        c++; // Handle 'classic' charset behavior

    for (int8_t i = 0; i < 5; i++)
    { // Char bitmap = 5 columns
        uint8_t line = pgm_read_byte(&font[c * 5 + i]);
        for (int8_t j = 0; j < 8; j++, line >>= 1)
        {
            if (line & 1)
            {
                drawPixel(x + i, y + j);
            }
        }
    }
}

/** Draws pixel at (x, y) position. */
void SimpleSSD1306::drawPixel(int16_t x, int16_t y)
{
    if ((x >= 0) && (x < WIDTH) && (y >= 0) && (y < HEIGHT)) // Pixel is in-bounds.
    {
        // Draw white pixel on black background
        buffer[x + (y / 8) * WIDTH] |= (1 << (y & 7));
    }
}
