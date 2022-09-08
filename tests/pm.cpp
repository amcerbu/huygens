#include "../src/audio.h"
#include "../src/midi.h"
#include "../src/synth.h"
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

Synth<double> oscA(&cycle, 100, 0, 0);
Synth<double> oscB(&cycle, 200, 0, 0);
Synth<double> oscC(&cycle, 300, 0, 0);
Synth<double> oscD(&cycle, 400, 0, 0);
Synth<double> oscE(&cycle, 500, 0, 0);
Synth<double> oscF(&cycle, 600, 0, 0);
Synth<double> oscG(&cycle, 700, 0, 0);

// Synth<double> oscA(&cycle, 420, 0, 0);
// Synth<double> oscB(&cycle, 425, 0, 0);
// Synth<double> oscC(&cycle, 916, 0, 0);
// Synth<double> oscD(&cycle, 103, 0, 0);
// Synth<double> oscE(&cycle, 103.2, 0, 0);
// Synth<double> oscF(&cycle, 600, 0, 0);
// Synth<double> oscG(&cycle, 700, 0, 0);


Synth<double> lfo(&cycle, 0.05);
double sensitivity = 0;

int n = 5;
Synth<double>* synths[] = { &oscA, &oscB, &oscC, &oscD, &oscE, &oscF, &oscG };


inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		double sample = 0;
		for (int j = 0; j < n; j++)
			sample += (*synths[j])();

		sample /= n * 10;

		out[2 * i] = (float)sample;
		out[2 * i + 1] = (float)sample;

		sensitivity = (lfo()) / 240;
		// sensitivity = 1.0 / 120;
		for (int j = 0; j < n; j++)
			for (int k = j + 1; k < n; k++)
			{
				double distance = synths[j]->lookup() - synths[k]->lookup();
				synths[k]->phasemod(distance * sensitivity);
				synths[j]->phasemod(-distance * sensitivity);
			}
		
		for (int j = 0; j < n; j++)
			synths[j]->tick();

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
		Pa_Sleep(1);
	}

	A.shutdown(); // shutdown audio engine

	return 0;
}