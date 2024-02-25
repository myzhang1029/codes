/// Capture continuous data from the ADC and send it to stdout.
/// Based on BSD-3-Clause adc/dma_capture/dma_capture.c

#include <stdio.h>

#include "pico/stdlib.h"

#include "hardware/adc.h"
#include "hardware/dma.h"

// Pin - 26
#define ADC_CH 2
// 48MHz / 960 = 50kHz
#define CLK_DIV 960
// 4 seconds of data, that is about the max we can fit in RP2040's RAM
#define LENGTH 200000

uint8_t buffer[LENGTH];

int main() {
    stdio_init_all();
    adc_init();
    adc_gpio_init(26 + ADC_CH);
    adc_select_input(ADC_CH);
    adc_set_clkdiv(CLK_DIV);
    // Shift 12-bit ADC data to 8-bit
    adc_fifo_setup(true, true, 1, false, true);
    // Get a DMA channel
    uint dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    // Transfer 8-bit data
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    // The read address is the ADC FIFO
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    channel_config_set_dreq(&c, DREQ_ADC);

    while (1) {
        adc_run(false);
        adc_fifo_drain();
        // Start the DMA transfer immediately
        dma_channel_configure(dma_chan, &c, buffer, &adc_hw->fifo, LENGTH, true);
        adc_run(true);
        uint64_t start = time_us_64();
        dma_channel_wait_for_finish_blocking(dma_chan);
        uint64_t end = time_us_64();
        uint64_t elapsed = end - start;
        for (int i = 0; i < LENGTH; i++) {
            printf("0x%02x,", buffer[i]);
        }
        printf("\nTook %d samples in %llu us\n", LENGTH, elapsed);
        sleep_ms(1000);
    }
}
