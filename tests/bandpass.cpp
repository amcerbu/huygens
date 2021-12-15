#include "../src/audio.h"
#include "../src/synth.h"
#include "../src/filter.h"
#include "../src/noise.h"
#include <signal.h>

using namespace std;
using namespace soundmath;

#define BSIZE 128

static bool running = true;
void interrupt(int ignore)
{
	cout << " [keyboard interrupt, exiting ...]" << endl;
	running = false;
}


Filter<double> bp({0, 0, 0}, {0, 0, 0});
Filter<double> bp2({0, 0, 0}, {0, 0, 0});
Filter<double> bp3({0, 0, 0}, {0, 0, 0});
Synth<double> bplfo(&cycle, 1);
Noise<double> noise;

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		float sample = (bp(noise()) + bp2(noise()) + bp3(noise())) / 6;
		if (sample > 1 || sample < -1)
			cout << "clipping failure" << endl;

		out[2 * i] = sample;
		out[2 * i + 1] = sample;

		bp.resonant(300 + 3 * bplfo(), 0.99999);
		bp2.resonant(450 + 4.5 * bplfo(), 0.99999);
		bp3.resonant(750 + 7.5 * bplfo(), 0.99999);

		bp.tick();
		bp2.tick();
		bp3.tick();
		bplfo.tick();
		noise.tick();
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