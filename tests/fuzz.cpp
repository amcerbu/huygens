#include "../src/audio.h"
#include "../src/synth.h"
#include "../src/filter.h"
#include "../src/buffer.h"
#include "../src/metro.h"
#include "../src/noise.h"

#include <signal.h>

using namespace std;
using namespace soundmath;

#define BSIZE 64

static bool running = true;
void interrupt(int ignore)
{
	cout << " [keyboard interrupt, exiting ...]" << endl;
	std::fclose(stdin);
    std::cin.setstate(std::ios::badbit); // To differentiate from EOF input
	running = false;
}

Buffer<double> history(6 * SR);
Metro<double> metro(10); // randomizes delay times
Filter<double> twozero({1,0,-1},{0}); // filters DC and Nyquist
Synth<double> tri(&triangle, 2);

Filter<double>** smoothers;
Noise<double>** noises;
Synth<double>** tris;
Filter<double> follower({0.0001},{0,-0.9999});

const int voices = 2;

double delay = SR;
double gain = 1;
double determinism = 0.9;
double headroom = 1;
double feedback = 0.9999;
double outsample = 0;
double dissipation = 0.1;
double mix = 1;

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		history.write((in[2 * i + 1] + in[2 * i]));

		outsample = 0;
		for (int j = 0; j < voices; j++) // each voice contributes some doppler'd lookup
			// outsample += history((*smoothers[j])(delay * determinism + delay * (1 - determinism) * (*noises[j])(true)));
			outsample += history((*smoothers[j])(delay * (1 - (0.5 + 0.5 * (*tris[j])()))));

		outsample /= (double)voices; // scaling
		outsample = limiter(gain * outsample) / gain; // distortion
		outsample = dissipation * twozero(outsample) + (1 - dissipation) * outsample; // filtering
		// outsample = feedback * outsample + history(0);

		out[2 * i] = limiter((1 - mix) * (in[2 * i + 1] + in[2 * i]) + mix * outsample);
		out[2 * i + 1] = limiter((1 - mix) * (in[2 * i + 1] + in[2 * i]) + mix * outsample);

		history.accum(feedback * outsample);

		if (metro())
		{
			for (int j = 0; j < voices; j++)
				noises[j]->tick();
		}
		
		for (int j = 0; j < voices; j++)
		{
			// tris[j]->freqmod((1 + j) * int(sqrt(follower(int(5000 * outsample * outsample)))));
			smoothers[j]->tick();
			tris[j]->tick();
		}

		history.tick();
		metro.tick();
		twozero.tick();
		tri.tick();
		follower.tick();
	}

	return 0;
}

static Audio A = Audio(process, BSIZE);


int main()
{
	smoothers = new Filter<double>*[voices];
	noises = new Noise<double>*[voices];
	tris = new Synth<double>*[voices];

	for (int j = 0; j < voices; j++)
	{
		// smoothers[j] = new Filter<double>({0.0001, 0}, {0,-0.9999});
		smoothers[j] = new Filter<double>({0.01, 0}, {0,-0.99});
		noises[j] = new Noise<double>(0, 1);
		tris[j] = new Synth<double>(&triangle, pow(2, j), j / (double)voices);
	}

	A.startup(2, 2, true); // startup audio engine; 2 input, 2 outputs, console output on

	while (running)
	{
		cin >> delay;
		if (delay < 0)
			delay = 0;
		else if (delay > SR * 6)
			delay = SR * 6;
		
		Pa_Sleep(1);
	}

	A.shutdown(); // shutdown audio engine

	for (int j = 0; j < voices; j++)
	{
		delete smoothers[j];
		delete noises[j];
	}

	delete [] smoothers;
	delete [] noises;
	delete [] tris;


	return 0;
}