#include "Microphone.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include <math.h>
    

#define ADC_NUM 0
#define ADC_PIN (26 + ADC_NUM)
#define NSAMP 1024
float fft_input[NSAMP];

uint16_t sample_buffer[NSAMP];
int dma_chan;

void setup_adc_dma() {
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(ADC_NUM);

    // Setup FIFO
    adc_fifo_setup(
        true,    // Write each completed conversion to the FIFO
        true,    // Enable DMA data request (DREQ)
        1,       // DREQ asserted when at least 1 sample is present
        false,   // We don't need the error bit
        false    // Keep full 12-bit resolution (don't shift to 8-bit)
    );

    // Set sampling rate. 48MHz (clock speed) / (1 + div) = Sample Rate
    // For ~44.1kHz sampling: 992 div
    adc_set_clkdiv(992);

    // Setup DMA
    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_dreq(&cfg, DREQ_ADC);

    dma_channel_configure(
        dma_chan, &cfg,
        sample_buffer,    // Destination
        &adc_hw->fifo,    // Source
        NSAMP,            // Number of transfers
        false             // Don't start yet
    );
}

void capture_and_process() {
    // 1. Start ADC and DMA
    int dma_chan = dma_claim_unused_channel(true);
    adc_fifo_drain();
    dma_channel_set_trans_count(dma_chan, NSAMP, true);
    adc_run(true);

    // 2. Wait for DMA to finish
    dma_channel_wait_for_finish_blocking(dma_chan);
    adc_run(false); // Stop ADC

    // 3. DC Removal and Conversion to Float
    float sum = 0;
    for (int i = 0; i < NSAMP; i++) {
        sum += sample_buffer[i];
    }
    float average = sum / NSAMP;

    for (int i = 0; i < NSAMP; i++) {
        // Subtract average to center the wave at 0.0
        // We also normalize it to a -1.0 to 1.0 range (optional but helpful)
        fft_input[i] = ((float)sample_buffer[i] - average) / 2048.0f;
    }
}

void apply_window(float* data, int n) {
    for (int i = 0; i < n; i++) {
        // Hann window formula
        float window = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (n - 1)));
        data[i] *= window;
    }
}