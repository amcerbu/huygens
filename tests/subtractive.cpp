#include "../src/audio.h"
#include "../src/subtractive.h"
#include "../src/noise.h"
#include "../src/metro.h"
#include "../src/midi.h"
#include "../src/delay.h"
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

Metro<double> physics(SR / 32);
Subtractive<double> sub(7, 7, 0.7, 0.999995, 1, 1);
Noise<double> excitation;

double boost = 1;
double attenuation = 1;
double air = 0.5;
double gain;
double the_sample;

Filter<double> smoother({0.0001},{0,-0.9999});
Filter<double> nyquist({1,0.5}, {0});
Delay<double> chandelay(1, SR);

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		the_sample = sub(excitation());
		out[2 * i] = the_sample; // limiter(air * the_sample) + attenuation * limiter(boost * smoother(gain) * the_sample);
		out[2 * i + 1] = the_sample;
		// out[2 * i + 1] = chandelay(the_sample);

		if (physics())
			sub.physics();

		sub.tick();
		physics.tick();
		excitation.tick();
		smoother.tick();
		// chandelay.tick();
	}

	return 0;
}

unsigned char pressures[128];

void process_midi(MidiIn* device)
{
	static vector<unsigned char> message;
	static int nBytes;
	static double stamp;

	stamp = device->get(&message);
	nBytes = message.size();
	int pitch, velocity;
	double weight;

	if (nBytes)
	{
		switch ((Status)message[0])
		{
			case note_on:
			case note_off:
				pitch = (int)message[1];
				velocity = (int)message[2];
				if (velocity && (Status)message[0] != note_off)
				{
					sub.makenote(pitch, dbtoa(-8 * (1 - (double)velocity / 127)));
				}
				else
				{
					sub.endnote(pitch); 
					pressures[pitch] = 0;
				}
				break;

			case aftertouch:
				pitch = (int)message[1];
				velocity = (int)message[2];

				pressures[pitch] = velocity;
				weight = 0;
				for (int i = 0; i < 128; i++)
				{
					double w = (double)pressures[i] / 127;
					weight += w * w;
				}

				gain = sqrt(weight);
				// sub.aftertouch(pitch, 0.999 * (weight) + 0.99999 * (1 - weight));
				break;

			default:
				break;
		}
	}
}


Audio A = Audio(process, BSIZE);
MidiIn MI = MidiIn();

int main()
{
	memset(pressures, 0, 128 * sizeof(unsigned char));
	chandelay.coefficients({{2000,1}},{});
	// bind keyboard interrupt to program exit
	signal(SIGINT, interrupt);

	MI.startup(); // startup midi in
	MI.ignore(true, true, true); // ignore sysex, time, but not aftertouch
	MI.getports(false); // get ports but don't print anything
	MI.open("IAC Driver Bus 1");

	A.startup(1, 2, true); // startup audio engine; 1 input, 1 output, console output on

	while (running)
	{
		process_midi(&MI);

		Pa_Sleep(1);
	}

	A.shutdown(); // shutdown audio engine
	MI.shutdown();

	return 0;
}