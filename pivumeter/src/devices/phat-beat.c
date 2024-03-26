/*   pivumeter : ALSA VU meter for PHAT-BEAT using libgpio
 *   Copyright (c) 2024 by Memotech-Bill
 *   SPDX short identifier: GPL-3.0-only 
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <gpiod.h>
#include "../pivumeter.h"


#define MIN(x, y)   (((x) < (y)) ? (x) : (y))
#define MAX(x, y)   (((x) > (y)) ? (x) : (y))

#define LOG_SCALE   1
#define DAT 23
#define CLK 24
#ifndef NUM_SAMPLES
#define NUM_SAMPLES 2
#endif

uint8_t b_l[8] = {0};
uint8_t b_r[8] = {0};

struct gpiod_chip *chip;
struct gpiod_line *line_clk;
struct gpiod_line *line_dat;
const char *consumer = "vumeter";

FILE * fLog = NULL;

void set_leds (uint32_t l, uint32_t r, int brightness)
    {
#if LOG_SCALE
    int led = 7;
    while ( led > 0 )
        {
        if ( l >= 0x2000 ) break;
        b_l[led] = 0;
        l <<= 2;
        --led;
        }
    b_l[led] = brightness * l / 0x7FFF;
    --led;
    while ( led >= 0 )
        {
        b_l[led] = brightness;
        --led;
        }
    led = 7;
    while ( led > 0 )
        {
        if ( r >= 0x2000 ) break;
        b_r[led] = 0;
        r <<= 2;
        --led;
        }
    b_r[led] = brightness * r / 0x7FFF;
    --led;
    while ( led >= 0 )
        {
        b_r[led] = brightness;
        --led;
        }
#else
    static int avg_l[NUM_SAMPLES] = {0};
    static int avg_r[NUM_SAMPLES] = {0};
    static unsigned int avg_idx = 0;
    
    int32_t meter_l = (l * 8 * brightness) / 16384;
    int32_t meter_r = (r * 8 * brightness) / 16384;

    avg_l[avg_idx] = meter_l;
    avg_r[avg_idx] = meter_r;

    avg_idx ++;
    avg_idx %= NUM_SAMPLES;

    int64_t a_l = 0;
    int64_t a_r = 0;
    for (int i = 0; i < NUM_SAMPLES; ++i)
        {
        a_l += avg_l[i];
        a_r += avg_r[i];
        }

    meter_l = a_l / NUM_SAMPLES;
    meter_r = a_r / NUM_SAMPLES;

    for (int led = 0; led < 8; led++)
        {
        b_l[led] = MIN (meter_l, brightness);
        meter_l -= brightness;
        meter_l = MAX (0, meter_l);

        b_r[led] = MIN (meter_r, brightness);
        meter_r -= brightness;
        meter_r = MAX (0, meter_r);
        }
#endif
    if ( fLog != NULL )
        {
        fprintf (fLog, "Meter left = %d, right = %d, brightness = %d\n", l, r, brightness);
        fprintf (fLog, "Left: ");
        for (int i = 0; i < 8; ++i) fprintf (fLog, " %3d", b_l[i]);
        fprintf (fLog, "\n");
        fprintf (fLog, "Right:");
        for (int i = 0; i < 8; ++i) fprintf (fLog, " %3d", b_r[i]);
        fprintf (fLog, "\n");
        fflush (fLog);
        }
    }

void write_byte (uint8_t byte)
    {
    for (int i = 0; i < 8; ++i)
        {
        gpiod_line_set_value (line_dat, byte & 0b10000000);
        gpiod_line_set_value (line_clk, 1);
        byte <<= 1;
        gpiod_line_set_value (line_clk, 0);
        }
    }

void sof ()
    {
    for (int i = 0; i < 4; ++i)
        {
        write_byte (0);
        }
    }

void eof ()
    {
    for (int i = 0; i < 5; ++i)
        {
        write_byte (0);
        }
    }

void render ()
    {
    sof ();
    for (int led = 0; led < 8; ++led)
        {
        uint8_t led_r = 7 - led;
        unsigned int green = 255 - (led_r * 36);
        green = (green * b_r[led_r]) / 255;
        unsigned int red = led_r * 36;
        red = (red * b_r[led_r]) / 255;
        write_byte (0b11100000 | 15);
        write_byte (0);
        write_byte (green);
        write_byte (red);
        }
    for (int led = 0; led < 8; ++led)
        {
        uint8_t led_l = led;
        unsigned int green = 255 - (led_l * 36);
        green = (green * b_l[led_l]) / 255;
        unsigned int red = led_l * 36;
        red = (red * b_l[led_l]) / 255;
        write_byte (0b11100000 | 15);
        write_byte (0);
        write_byte (green);
        write_byte (red);
        }
    }

int init (const char *log_file)
    {
    if ( log_file && log_file[0] ) fLog = fopen (log_file, "w");
    if ( fLog ) fprintf (fLog, "pivumeter.init");
    chip = gpiod_chip_open ("/dev/gpiochip0");

    line_clk = gpiod_chip_get_line (chip, CLK);
    if (gpiod_line_request_output (line_clk, consumer, 0))
        {
        fprintf (stderr, "Failed to set up Clock pin\n");
        return 1;
        }

    line_dat = gpiod_chip_get_line (chip, DAT);
    if (gpiod_line_request_output (line_dat, consumer, 0))
        {
        fprintf (stderr, "Failed to set up Data pin\n");
        return 1;
        }

    return 0;
    }

void deinit ()
    {
    if ( fLog ) fprintf (fLog, "pivumeter.deinit");
    for (int led = 0; led < 8; ++led)
        {
        b_l[led] = 0;
        b_r[led] = 0;
        }
    render ();
    if ( fLog ) fclose (fLog);
    gpiod_line_release (line_clk);
    gpiod_line_release (line_dat);
    gpiod_chip_close (chip);
    }

static void update (int meter_level_l, int meter_level_r, snd_pcm_scope_ameter_t *level)
    {
    set_leds (meter_level_l, meter_level_r, level->led_brightness);
    render ();
    }

device phat_beat ()
    {
    struct device _phat_beat;
    _phat_beat.init = &init;
    _phat_beat.update = &update;
    _phat_beat.deinit = &deinit;
    return _phat_beat;
    }
