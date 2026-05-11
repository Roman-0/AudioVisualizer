## Audio Visualizer
This an Audio visualizer using a Raspberry Pi Pico 2. This project takes sound as input, and performs multiple DSP-heavy calculations to transform the sound into 8 logarithmically separated groups of magnitudes, then displays them through the display. The device is fully standalone but can also be plugged in to receive more accurate data. 

Hardware used:
  Raspberry Pi Pico 2
  Elegoo Max7219 Dot Matrix Module
  KS0035 Microphone Sound Sensor
  Battery Pack

## DSP Concepts:
-  **Hann Window** attenuates values on the edges of the sample to reduce leakage. 
-  **Nyquist Sampling rate**. To Receive 20 khz signals, we need >40 khz sampling rate from mic.
 -  **ADC** to convert the analog audio into a discrete digital signal for the FFT
 -  **FFT** in order to convert sound into frequency domain. 
 -  **A-weighting filter** to normalize magnitudes for human hearing.

## Specifications
- Sample Rate: 44.1kHz
- ADC clock divisor: 992
- Frequency Bins: 20,60,150,400,1000,2400,5000,10000,12000


This project took a ton of work, we faced many issues with our components and kept having to pivot strategies to maintain time. It also taught us a lot involving DSP and Embedded Development.
