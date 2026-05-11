#include <stdio.h>
#include <math.h>

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"

#include "src/Display.h"
#include "src/Microphone.h"
#include "lib/kiss_fft.h"

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

#define NSAMP       1024
#define SAMPLE_RATE 44100   // Hz (48MHz / (1 + 992) ≈ 44.1kHz)
#define ADC_PIN     26

// Frequency edges (in Hz) for each of the 8 display columns.
// Converted to FFT bin indices at runtime in setup_hardware().
// Each bin = SAMPLE_RATE / NSAMP ≈ 43 Hz.
#define NUM_COLS    8
static const float FREQ_EDGES_HZ[NUM_COLS + 1] = {
    20, 60, 150, 400, 1000, 2400, 5000, 10000, 12000
};

// Tune this to adjust display sensitivity (lower = more sensitive)
#define MAG_SCALE   100.0f

// ---------------------------------------------------------------------------
// Module-level state
// ---------------------------------------------------------------------------

static uint16_t          sample_buf[NSAMP];
static kiss_fft_cpx      fft_in[NSAMP];
static kiss_fft_cpx      fft_out[NSAMP];
static kiss_fft_cfg      fft_cfg;

static int               dma_chan;
static dma_channel_config dma_cfg;

// Precomputed bin index edges (derived from FREQ_EDGES_HZ at startup)
static int bin_edges[NUM_COLS + 1];

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------

static void compute_bin_edges(void) {
    float hz_per_bin = (float)SAMPLE_RATE / (float)NSAMP;
    for (int i = 0; i <= NUM_COLS; i++) {
        bin_edges[i] = (int)(FREQ_EDGES_HZ[i] / hz_per_bin);
        // Clamp to the valid range of non-redundant FFT bins (0 .. NSAMP/2)
        if (bin_edges[i] > NSAMP / 2) bin_edges[i] = NSAMP / 2;
    }
}

static void setup_adc(void) {
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(0);
    adc_fifo_setup(
        true,   // write each conversion to FIFO
        true,   // enable DMA DREQ
        1,      // assert DREQ when >= 1 sample present
        false,  // don't append error bit
        false   // keep full 12-bit resolution
    );
    adc_set_clkdiv(992); // 48MHz / (1+992) ≈ 44.1kHz
}

static void setup_dma(void) {
    dma_chan = dma_claim_unused_channel(true);
    dma_cfg  = dma_channel_get_default_config(dma_chan);

    channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&dma_cfg, false);  // always read from FIFO
    channel_config_set_write_increment(&dma_cfg, true);  // step through sample_buf
    channel_config_set_dreq(&dma_cfg, DREQ_ADC);
}

void setup_hardware(void) {
    stdio_init_all();
    init_display();

    setup_adc();
    setup_dma();

    fft_cfg = kiss_fft_alloc(NSAMP, 0, NULL, NULL);
    compute_bin_edges();
}

// ---------------------------------------------------------------------------
// Per-frame processing
// ---------------------------------------------------------------------------

static void capture_samples(void) {
    adc_fifo_drain();
    dma_channel_configure(
        dma_chan, &dma_cfg,
        sample_buf,       // destination
        &adc_hw->fifo,    // source
        NSAMP,            // transfer count
        true              // start immediately
    );
    adc_run(true);
    dma_channel_wait_for_finish_blocking(dma_chan);
    adc_run(false);
    adc_fifo_drain();
}

// Remove DC offset and apply a Hann window, writing into fft_in[].
static void window_samples(void) {
    float sum = 0.0f;
    for (int i = 0; i < NSAMP; i++) sum += (float)sample_buf[i];
    float avg = sum / (float)NSAMP;

    for (int i = 0; i < NSAMP; i++) {
        float hann = 0.5f * (1.0f - cosf(2.0f * (float)M_PI * i / (NSAMP - 1)));
        fft_in[i].r = ((float)sample_buf[i] - avg) * hann;
        fft_in[i].i = 0.0f;
    }
}

// Return the peak FFT magnitude across bins [start, end).
static float peak_magnitude(int start, int end) {
    float peak = 0.0f;
    for (int i = start; i < end; i++) {
        float mag = sqrtf(fft_out[i].r * fft_out[i].r +
                          fft_out[i].i * fft_out[i].i);
        if (mag > peak) peak = mag;
    }
    return peak;
}

// Convert a column height (0-8) to a bottom-aligned LED bitmask.
// Height 0 → 0x00, height 8 → 0xFF.
static uint8_t height_to_bitmask(int height) {
    if (height <= 0) return 0x00;
    if (height >= 8) return 0xFF;
    return (uint8_t)((1 << height) - 1);
}

// Build an 8-byte row pattern from 8 column bitmasks so the MAX7219
// row-oriented driver draws the correct bar-graph image.
// column_bits[c] has bit k set when column c should be lit at row k.
static void columns_to_rows(uint8_t column_bits[NUM_COLS],
                             uint8_t row_pattern[8]) {
    for (int row = 0; row < 8; row++) {
        uint8_t byte = 0;
        for (int col = 0; col < NUM_COLS; col++) {
            if (column_bits[col] & (1 << row)) {
                byte |= (1 << col);
            }
        }
        row_pattern[row] = byte;
    }
}

void process_frame(void) {
    capture_samples();
    window_samples();
    kiss_fft(fft_cfg, fft_in, fft_out);

    uint8_t column_bits[NUM_COLS];
    for (int col = 0; col < NUM_COLS; col++) {
        float mag    = peak_magnitude(bin_edges[col], bin_edges[col + 1]);
        int   height = (int)(mag / MAG_SCALE);
        if (height > 8) height = 8;

        column_bits[col] = height_to_bitmask(height);

        printf("Col %d [%d-%d bins]: mag=%.1f height=%d\n",
               col, bin_edges[col], bin_edges[col + 1], mag, height);
    }

    uint8_t row_pattern[8];
    columns_to_rows(column_bits, row_pattern);
    display_pattern(row_pattern);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int main(void) {
    setup_hardware();
    while (true) {
        process_frame();
    }
}