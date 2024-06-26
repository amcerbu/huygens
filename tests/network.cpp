#include "../src/audio.h"
#include "../src/noise.h"
#include "../src/network.h"

using namespace soundmath;

// using namespace std;
using namespace soundmath;

#define BSIZE 128

static bool running = true;
void interrupt(int ignore)
{
	std::cout << "\n [keyboard interrupt, exiting ...]" << std::endl;
	running = false;
}

// compute transfer function of our bandpass filter with params freq, r at incoming frequency f
std::complex<double> transfer(double frequency, double r, double f)
{
	std::complex<double> z(cos(2 * PI * f / SR), -sin(2 * PI * f / SR));
	return (1.0 - z * z) / (1.0 - 2 * r * cos(2 * PI * frequency / SR) * z + r * r * z * z);
}

double softclip(double sample, double width)
{
	if (abs(sample) < width)
		return sample;

	int sign = sgn(sample);
	double gap = sample - sign * width;
	return sign * width + (1 - width) * 2.0 / PI * atan(PI * gap / (2 * (1 - width)));
}

double softclip(double sample)
{
	return softclip(sample, 0.125);
}

double saturate(double sample)
{
	return 2.0 / PI * atan(2 * PI * sample / 2.0);
}

const int courses = 9; // "strings" per notes coursed,
const int octaves = 8; // number of octaves
const double division = 12; // edo
const double detune = 0.125;
const int N = division * octaves * courses; // lots of filters!
double frequency = 27.5; // A0
int A0 = 21;
double tail = 100;
double r = relaxation(tail);

double rolloff = 1;
double gains[N];
double amplitudes[N];

Network<std::complex<double>> F(2, N, 0.001, 1);

// Noise<double> noise;
// RMS<double> rms;
// double air = 0;
// double noisefloor = 1;
// // double air = 0.25;
// double airmod = 0.125;
// // double airmod = 0;
double dry = 1;
// double distorted = 0.1;
// double drive = 5;
// double follow = 0;

double the_sample = 0;

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		the_sample = softclip(dry * in[i] + std::real((1 - dry) * F(in[i])), 0.9);

		out[i] = the_sample;
		F.tick();
	}

	return 0;
}


void coefficients()
{
	F.coefficients({1.0, -1.0}, {-2 * r, r * r});

	for (int i = 0; i < division * octaves; i++)
	{
		for (int j = 0; j < courses; j++)
		{
			double partial = frequency * pow(2, (i + detune * pow((2 * (j + 0.5) / (double)courses - 1), 1)) / division);

			double angle = partial * (2 * PI / SR);
			gains[courses * i + j] = 1.0 / abs(transfer(partial, r, partial));

			F.frequency(courses * i + j, exp(std::complex<double>(0, angle)), exp(std::complex<double>(0, -2 * angle)));
		}
	}

	// F.mix(std::vector<std::complex<double>>(gains, gains + N));
	// F.boost(std::vector<std::complex<double>>(N, 1));
}



Audio A = Audio(process, BSIZE);

int main()
{
	// bind keyboard interrupt to program exit
	signal(SIGINT, interrupt);

	return 0;
	
	coefficients();
	memset(amplitudes, 0, sizeof(double) * N);
	A.startup(1, 1, true); // startup audio engine; 1 input, 1 output, console output on

	while (running)
	{
		Pa_Sleep(20);
	}

	A.shutdown(); // shutdown audio engine

	return 0;
}
