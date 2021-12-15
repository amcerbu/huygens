#include "../src/metro.h"
#include "../src/filter.h"
#include "../src/noise.h"
#include "../src/audio.h"
#include <signal.h>

using namespace std;
using namespace soundmath;

#define BSIZE 16

static bool running = true;
void interrupt(int ignore)
{
	cout << " [keyboard interrupt, exiting ...]" << endl;
	std::fclose(stdin);
    std::cin.setstate(std::ios::badbit); // To differenciate from EOF input
	running = false;
}

Metro<double> metronome(2); // 120 bpm
Filter<double> filter({1,0,-1}, {0});
Noise<double> noise;
double gain = 1;

int elapsed;
int lag;
bool flag;


inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		if (metronome())
		{
			gain = 1;
			lag = elapsed;
			elapsed = 0;
			flag = true;
		}
		else
		{
			gain *= 0.95;
			elapsed++;
		}

		float sample = 0.75 * filter(0.5 * gain);
		out[2 * i] = sample;
		out[2 * i + 1] = sample;

		metronome.tick();
		filter.tick();
		noise.tick();
	}

	return 0;
}



Audio A = Audio(process, BSIZE);

int main()
{
	// bind keyboard interrupt to program exit
	signal(SIGINT, interrupt);

	filter.resonant(220, 0.995);

	A.startup(1, 2, true); // startup audio engine; 1 input, 2 outputs, console output on

	while (running)
	{
		double tempo;
		cin >> tempo;
		metronome.freqmod(tempo / 60.0);
		Pa_Sleep(1);
	}

	A.shutdown(); // shutdown audio engine

	return 0;
}
