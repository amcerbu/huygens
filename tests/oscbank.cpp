#include "../src/audio.h"
#include "../src/oscbank.h"
#include "../src/synth.h"
#include "../src/rms.h"
#include "../src/stickbank.h"
#include "../src/mixer.h"
#include "../src/modbank.h"
#include "../src/distbank.h"
#include "../src/noise.h"
#include "../src/delay.h"
#include "../src/filter.h"
#include "../src/filterbank.h"
#include "../src/buffer.h"
#include "../src/latchbank.h"
#include "../src/rmsbank.h"

#include <signal.h>

using namespace std;
using namespace soundmath;

#define courses 1 // "strings" per notes coursed,
#define harmonics 1 // overtones
#define octaves 6 // number of octaves
#define division 12 // edo
#define parities 1
#define N parities * division * octaves * courses * harmonics // lots of filters!

const double detune = 0.125;
const double frequency = 27.5; // A0

#define BSIZE 64

static bool running = true;
void interrupt(int ignore)
{
	cout << " [keyboard interrupt, exiting ...]" << endl;
	std::fclose(stdin);
    std::cin.setstate(std::ios::badbit); // To differenciate from EOF input
	running = false;
}

std::complex<double> id(std::complex<double> x)
{
	return x;
}

std::complex<double> gate(std::complex<double> x)
{
	return (std::abs(x) < 0.001 ? 0 : x);
}

std::complex<double> quad(std::complex<double> x)
{
	return x * x / std::abs(x);
}

std::complex<double> angle(std::complex<double> x)
{
	return 0.01 * (abs(x) > 0.001) * (std::arg(x) > 0 ? 1 : -1);
}


Oscbank<double, N> oscbank1; // modulators
Oscbank<double, N> oscbank2; // demodulators

Stickbank<double, N> stickbank(1, -0.999); // low-pass filters
Stickbank<double, N> stickbank2(1, -0.999);
Stickbank<double, N> stickbank3(1, -0.999);
Stickbank<double, N> stickbank4(1, -0.999);

Distbank<double, N> distbank(&gate); // channel distortion
Latchbank<double, N> latchbank(0.005);
RMSbank<double, N> rmsbank;
Distbank<double, N> sawmill(&angle);
Filter<double> smoother({0.1}, {0, -0.9}); // low-pass

Mixer<double, N> mixdown;

Modbank<double, N> modbank1; // modulation
Modbank<double, N> modbank2; // demodulation
Filterbank<double> F(2, N, 0.001, 1);

Noise<double> rnoise;
Noise<double> inoise;

Synth<double> lfo(&cycle, 1.5);
Delay<double> delay(1, SR);

Buffer<double> buffer(SR / 300);

RMS<double> rms;
bool emitted = false;
bool edge = false;
double thresh = 0.02;

bool ready = false;
double the_sample = 0;
double depth = 0;
double dry = 0;

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		the_sample = dry * in[i] +
			mixdown(
					modbank2(
						oscbank2(),
						stickbank4(distbank(stickbank3(stickbank2(stickbank(modbank1(in[i],oscbank1()))))))));

		buffer.write(the_sample);		

		out[i] = buffer(depth * (1 + lfo()));

		oscbank1.tick();
		oscbank2.tick();
		
		stickbank.tick();
		stickbank2.tick();
		stickbank3.tick();
		stickbank4.tick();
		lfo.tick();

		rmsbank.tick();
		buffer.tick();
	}

	return 0;
}


double transp = 12;
double scale[division * octaves * harmonics];
double diatonic[] = {0,2,4,5,7,9,11};


// compute transfer function of our bandpass filter with params freq, r at incoming frequency f
std::complex<double> transfer(double frequency, double r, double f)
{
	std::complex<double> z(cos(2 * PI * f / SR), -sin(2 * PI * f / SR));
	return (1.0 - z * z) / (1.0 - 2 * r * cos(2 * PI * frequency / SR) * z + r * r * z * z);
}



// double gains[N];
// double tail = 50;
// double r = relaxation(tail);


void transpose(int interval)
{
	transp = pow(2, (double)interval / 12);
	for (int i = 0; i < division * octaves; i++)
	{
		for (int j = 0; j < courses; j++)
		{
			for (int k = 0; k < harmonics; k++)
			{
				for (int l = 0; l < parities; l++)
				{
					double midi = (scale[i] + detune * pow((2 * (j + 0.5) / (double)courses - 1), 1)) / division;
					double partial = -frequency * pow(2, midi) * (k + 1) * transp * pow(-1, l);
					// double angle = partial * (2 * PI / SR);

					oscbank2.freqmod(harmonics * (courses * i + j) + k, partial);
					// gains[courses * i + j] = 1.0 / abs(transfer(partial, r, partial));
					// F.coefficients(harmonics * (courses * i + j) + k, {1.0, 0, -1.0}, {-2 * r * cos(angle), r * r});
				}
			}
		}
	}
	// F.mix(std::vector<double>(gains, gains + N));
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
		scale[i] = (double)i / 2;
	}	
}

// void coefficients()
// {
// 	for (int i = 0; i < division * octaves; i++)
// 	{
// 		for (int j = 0; j < courses; j++)
// 		{
// 			for (int k = 0; k < harmonics; k++)
// 			{
// 				for (int l = 0; l < parities; l++)
// 				{
// 					double partial = frequency * pow(2, (i + detune * pow((2 * (j + 0.5) / (double)courses - 1), 1)) / division) * (1 + k);
// 					double angle = partial * (2 * PI / SR) * pow(-1, l);
// 					gains[courses * i + j] = 1.0 / abs(transfer(partial, r, partial));
// 					F.coefficients(harmonics * (courses * i + j) + k, {1.0, 0, -1.0}, {-2 * r * cos(angle), r * r});

// 				}
// 			}
// 		}
// 	}

// 	F.mix(std::vector<double>(gains, gains + N));
// 	F.boost(std::vector<double>(N, 0));
// }


bool thresholds[N];
Audio A = Audio(process, BSIZE);

int main()
{
	memset(thresholds, false, N * sizeof(bool));

	// diatonize();
	// microtonalize();
	defaultize();
	// coefficients();

	for (int i = 0; i < division * octaves; i++)
	{
		for (int j = 0; j < courses; j++)
		{
			for (int k = 0; k < harmonics; k++)
			{
				for (int l = 0; l < parities; l++)
				{
					double partial = frequency * pow(2, (i + detune * pow((2 * (j + 0.5) / (double)courses - 1), 1)) / division) * (1 + k);
					oscbank1.freqmod(harmonics * (courses * i + j) + k, partial * pow(-1, l));
				}
			}
		}
	}

	transpose(transp);

	oscbank1.open();
	oscbank2.open();

	delay.coefficients({{SR / 2.0, 1}},{}); // 1-second delay

	// int a;
	// std::cout << "transposition: ";
	// std::cin >> a;
	// transpose(a);

	A.startup(1, 1, true); // startup audio engine; 2 input, 2 outputs, console output on

	while (running)
	{
		// stickbank4.poll(thresholds, 0.01, 0.1 / courses);
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

		int a;
		std::cout << "transposition: ";
		std::cin >> a;
		transpose(a);

		
		Pa_Sleep(100);
	}

	A.shutdown(); // shutdown audio engine

	// delete stickout;

	return 0;
}