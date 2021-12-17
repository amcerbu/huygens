#include "../src/audio.h"
#include "../src/additive.h"
#include "../src/metro.h"
#include "../src/filter.h"
#include "../src/midi.h"
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

Metro<double> physics(SR); // rate of physics computation
Additive<double> addi(&cycle, 10, 11, 0.5); // sinusoids, 10 voices, 11 overtones, decay coeff 0.7

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		float sample = addi();
		out[2 * i] = sample;
		out[2 * i + 1] = sample;

		if (physics())
			addi.physics();

		addi.tick();
		physics.tick();
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