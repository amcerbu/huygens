#include "../src/audio.h"
#include "../src/synth.h"
#include "../src/filter.h"
#include "../src/filterbank.h"
#include "../src/noise.h"
#include "../src/delay.h"
#include <signal.h>
#include <map>

using namespace std;
using namespace std::complex_literals;
using namespace soundmath;

#define BSIZE 1024

bool running = true;
void interrupt(int ignore)
{
	cout << " [keyboard interrupt, exiting ...]" << endl;
	running = false;
}

std::complex<double> transfer(double frequency, double r, double f)
{
	std::complex<double> z(cos(2 * PI * f / SR), -sin(2 * PI * f / SR));
	return (1.0 - z * z) / (1.0 - 2 * r * cos(2 * PI * frequency / SR) * z + r * r * z * z);
}

const int N = 20;
const double R = 0.999;
Filterbank<double> bank(2, N);

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		out[i] = limiter(bank((double)in[i]) / N);
		bank.tick();
	}

	return 0;
}

Audio A = Audio(process, BSIZE);

int main()
{
	for (int i = 0; i < N; i++)
	{
		double frequency = 0.5 * (i + 1) * SR / N;
		double cosine = cos(2 * PI * frequency / SR);
		double gain = abs(transfer(frequency, R, frequency));
		bank.coefficients(i, {1.0 / gain, 0, -1.0 / gain}, {-2 * R * cosine, R * R});
	}

	A.startup(1, 1, true); // startup audio engine; 1 input, 2 outputs, console output on


	while (running)
	{
		Pa_Sleep(100);
	}

	A.shutdown(); // shutdown audio engine

	return 0;
}