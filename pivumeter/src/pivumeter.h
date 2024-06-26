/*   pivumeter : ALSA VU meter for PHAT-BEAT using libgpio
 *   Copyright (c) 2024 by Memotech-Bill
 *   SPDX short identifier: GPL-3.0-only 
 */
#ifndef _PHATMETER_H_
#define _PHATMETER_H_
#include <alsa/asoundlib.h>

typedef struct _snd_pcm_scope_ameter_channel
    {
    int16_t levelchan;
    int16_t peak;
    unsigned int peak_age;
    int16_t previous;
    } snd_pcm_scope_ameter_channel_t;

typedef struct _snd_pcm_scope_ameter
    {
    snd_pcm_t *pcm;
    snd_pcm_scope_t *s16;
    snd_pcm_scope_ameter_channel_t *channels;
    snd_pcm_uframes_t old;
    int top;
    unsigned int decay_ms;
    unsigned int peak_ms;
    unsigned int led_brightness;
    unsigned int bar_reverse;
    } snd_pcm_scope_ameter_t;

typedef struct device
    {
	int (*init)(const char *);
	void (*update)(int meter_level_l, int meter_level_r, snd_pcm_scope_ameter_t *level);
    void (*deinit)(void);
    } device;
#endif
