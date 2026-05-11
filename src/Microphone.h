#ifndef MICROPHONE_H
#define MICROPHONE_H

// Microphone.h / Microphone.c are currently unused — all ADC/DMA setup
// and sample capture is handled in main.c for tighter integration with
// the DMA channel lifetime and FFT pipeline.
//
// If you want to extract microphone logic here in the future, a clean API
// would look something like:
//
//   void  mic_init(int adc_pin, float sample_rate_hz);
//   void  mic_capture(float *out_buf, int n);   // blocks until n samples ready
//   void  mic_apply_hann_window(float *buf, int n);

#endif // MICROPHONE_H