#include "../src/audio.h"
#include "../src/noise.h"
#include "../src/delay.h"
#include "../src/rms.h"
#include "../src/synth.h"
#include "../src/filter.h"
using namespace soundmath;

using namespace std;
using namespace soundmath;

#define BSIZE 64


// clips (-infty, infty) to (-1, 1); linear in (-width, width)
double softclip(double sample, double width = 0.5)
{
	if (abs(sample) < width)
		return sample;

	int sign = sgn(sample);
	double gap = sample - sign * width;
	return sign * width + (1 - width) * 2.0 / PI * atan(PI * gap / (2 * (1 - width)));
}

// compute transfer function of our bandpass filter with params freq, r at incoming frequency f
std::complex<double> transfer(double frequency, double r, double f)
{
	std::complex<double> z(cos(2 * PI * f / SR), -sin(2 * PI * f / SR));
	return (1.0 - z * z) / (1.0 - 2 * r * cos(2 * PI * frequency / SR) * z + r * r * z * z);
}

static bool running = true;
void interrupt(int ignore)
{
	cout << "\n [keyboard interrupt, exiting ...]" << endl;
	running = false;
}

double frequency = 220;
double r = 0.9999;

Filter<double> bandpass({1,0,-1},{0, -2 * r * cos(2 * PI * frequency / SR), r * r});
std::complex<double> scaling = transfer(frequency, r, frequency); // transfer function at center frequency
double gain = abs(scaling);
double phase = std::arg(scaling);
Delay<double> aligner(1, SR / frequency);

RMS<double> rms, rms2;
Synth<double> carrier(&cycle, frequency);
Synth<double> fmodder(&cycle, frequency / 2);


inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		double amp = softclip(10 * rms(in[i]));
		double the_sample = carrier();
		double filtered = bandpass(the_sample);

		out[i] = 0.5 * (the_sample - aligner(filtered) / gain);
		carrier.freqmod(frequency + frequency * amp * fmodder());
		fmodder.freqmod(frequency / 2 + frequency / 2 * amp);
		rms.tick();
		rms2.tick();
		carrier.tick();
		fmodder.tick();
		bandpass.tick();
		aligner.tick();
	}

	return 0;
}


Audio A = Audio(process, BSIZE);

int main()
{
	// bind keyboard interrupt to program exit
	signal(SIGINT, interrupt);

	aligner.coefficients({{(int)(frequency / SR * (PI - phase)), 1}}, {});
	A.startup(1, 1, true); // startup audio engine; 1 input, 1 output, console output on

	while (running)
	{
		Pa_Sleep(10);
	}

	A.shutdown(); // shutdown audio engine

	return 0;
}
