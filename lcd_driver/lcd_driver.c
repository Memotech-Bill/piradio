/* lcd_driver.c - Program to drive Epson EA-C20017AR LCD display

(c) Copyright 2024, Memotech-Bill. SPDX short identifier: BSD-3-Clause
*/

// Pin numbers
#define PNO_D0      0               // Out pins for data state machine
#define PNO_D1      (PNO_D0 + 1)
#define PNO_D2      (PNO_D0 + 2)
#define PNO_D3      (PNO_D0 + 3)
#define PNO_A0      (PNO_D0 + 4)
#define PNO_RD      5               // Side-set pins for data state machine
#define PNO_WR      (PNO_RD + 1)
#define PNO_CS      (PNO_RD + 2)
#define PNO_CLK     8               // Side set pin for clock state machine

// Pin bit masks
#define PMSK_D0     ( 1 << PNO_D0 )
#define PMSK_D1     ( 1 << PNO_D1 )
#define PMSK_D2     ( 1 << PNO_D2 )
#define PMSK_D3     ( 1 << PNO_D3 )
#define PMSK_DAT    ( PMSK_D0 | PMSK_D1 | PMSK_D2 | PMSK_D3 )
#define PMSK_A0     ( 1 << PNO_A0 )
#define PMSK_RD     ( 1 << PNO_RD )
#define PMSK_WR     ( 1 << PNO_WR )
#define PMSK_CS     ( 1 << PNO_CS )
#define PMSK_CTL    ( PMSK_A0 | PMSK_RD | PMSK_WR | PMSK_CS )
#define PMSK_ALL    ( PMSK_DAT |PMSK_CTL )
#define PMSK_CLK    ( 1 << PNO_CLK )

// SED1200 commands
#define CURSOR_DIRECTION    0x04
#define CURSOR_MOVE         0x06
#define CURSOR_FONT         0x08
#define CURSOR_BLINK        0x0A
#define DISPLAY_ENABLE      0x0C
#define CURSOR_ENABLE       0x0E
#define SYSTEM_RESET        0x10
#define LINE_MODE           0x12
#define CURSOR_ADDRESS      0x80
#define USRCH_ADDRESS       0x20
#define USRCH_DATA          0x40

#ifndef PROMPT
#define PROMPT              0       // 1 to enable feedback on USB (for testing)
#endif
#ifndef LCD_SIZE
#define LCD_SIZE            20      // Size of the display
#endif
#ifndef SCROLL_RATE
#define SCROLL_RATE         200     // Text scroll rate (ms per character)
#endif
#ifndef SCROLL_PAUSE
#define SCROLL_PAUSE        2000    // Pause before starting scrolling
#endif
#ifndef BUFF_LEN
#define BUFF_LEN            256     // Buffer size for received text
#endif

#include <stdbool.h>
#include <stdio.h>
#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <hardware/timer.h>
#include <pico/stdlib.h>
#include <pico/stdio.h>
#include <pico/time.h>
#include <lcd_driver.pio.h>

PIO pio;
int sm_clk;     // Clock generator
int sm_dat;     // Data output

// Send a byte to the display
void lcd_out (int addr, char ch)
    {
    pio_sm_put_blocking (pio, sm_dat,  (addr << 8) | ch);
    }

// Main program
int main (int nArg, const char *psArg[])
    {
    // Configure Pico
    pio = pio0;
    set_sys_clock_khz (100000, true);
    stdio_init_all ();

#if PROMPT
    // Wait for USB connection
    while ( ! stdio_usb_connected () )
        {
        sleep_ms (1000);
        }
    printf ("Connected\n");
#endif

    // Configure clock generation PIO
    pio_gpio_init (pio, PNO_CLK);
    sm_clk = pio_claim_unused_sm (pio, true);
    uint offset_clk = pio_add_program (pio, &lcd_clock_program);
    pio_sm_config cfg_clk = lcd_clock_program_get_default_config (offset_clk);
    sm_config_set_sideset (&cfg_clk, 1, false, false);
    sm_config_set_sideset_pins (&cfg_clk, PNO_CLK);
    sm_config_set_clkdiv_int_frac (&cfg_clk, 50, 0);
    pio_sm_set_consecutive_pindirs (pio, sm_clk, PNO_CLK, 1, true);
    pio_sm_init (pio, sm_clk, offset_clk, &cfg_clk);

    // Configure data output PIO
    pio_gpio_init (pio, PNO_D0);
    pio_gpio_init (pio, PNO_D1);
    pio_gpio_init (pio, PNO_D2);
    pio_gpio_init (pio, PNO_D3);
    pio_gpio_init (pio, PNO_A0);
    pio_gpio_init (pio, PNO_RD);
    pio_gpio_init (pio, PNO_WR);
    pio_gpio_init (pio, PNO_CS);
    sm_dat =  pio_claim_unused_sm (pio, true);
    uint offset_dat = pio_add_program (pio, &lcd_data_program);
    pio_sm_config cfg_dat = lcd_data_program_get_default_config (offset_dat);
    sm_config_set_sideset (&cfg_dat, 3, false, false);
    sm_config_set_sideset_pins (&cfg_dat, PNO_RD);
    sm_config_set_out_pins (&cfg_dat, PNO_D0, 5);
    sm_config_set_set_pins (&cfg_dat, PNO_D0, 4);
    sm_config_set_clkdiv_int_frac (&cfg_dat, 100, 0);
    sm_config_set_out_shift (&cfg_dat, true, false, 32);
    sm_config_set_in_shift (&cfg_dat, false, false, 32);
    sm_config_set_fifo_join (&cfg_dat, PIO_FIFO_JOIN_TX);
    pio_sm_set_pins (pio, sm_dat, PMSK_ALL);
    pio_sm_set_consecutive_pindirs (pio, sm_dat, PNO_D0, 8, true);
    pio_sm_init (pio, sm_dat, offset_dat, &cfg_dat);

    // Start state machines
    pio_enable_sm_mask_in_sync (pio, (1 << sm_clk) | (1 << sm_dat));

    // Configure display
    lcd_out (0, SYSTEM_RESET);
    lcd_out (0, LINE_MODE | 1);
    lcd_out (0, DISPLAY_ENABLE | 1);
    lcd_out (0, CURSOR_ADDRESS | 0);
    lcd_out (0, CURSOR_DIRECTION | 0);

    // Setup text control - Two buffers, one for receiving text the other holding text to display.
    int iBuf = 0;
    int nLen[2] = { 0, LCD_SIZE };
    char sTxt[2][BUFF_LEN] = { "", "                    " };
    absolute_time_t tUpdate = get_absolute_time ();     // Time for next update
    bool bFirst = true;                                 // Initial update
    const char *psFst = &sTxt[iBuf][0];                 // First character to display

    // Main loop
#if PROMPT
    printf ("\n> ");
#endif
    while (true)
        {
        // Test for receiving a character
        int ch = getchar_timeout_us (0);
        if (ch >= 0)
            {
#if PROMPT            
            putchar (ch);
#endif
            if ((ch == '\r') || (ch == '\n'))
                {
                // End of line
                if (nLen[iBuf] <= LCD_SIZE)
                    {
                    // Short message - Pad to length of display
                    while (nLen[iBuf] < LCD_SIZE)
                        {
                        sTxt[iBuf][nLen[iBuf]] = ' ';
                        ++nLen[iBuf];
                        }
                    }
                else
                    {
                    // Long message - Add padding for roll
                    for (int i = 0; i < 4; ++i)
                        {
                        if (nLen[iBuf] == (sizeof (sTxt[iBuf]) - 1)) break;
                        sTxt[iBuf][nLen[iBuf]] = ' ';
                        ++nLen[iBuf];
                        }
                    }
                // Swap buffers
                sTxt[iBuf][nLen[iBuf]] = '\0';
                psFst = &sTxt[iBuf][0];
                iBuf = 1 - iBuf;
                nLen[iBuf] = 0;
                bFirst = true;
#if PROMPT
                printf ("\n> ");
#endif
                }
#if PROMPT
            else if (ch == 0x08)
                {
                // Backspace - remove a character
                if (nLen[iBif] > 0) --nLen[iBuf];
                }
#endif
            else
                {
                // Add character to buffer
                if (nLen[iBuf] <= (sizeof (sTxt[iBuf]) - 1))
                    {
                    sTxt[iBuf][nLen[iBuf]] = ch;
                    ++nLen[iBuf];
                    }
                }
            }
        if (bFirst || time_reached (tUpdate))
            {
            // Update the display
            int jBuf = 1 - iBuf;
            const char *ps = psFst;
            lcd_out (0, CURSOR_ADDRESS | 0);
            for (int i = 0; i < LCD_SIZE; ++i)
                {
                if (i == LCD_SIZE / 2) lcd_out (0, CURSOR_ADDRESS | 0x40);
                lcd_out (1, *ps);
                ++ps;
                if ( *ps == '\0' ) ps = &sTxt[jBuf][0];
                }

            // Time for next update
            if (nLen[jBuf] > LCD_SIZE)
                {
                // Roll the display
                ++psFst;
                if (*psFst == '\0') psFst = &sTxt[jBuf][0];
                tUpdate = make_timeout_time_ms (bFirst ? SCROLL_PAUSE : SCROLL_RATE);
                }
            else
                {
                // Short message - No update needed until new message received
                tUpdate = at_the_end_of_time;
                }
            bFirst = false;
            }
        }
    }
