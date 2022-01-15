# Program notes
Below are the technical and creative materials behind a composition for viola, cello, and electronics. 

The piece aims to develop a single musical gesture: a gradual accumulation of sonic energy that "breaks" into a cascade of glassy, smooth sound. I'm drawing on an experience from August 2021, of swimming in the ocean near the Golden Gate Bridge, being picked up and knocked over by waves. 

The form of the piece is arranged into sections of increasing length and timbral density. The smallest formal unit of the piece is 28 beats of 132 bpm. The structure is as follows:

(0:12) A: 1 unit; 1 timbral idea (unison tremollo, ponticello, with accents)

(0:12) B: 1 unit; 1 timbral idea (double-stop tremollo, ponticello, with accents)

(0:25) C: 2 units; 2 timbral ideas (double-stop tremollo with motion of the voices, intermittent ordinario sustained double stops)

(0:38) D: 3 units; 3 timbral ideas (rocking / string-crossing motion across three strings, sustained ordinario harmonics, intermittent spectral freeze of sustained notes)

(1:04) E: 5 units; 

(1:42) F: 8 units;

(2:45) G: 13 units;

(1:00) Coda

Each lettered section negotiates its own increase in energy. 

## The `scores` folder
This folder contains pictures of different portions of the (ongoing) creative process. 

1. `graphic`: an initial formal sketch / graphic score, with tabulation of sounds & techniques I was interested in exploring. 
2. `experiment`: an experiment using some techniques I was interested in -- harmonics with overpressure, harmonic glissandi.
3. `sectional draft`: a draft of the opening section of the piece; an expansion of the experiment. 
4. `revision`: a revised formal sketch. 


## The `doc`, `max`, `python`, `src`, and `tests` folders
These folders contain the in-progress documentation and implementation of a library of C++ audio processing classes. The library classes themselves are all in the `src` directory. 

Some of the scripts in `python` depend on `mido` and `numpy`, both of which can be installed using `pip`.


## C++ dependencies; compiling and running:
1. This project depends on `portaudio` (https://github.com/PortAudio/portaudio),  `RtMidi` (https://github.com/thestk/rtmidi), and `fftw3` (https://www.fftw.org/). All can be installed with a package manager such as `homebrew`. The project also depends on `tinyosc` (https://github.com/mhroth/tinyosc), which is used, at the author's suggestion, by embedding its source within the project. 
2. A number of example projects are provided in the `tests` directory. From within that directory, the command `./build a_test_source.cpp` will genereate an executable named `A_test_source`.
3. The `tests/build` script simply automates a simple command-line compiler instruction. You may need to adjust the compiler flags in that file depending on how you've installed the dependencies above.  


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
11. `octave.cpp`: (in progress) a frequency-domain pitch-shifter / octave pedal. 
12. `pm.cpp`: demonstrates the phase-modulation capabilities of the `Synth<T>` template. 
13. `resynthesis.cpp`: demonstrates an ensemble of resonant IIR bandpass filters running in paralellel, using the `Filterbank<T>` template.
14. `spectral.cpp`: demonstrates use of classes for frequency-domain manipulations (via a callback interface, a bit like Max's `pfft~`). This example performs spectral gating, only allowing high-amplitude components through. This tends to remove transients and other wideband sounds. 
15. `subtractive.cpp`: polyphonic subtractive synthesizer with dynamic tempering. 

The additive and subtractive synthesizers expect midi messages from the IAC Driver Bus. The script `launch.py` (in the `python` folder) is designed to provide an interface for sending such messages, and expects to talk to a Novation Launchpad Pro Mk3 plugged in over USB. If you prefer to use a different device, run the terminal command `python3 mirror.py "your_input_device_name"` to send your device's midi messages to the driver bus. 