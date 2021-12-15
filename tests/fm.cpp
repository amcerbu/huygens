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

Synth<double> osc1 = Synth<double>(&cycle, 330);
Synth<double> osc2 = Synth<double>(&cycle, 165);
Synth<double> osc3 = Synth<double>(&cycle, 1);
Synth<double> osc4 = Synth<double>(&cycle, 0.05);

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		out[2 * i] = 0.5 * (float)osc1();
		out[2 * i + 1] = 0.5 * (float)osc1();
		osc1.freqmod(330 + (165 + 165 * osc3()) * osc2());
		osc3.freqmod(330 + 330 * osc4());
		// osc1.freqmod(330 + (165 + 165 * osc3()));
		osc1.tick();
		osc2.tick();
		osc3.tick();
		osc4.tick();		
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

