#include "../src/audio.h"
#include "../src/midi.h"
#include "../src/delay.h"
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

Delay<double> delay(10, 2 * SR); // 10 delay lines, buffer size 2 seconds

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		float sample = delay(in[i]);
		out[2 * i] = sample;
		out[2 * i + 1] = sample;

		delay.tick();
	}

	return 0;
}

Audio A = Audio(process, BSIZE);

int main()
{
	// bind keyboard interrupt to program exit
	signal(SIGINT, interrupt);

	delay.coefficients({{0,1}}, {{20000,0.5}, {30000,0.5}});
	A.startup(1, 2, true); // startup audio engine; 1 input, 2 outputs, console output on

	while (running)
	{
		Pa_Sleep(1);
	}

	A.shutdown(); // shutdown audio engine

	return 0;
}