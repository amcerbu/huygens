#include "../src/audio.h"
#include "../src/midi.h"
#include "../src/oscbank.h"
#include "../src/mixer.h"
#include "../src/modbank.h"
#include "../src/slidebank.h"
#include "../src/stickbank.h"
#include "../src/distbank.h"
#include "../src/signal.h"
#include "../src/latchbank.h"
#include "../src/rmsbank.h"
#include "../src/rms.h"
#include "../src/noise.h"
#include "../src/filter.h"
#include "../src/synth.h"

#include <signal.h>

using namespace soundmath;
#define BSIZE 32

static bool running = true;
void interrupt(int ignore)
{
	std::cout << " [keyboard interrupt, exiting ...]" << std::endl;
	std::fclose(stdin);
    std::cin.setstate(std::ios::badbit); // To differenciate from EOF input
	running = false;
}

#define courses 1 // "strings" per notes coursed,
#define harmonics 1 // overtones
#define octaves 4 // number of octaves
#define division 12 // edo
#define parities 2
#define N parities * division * octaves * courses * harmonics // lots of filters!

const double detune = 0.125;
const int C1 = 24;
const double frequency = mtof(C1);


std::complex<double> gate(std::complex<double> x)
{ return (std::abs(x) < 0.0008 ? 0 : x); }

Oscbank<double, N> analysis; // modulators
Modbank<double, N> modulators; // modulation
Slidebank<double, N> slidebank; // low-pass filterbank
Distbank<double, N> distbank(&gate); // channel distortion
Latchbank<double, N> latchbank(0.0005);
RMSbank<double, N> rmsbank;
Stickbank<double, N> smoothbank(1, -0.9); 
Oscbank<double, N> synthesis; // demodulators
Modbank<double, N> demodulators; // demodulation
Mixer<double, N> mixdown;

RMS<double> rms;
Noise<double> noise;

const double dry = 0;
const double gain = 3;
double the_sample = 0;

const int multiplicity = 4; // low-pass multiplicity
const double wavelengths = 25;
// const double wavelengths = 300; // "reverb"
const double cutoff = 0.999;


// consider implementing some feature low-pass filters the _phase_ of
// the filtered signal as a way of detecting near-misses (think vocoder)
inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		double the_sample = limiter(dry * in[i] + gain *
			mixdown(
				demodulators(
					synthesis(),
					smoothbank(
						latchbank(&rmsbank,
							slidebank(
								modulators(
									in[i],
									analysis()
								)
							)
						)
					)
				)
			));

		out[i] = the_sample;

		analysis.tick();
		synthesis.tick();
		slidebank.tick();
		smoothbank.tick();
		rmsbank.tick();
	}

	return 0;
}

double transp = division;
double scale[division * octaves * harmonics];
bool open[division]; // which pitch classes are on?

void defaultize()
{
	for (int i = 0; i < division * octaves; i++)
		scale[i] = i;
}

void transpose(Oscbank<double, N>* oscbank, int interval, int spectral, bool reverse = false)
{
	double ratio = pow(2, (double)interval / division);
	for (int i = 0; i < division * octaves; i++)
	{
		for (int j = 0; j < courses; j++)
		{
			for (int k = 0; k < harmonics; k++)
			{
				for (int l = 0; l < parities; l++)
				{
					double midi = (scale[i] + detune * pow((2 * (j + 0.5) / (double)courses - 1), 1)) / division;
					double partial = frequency * pow(2, midi) * (k + spectral + 1) * ratio * pow(-1, l);
	
					oscbank->freqmod(parities * (harmonics * (courses * i + j) + k) + l, (reverse ? -1 : 1) * partial);
				}
			}
		}
	}
}

void snap()
{
	bool anything = false;
	int last;
	int first;
	for (int j = 0; j < division; j++)
	{
		if (open[j] && !anything)
			first = j;
		anything |= open[j];
		if (anything)
			last = j;
	}

	if (!anything)
	{
		defaultize();
		return;
	}

	for (int i = 0; i < division * octaves; i += division)
	{
		scale[i] = (open[0] ? i : i - last);
	}

	for (int j = 1; j < division; j++)
	{
		if (open[j])
		{
			for (int i = j; i < division * octaves; i += division)
				scale[i] = i;
		}
		else
		{
			for (int i = j; i < division * octaves; i += division)
				scale[i] = scale[i - 1];
		}
	}
}

void measure(Slidebank<double, N>* slidebank, std::vector<std::complex<double>>& radii)
{
	for (int i = 0; i < division * octaves; i++)
	{
		for (int j = 0; j < courses; j++)
		{
			for (int k = 0; k < harmonics; k++)
			{
				for (int l = 0; l < parities; l++)
				{
					double midi = (scale[i] + detune * pow((2 * (j + 0.5) / (double)courses - 1), 1)) / division;
					double partial = frequency * pow(2, midi) * (k + 1) * pow(-1, l);

					radii[parities * (harmonics * (courses * i + j) + k) + l] = 
						std::min(cutoff, std::exp((std::log(0.5) - multiplicity - 1) 
													/ (wavelengths * SR / std::abs(partial))));
				}
			}
		}
	}
}

void process_midi(MidiIn* device)
{
	int pitch, velocity, index;
	device->get();

	if (device->bytes)
	{
		switch ((Status)(device->message[0]))
		{
			case note_on:
			case note_off:
				pitch = (int)device->message[1];
				velocity = (int)device->message[2];
				index = pitch - C1;
				if (index >= 0 and index < N)
				{
					if (velocity && (Status)device->message[0] != note_off)
						open[index % division] = true;
					else
						open[index % division] = false;
				}

				snap();
				transpose(&synthesis, transp, 0, true); // set frequencies of backward oscillators
				break;

			default:
				break;
		}
	}
}


bool realtime = true;
Audio A = Audio(process, BSIZE);
MidiIn MI = MidiIn();

int main()
{
	defaultize();	
	transpose(&analysis, 0, 0, false); // set frequencies of forward oscillators

	transpose(&synthesis, transp, 0, true); // set frequencies of backward oscillators

	analysis.open(); // open oscillator banks
	synthesis.open();

	std::vector<std::complex<double>> radii(N);
	measure(&slidebank, radii);
	slidebank.setup(multiplicity, radii);

	MI.startup(); // startup midi in
	MI.ignore(true, true, true); // ignore sysex, time, aftertouch
	MI.getports(false); // get ports but don't print anything
	MI.open("IAC Driver Bus 1");

	A.startup(1, 1, true); // startup audio engine; 2 input, 2 outputs, console output on

	while (running)
	{
		process_midi(&MI);

		Pa_Sleep(5);
	}

	A.shutdown(); // shutdown audio engine
	MI.shutdown(); // shutdown midi engine

	return 0;
}