#include "../src/synth.h"
#include "../src/audio.h"

#include <signal.h>

using namespace std;
using namespace soundmath;

#define BSIZE 16

static bool running = true;
void interrupt(int ignore)
{
	cout << " [keyboard interrupt, exiting ...]" << endl;
	running = false;
}

Synth<double> osc = Synth<double>(&cycle, 165);
Synth<double> lfo = Synth<double>(&cycle, 0.05);

double hardclip(double input)
{
	return (input > 1 ? 1 : (input < -1 ? -1 : input));
}

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		out[2 * i] = out[2 * i + 1] = limiter(in[i] * (lfo() * 20));
		
		osc.tick();
		lfo.tick();
	}

	return 0;
}

Audio A = Audio(process, BSIZE);

int main()
{
	// bind keyboard interrupt to program exit
	signal(SIGINT, interrupt);

	A.startup(1, 2, true); // startup audio engine; 1 input, 2 outputs, console output on
	
	while (running)
	{
		Pa_Sleep(10);
	}

	A.shutdown(); // shutdown audio engine

	return 0;
}

