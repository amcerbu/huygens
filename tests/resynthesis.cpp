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

const int N = 256;
const double R = 0.99;
Filterbank<double> bank(2, N);

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		out[2 * i] = out[2 * i + 1] = limiter(bank((double)in[i] / (N / 3)));
		bank.tick();
	}

	return 0;
}

Audio A = Audio(process, BSIZE);

int main()
{
	for (int i = 0; i < N; i++)
	{
		double cosine = cos((PI * i) / N);
		bank.coefficients(1.0 / N, {1}, {0, -2 * R * cosine, R * R}, i);
	}

	A.startup(1, 2, true); // startup audio engine; 1 input, 2 outputs, console output on


	while (running)
	{
		Pa_Sleep(100);
	}

	A.shutdown(); // shutdown audio engine

	return 0;
}