# Driver for Epson EA-C20017AR LCD Display

I have had a couple of these lying around since the 1980s.
A data sheet for these can be found
[here](http://www.geocities.ws/j_ds_au/9592/lcdinfo.pdf).

The display uses a
[SED1200](http://www.dougrice.plus.com/hp/LCDdata/pdfs/sed1200f.pdf)
driver chip.

This display is physically organised as one row of 20 characters,
but logically it appears as two rows of 10 characters. Therefore
when going from the tenth to eleventh ccharacter it is necessary
to set a new cursor address, the auto-increment is not sufficient.

A Raspberry Pi Pico is used as a driver for the display. On receiving
a line of text, terminated by either CR or LF, on either USB or UART
the text is displayed on the LCD. Messages of up to 20 characters are
displayed statically. Messages longer than that are rolled continuously
across the display. The longest message that can be displayed is 255
characters, changable by a #define statement.

The code uses two PIO state machines:

*   The first one simply runs continuously to generate the required
    clocck signal.

*   The second one clocks data into the LCD, splitting the data byte
    into two halves, and setting the address bit appropriately.

These two state machines maintain the timing of the signals to the
LCD display. These timings are probably conservative.

The connections are:

| Connection   |  Pico Pin    | LCD Pin |
|--------------|--------------|---------|
| LCD Data 0   |  1 (GPIO 0)  |   12    |
| LCD Data 1   |  2 (GPIO 1)  |   11    |
| LCD Data 2   |  4 (GPIO 2)  |   10    |
| LCD Data 3   |  5 (GPIO 3)  |    9    |
| LCD Address  |  6 (GPIO 4)  |    6    |
| Read Enable  |  7 (GPIO 5)  |    8    |
| Write Strobe |  9 (GPIO 6)  |    7    |
| Chip Select  | 10 (GPIO 7)  |    5    |
| Clock        | 11 (GPIO 8)  |    4    |
| Vcc (+5V)    |   39 or 40   |    1    |
| Ground       | 3, 8 ... 38  |    2    |
| LCD Contrast |              |    3    |
| RX Data      | 22 (GPIO 17) |         |

The Pico connections can be altered by updating
#define statements in the source code.

*   LCD Data and Address signals are PIO OUT pins
    and therefore have to be consecutive GPIOs.

*   The Read Enable, Write Strobe and Chip Select
    signals are PIO side-set pins and therefore
    have to be consecutive GPIOs.
