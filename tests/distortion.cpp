#include "../src/audio.h"
#include "../src/wave.h"
#include "../src/synth.h"
#include "../src/filter.h"
#include "../src/delay.h"
#include "../src/noise.h"
#include "../src/metro.h"
#include "../src/midi.h"

#include <signal.h>

using namespace std;
using namespace soundmath;

#define BSIZE 64

static bool running = true;
void interrupt(int ignore)
{
	cout << " [keyboard interrupt, exiting ...]" << endl;
	std::fclose(stdin);
    std::cin.setstate(std::ios::badbit); // To differenciate from EOF input
	running = false;
}

double amplitude;
double up = 0.1;
double down = 0.0001;

Synth<double> carrier(&cycle, 0);


inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		static double squared = in[2 * i] * in[2 * i];
		if (squared > amplitude)
			amplitude = up * squared + (1 - up) * amplitude;
		else
			amplitude = down * squared + (1 - down) * amplitude;

		carrier.phasemod(sin(30 * (1 + amplitude) * in[2 * i]) / 2);

		out[2 * i] = out[2 * i + 1] = 0.5 * in[2 * i] + 0.5 * sin(amplitude * carrier()/ 10 + 20 * (1 + amplitude) * in[2 * i]) / 2;

		carrier.tick();
	}

	return 0;
}

Audio A = Audio(process, BSIZE);

int main()
{
	A.startup(2, 2, true); // startup audio engine; 2 input, 2 outputs, console output on

	while (running)
	{
		Pa_Sleep(100);
	}

	A.shutdown(); // shutdown audio engine

	return 0;
}