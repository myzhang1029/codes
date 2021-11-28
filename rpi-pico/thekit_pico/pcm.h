#ifndef PCM_H
#define PCM_H

#include "pico/stdlib.h"

struct pcmaudio_player {
    // Speaker/amplifier pin
    uint pin;
    // PCM buffer
    uint8_t *audio_buf;
    // Whether `free(audio_buf)` should be done when finished
    bool free_buf;
    // Total length in bytes
    uint32_t audio_length;
    // Current index
    uint32_t index;
    // PCM timer
    struct repeating_timer pcm_timer;
    // Whether timer has started
    bool started;
};

void pcmaudio_init(struct pcmaudio_player *player, uint pin);
void pcmaudio_fill(struct pcmaudio_player *player, uint8_t *buffer, uint32_t length, bool free_buf);
bool pcmaudio_play(struct pcmaudio_player *player);
void pcmaudio_stop();

#endif
