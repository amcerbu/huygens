# huygens
a library for audio processing


## Compiling and running projects:
1. This project depends on `portaudio` (https://github.com/PortAudio/portaudio),  `RtMidi` (https://github.com/thestk/rtmidi), and `fftw3` (https://www.fftw.org/). All can be installed with a package manager such as `homebrew`.
2. A number of example projects are provided in the `tests` directory. From within that directory, the command `./build a_test_source.cpp` will genereate an executable named `A_test_source`.
3. The `build` script in that folder may need to be edited depending on the locations of linked libraries and the compiler of your system. 


## Description of tests
1. `additive.cpp`: polyphonic additive synthesizer with dynamic tempering. 
2. `bandpass.cpp`: demonstrates usage of the `Filter<T>` template.
3. `bowl.cpp`: demonstrates usage of the `Bowl<T>` template, which allows for synthesis of resonance models (frequency, amplitude, decay).
4. `delay.cpp`: demonstartes usage of the `Delay<T>` template, which allows for delay lines with feedback ("sparse" filters). Caution, use headphones!
5. `fft.cpp`: attempts to match pitch of the incoming audiostream; basic demonstration of the `Cosine` class in `src/fourier.h`.
6. `fm.cpp`: demonstrates the frequency-modulation capabilities of the `Synth<T>` template. 
7. `fuzz.cpp`: modulated playback of the contents of a delayline (the mic input). Headphones aren't required; feedback can lead to interesting results. Adjust the amount of mangling by entering a number (samples of delay) into the terminal while the program runs. 
8. `granny.cpp`: demonstrates both the granular synthesis template, `Granny<T>`, and `UDPReceive`, the system for receiving OSC messages (e.g., from Max). The synthesizer expects to be controlled with `max/gr.maxpat`. It is configured to perform granular synthesis on the incoming audiostream but can be configured to use a different buffer instead.
9. `metronome.cpp`: a simple metronome utility (type a number in the terminal window to specify bpm). 
10. `mirror.cpp`: mirrors the incoming audiostream. Also has a small built-in monophonic synthesizer than can be played with the computer keyboard. The buttons of a standard QWERTY keyboard are mapped chromatically in rows, and in ascending fourths from row to row (like a guitar or bass).
11. `pm.cpp`: demonstrates the phase-modulation capabilities of the `Synth<T>` template. 
12. `resynthesis.cpp`: demonstrates an ensemble of resonant IIR bandpass filters running in paralellel, using the `Filterbank<T>` template.
13. `spectral.cpp`: demonstrates use of classes for frequency-domain manipulations (via a callback interface, a bit like Max's `pfft~`). This example performs spectral gating, only allowing high-amplitude components through. This tends to remove transients and other wideband sounds. 
14. `subtractive.cpp`: polyphonic subtractive synthesizer with dynamic tempering. 


## Description of scores
The `scores` folder contains pictures of different portions of the (ongoing) creative process. 

1. `graphic`: an initial formal sketch, tabulation of sounds & techniques. 
2. # Music-203-F21
# Music-203-F21
