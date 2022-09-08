#include "../src/audio.h"
#include "../src/noise.h"
#include "../src/filterbank.h"
#include "../src/wave.h"
#include "../src/synth.h"

using namespace soundmath;

using namespace soundmath;

#define BSIZE 64

static bool running = true;
void interrupt(int ignore)
{
	std::cout << "\n [keyboard interrupt, exiting ...]" << std::endl;
	running = false;
}

const int N = 800;
Filterbank<double> F(2, N);
Noise<double> noise;
Synth<double> lfo(&cycle, 2);

double gains[N];
double r = 0.9999;
double decay = 0.85;
double fundamental = 40;

std::complex<double> transfer(double frequency, double r, double f);

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		out[i] = F(noise());

		F.tick();
		noise.tick();
	}

	return 0;
}

// compute transfer function of our bandpass filter with params freq, r at incoming frequency f
std::complex<double> transfer(double frequency, double r, double f)
{
	std::complex<double> z(cos(2 * PI * f / SR), -sin(2 * PI * f / SR));
	return (1.0 - z * z) / (1.0 - 2 * r * cos(2 * PI * frequency / SR) * z + r * r * z * z);
}

Audio A = Audio(process, BSIZE);

int main()
{
	// bind keyboard interrupt to program exit
	signal(SIGINT, interrupt);

	for (int i = 0; i < N; i++)
	{
		// double partial = mtof((ftom(fundamental) + offset * (i + 1)));
		double partial = fundamental * (1 + i);
		double angle = partial * (2 * PI / SR);
		gains[i] = 1.0 / abs(transfer(partial, r, partial));
		F.coefficients(i, {pow(decay, i), 0, -pow(decay, i)}, {-2 * r * cos(angle), r * r});
	}

	F.mix(std::vector<double>(gains, gains + N));
	F.boost(std::vector<double>(N, 1));

	A.startup(1, 1, true); // startup audio engine; 1 input, 1 output, console output on

	while (running)
	{
		Pa_Sleep(10);
	}

	A.shutdown(); // shutdown audio engine

	return 0;
}
