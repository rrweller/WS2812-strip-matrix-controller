My custom WS2812 Strip Matrix Controller built for Teensy 3.1/3.2. This code controls a matrix made of n WS2812 individually addressable LED strips each containing n LEDs. The controller has multiple modes that can be cycled through with a long press of a button. A short press randomly generates a new H and S value in certain modes to randomly generate a new color for the strips.

Current Modes are

1: Default white lamp

2: Dual color audio spectrum reactive via FFT

3: Single color audio spectrum reactive via FFT

4: Solid color audio reactivity via FFT

5: Solid random color

6: RGB cycle fade
