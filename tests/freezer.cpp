#include "../src/audio.h"
#include "../src/fourier.h"
#include "../src/wave.h"
#include "../src/filter.h"
#include "../src/delay.h"
#include "../src/synth.h"
#include <signal.h>

using namespace std;
using namespace soundmath;

#define BSIZE 256
#define N 2048
#define LAPS 8

static bool running = true;
void interrupt(int ignore)
{
	cout << " [keyboard interrupt, exiting ...]" << endl;
	std::fclose(stdin);
    std::cin.setstate(std::ios::badbit); // To differenciate from EOF input
	running = false;
}


Freezer<N> freezer(LAPS, 1);
Delay<double> delay(1, N);

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		out[2 * i] = out[2 * i + 1] = freezer(in[2 * i]);
		// out[2 * i] = out[2 * i + 1] = delay(in[2 * i]);
		// delay.tick();
	}

	return 0;
}

Audio A = Audio(process, BSIZE);
int a;

int main()
{
	delay.coefficients({{N,1}}, {});
	A.startup(2, 2, true); // startup audio engine; 2 input, 2 outputs, console output on

	while (running)
	{
		cin >> a;
		if (a > 0)
			freezer.freeze();
		else
			freezer.unfreeze();
	}

	A.shutdown(); // shutdown audio engine

	return 0;
}