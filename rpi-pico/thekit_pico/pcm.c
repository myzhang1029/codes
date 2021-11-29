/* PCM Audio Player for Raspberry Pi Pico */
/*
 *  pcm.c
 *  Copyright (C) 2021 Zhang Maiyun <myzhang1029@hotmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/* Basic usage:
 * - pcmaudio_init(struct, PIN);
 * - pcmaudio_play(initialized struct, length)
 * - pcmaudio_stop() to interrupt (safe to call multiple times)
 *
 * if playback is finished, pcmaudio_stop() is automatically called,
 * and the buffer can be free()d.
 * Due to communication limitations, only one playback can be done at a time.
*/

#include "hardware/pwm.h"
#include "malloc.h"
#include "pico/stdlib.h"

#include "pcm.h"

// 8kHz: 125us per sample
#define SAMPLE_TIME 125

// If this is not NULL, it should not be overwritten
static struct pcmaudio_player *pplayer = NULL;

static bool update_pcm_callback(struct repeating_timer *t) {
    if (pplayer->audio_length > pplayer->index) {
        uint8_t next = pplayer->audio_buf[pplayer->index++];
        pwm_set_gpio_level(pplayer->pin, next);
        return true;
    }
    // the audio has been drained
    pcmaudio_stop();
    return false;
}

/// Initialize player on the pin
void pcmaudio_init(struct pcmaudio_player *player, uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    player->pin = pin;
    player->started = false;
}

/// Put data into the player
/// `audio_buffer` is an array of `uint8_t`s representing PCM samples
/// `length` is the total length in bytes
void pcmaudio_fill(struct pcmaudio_player *player, uint8_t *buffer, uint32_t length, bool free_buf) {
    player->audio_buf = buffer;
    player->audio_length = length;
    player->free_buf = free_buf;
    player->index = 0;
}

/// Start playing
bool pcmaudio_play(struct pcmaudio_player *player) {
    // Player lock to avoid overwriting `pcmaudio_player`
    if (pplayer)
        return false;
    pplayer = player;
    uint slice_num = pwm_gpio_to_slice_num(player->pin);
    // 8-bit wraps
    pwm_set_wrap(slice_num, 255);
    pwm_set_gpio_level(player->pin, 1);
    pwm_set_enabled(slice_num, true);
    add_repeating_timer_us(-(SAMPLE_TIME), update_pcm_callback, NULL, &player->pcm_timer);
    player->started = true;
    return true;
}

/// Make sure playback is stopped
void pcmaudio_stop() {
    if (pplayer && pplayer->started) {
        cancel_repeating_timer(&pplayer->pcm_timer);
        if (pplayer->free_buf)
            free(pplayer->audio_buf);
        pplayer->started = false;
    }
    pplayer = NULL;
}

