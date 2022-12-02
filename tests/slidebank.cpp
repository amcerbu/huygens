#include "../src/audio.h"
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

// #define courses 1 // "strings" per notes coursed,
// #define harmonics 8 // overtones
// #define octaves 1 // number of octaves
// #define division 12 // edo
// #define parities 1
// #define N parities * division * octaves * courses * harmonics // lots of filters!

#define courses 1 // "strings" per notes coursed,
#define harmonics 1 // number of odd overtones
#define octaves 4 // number of octaves
#define division 12 // edo
#define parities 1
#define N parities * division * octaves * courses * harmonics // lots of filters!

const double detune = 0.125;
const double frequency = mtof(28); // E1

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
Signal<double, N> S;

RMS<double> rms;
Noise<double> noise;

const double dry = 0;
const double gain = 1;
const double drive = 4;
double the_sample = 0;

const int multiplicity = 2; // low-pass multiplicity

const double wavelengths = 1000;
const double cutoff = 0.9995;

// const double wavelengths = 1000;
// const double cutoff = 1;

// const double wavelengths = 50;
// const double cutoff = 1;

double softclip(double sample, double width = 0.9)
{
	if (abs(sample) < width)
		return sample;

	int sign = sgn(sample);
	double gap = sample - sign * width;
	return sign * width + (1 - width) * 2.0 / PI * atan(PI * gap / (2 * (1 - width)));
}

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		the_sample = limiter(dry * in[i] + gain * softclip(drive *
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
			)));

		out[i] = the_sample;

		analysis.tick();
		synthesis.tick();
		slidebank.tick();
		smoothbank.tick();
		rmsbank.tick();
	}

	return 0;
}

double transp = 12;
double scale[division * octaves * harmonics];
double diatonic[] = {0,2,4,5,7,9,11};

void transpose(Oscbank<double, N>* oscbank, double interval, int spectral, bool reverse = false)
{
	double ratio = pow(2, (double)interval / 12);
	for (int i = 0; i < division * octaves; i++)
	{
		for (int j = 0; j < courses; j++)
		{
			for (int k = 0; k < harmonics; k++)
			{
				for (int l = 0; l < parities; l++)
				{
					double midi = (scale[i] + detune * pow((2 * (j + 0.5) / (double)courses - 1), 1)) / division;
					double partial = frequency * pow(2, midi) * (2 * k + spectral + 1) * ratio * pow(-1, l);
	
					oscbank->freqmod(parities * (harmonics * (courses * i + j) + k) + l, (reverse ? -1 : 1) * partial);
				}
			}
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
					double partial = frequency * pow(2, midi) * (2 * k + 1) * pow(-1, l);

					radii[parities * (harmonics * (courses * i + j) + k) + l] = 
						std::min(cutoff, std::exp((std::log(0.5) - multiplicity - 1) / (wavelengths * SR / std::abs(partial))));
				}
			}
		}
	}
}

void defaultize()
{
	for (int i = 0; i < division * octaves; i++)
	{
		scale[i] = i;
	}
}

void diatonize()
{
	for (int i = 0; i < division * octaves; i++)
	{
		bool searching = true;
		for (int j = 0; j < 7; j++)
		{
			if (i % division == diatonic[j])
			{
				scale[i] = i;
				searching = false;
			}
		}
		if (searching)
		{
			scale[i] = (i > 0 ? scale[i - 1] : 0);
		}
	}
}

void microtonalize()
{
	for (int i = 0; i < division * octaves; i++)
	{
		scale[i] = (double)(division * octaves - i);
	}	
}



bool realtime = true;
Audio A = Audio(process, BSIZE);

int main()
{
	defaultize();	
	transpose(&analysis, 0, 0, false); // set frequencies of forward oscillators

	// diatonize(); // initialize scale
	// microtonalize();


	transpose(&synthesis, transp, 0, true); // set frequencies of backward oscillators

	analysis.open(); // open oscillator banks
	synthesis.open();

	std::vector<std::complex<double>> radii(N);
	measure(&slidebank, radii);
	slidebank.setup(multiplicity, radii);
	// smoothbank.setup(1, radii); 

	bool thresholds[N]; // for debugging of slidebank
	memset(thresholds, false, N * sizeof(bool));

	if (realtime)
	{
		A.startup(1, 1, true); // startup audio engine; 2 input, 2 outputs, console output on

		while (running)
		{
			// slidebank.poll(thresholds, 0.0001, 0.1 / courses);
			// for (int i = 0; i < N / courses; i++)
			// {
			// 	bool on = false;
			// 	for (int j = 0; j < courses; j++)
			// 	{
			// 		on |= thresholds[i * courses + j];
			// 	}
			// 	std::cout << (on ? "█" : " ");
			// }
			// std::cout << std::endl;

			double a;
			std::cout << "transposition: ";
			std::cin >> a;
			transpose(&synthesis, a, 0, true);
			
			Pa_Sleep(100);
		}

		A.shutdown(); // shutdown audio engine
	}
	else
	{
		std::vector<double> delta_up(N, 1);
		std::vector<double> delta_down(N, 0);

		std::cout << mixdown(slidebank(S(delta_up))) << std::endl;
		slidebank.tick();
		for (int i = 1; i < 25; i++)
		{
			std::cout << mixdown(slidebank(S(delta_down))) << std::endl;
			slidebank.tick();
		}
	}

	return 0;
}