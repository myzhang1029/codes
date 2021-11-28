#ifndef BASE64_H
#define BASE64_H

#include <stdint.h>

struct base64decoder {
    uint16_t buf;
    uint16_t count;
} static const BASE64_INITIALIZER = {0, 0};

int base64_feed(struct base64decoder *decoder, int m);
uint8_t base64_read(struct base64decoder *decoder);

#endif
