#include "../src/audio.h"
#include "../src/noise.h"
#include "../src/filterbank.h"
#include "../src/wave.h"
using namespace soundmath;

using namespace std;
using namespace soundmath;

#define BSIZE 64

static bool running = true;
void interrupt(int ignore)
{
	cout << "\n [keyboard interrupt, exiting ...]" << endl;
	running = false;
}










double r = 0.99999;
int N = 9;
Filterbank<double> F(2, N);
Noise<double> noise;

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		out[i] = F(noise()) / (N * N * 100000);
		F.tick();
	}

	return 0;
}


Audio A = Audio(process, BSIZE);

int main()
{
	// bind keyboard interrupt to program exit
	signal(SIGINT, interrupt);

	for (int i = 0; i < N; i++)
	{
		double angle = 110 * i * (2 * PI / SR);
		double r = 0.99999;
		F.coefficients(i, {pow(0.9, i), 0, -pow(0.9, i)}, {-2 * r * cos(angle), r * r});
	}

	A.startup(1, 1, true); // startup audio engine; 1 input, 1 output, console output on

	while (running)
	{
		Pa_Sleep(10);
	}

	A.shutdown(); // shutdown audio engine

	return 0;
}
