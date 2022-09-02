#include "../src/audio.h"
#include "../src/additive.h"
#include "../src/metro.h"
#include "../src/filter.h"
#include "../src/midi.h"
#include "../src/delay.h"
#include <signal.h>

using namespace std;
using namespace soundmath;

#define BSIZE 64

static bool running = true;
void interrupt(int ignore)
{
	cout << " [keyboard interrupt, exiting ...]" << endl;
	running = false;
}

// Metro<double> physics(SR); // rate of physics computation
Metro<double> physics(SR / 2); // rate of physics computation
Additive<double> addi(&cycle, 10, 7, 0.66, 1); // sinusoids, 10 voices, 11 overtones, decay coeff 0.7
// Additive<double> addi(&cycle, 10, , 0.5); // triangle waves, 10 voices, 7 overtones, decay coeff 0.5
Filter<double> smoother1({1, 4, 6, 4, 1}, {0}); // quadruple zero at Nyquist
Filter<double> smoother2({1,-1}, {0, -0.999}); // low-pass

Delay<double> chandelay(1, SR);

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		float sample = (addi());
		out[2 * i] = sample;
		out[2 * i + 1] = sample;
		// out[2 * i + 1] = chandelay(sample);

		if (physics())
			addi.physics();

		addi.tick();
		physics.tick();
		// smoother.tick();
		// chandelay.tick();
		smoother2.tick();
		// smoother2.tick();
	}

	return 0;
}

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
					addi.makenote(pitch, dbtoa(-8 * (1 - (double)velocity / 127)));
				else
					addi.endnote(pitch);
				break;

			case aftertouch:
				pitch = (int)message[1];
				velocity = (int)message[2];
				weight = (double)velocity / 127.0;
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
	chandelay.coefficients({{2000,1}},{});

	// bind keyboard interrupt to program exit
	signal(SIGINT, interrupt);

	MI.startup(); // startup midi in
	MI.ignore(true, true, false); // listen for aftertouch messages
	MI.getports(false); // get ports but don't print anything
	MI.open("IAC Driver Bus 1");

	A.startup(1, 2, true); // startup audio engine; 1 input, 2 outputs, console output on

	while (running)
	{
		process_midi(&MI);

		Pa_Sleep(1);
	}

	A.shutdown(); // shutdown audio engine
	MI.shutdown();

	return 0;
}