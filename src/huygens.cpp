// huygens.cpp // main // an odd kind of sympathy
#include "tests.h"
#include "audio.h"
#include "midi.h"

#include <stdbool.h>
#include "stream.h"
#include <sys/time.h>

using namespace std;

static bool running = true;
void interrupt(int ignore)
{
	cout << " [keyboard interrupt, exiting ...]" << endl;
	running = false;
}

static Audio A = Audio(process);
static MidiIn MI = MidiIn();
static MidiOut MO = MidiOut();

inline int process(const float* in, float* out)
{
	for (int i = 0; i < A.bsize; i++)
		TEST(in, out, i);

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
				pitch = (int)message[1];
				velocity = (int)message[2];
				if (velocity)
					INSTRUMENT.makenote(pitch, dbtoa(-8 * (1 - (double)velocity / 127)));
				else
					INSTRUMENT.endnote(pitch);
				break;

			case aftertouch:
				pitch = (int)message[1];
				velocity = (int)message[2];
				weight = (double)velocity / 127.0;
				// INSTRUMENT.aftertouch(pitch, 0.999 * (weight) + 0.99999 * (1 - weight));
				break;

			default:
				break;
		}
	}
}

void process_osc(UDPReceive* listener, bool report = false)
{
	static Stream<tosc_message> osc; // buffer of osc messages

	listener->read(&osc);
	while (osc.nonempty())
	{
		tosc_message msg = osc.pop();
		// tosc_printMessage(&msg);
		string address = (string)tosc_getAddress(&msg);

		static const string addresses[] = {"/shape", "/jitter", "/speed", "/warble", "/size", "/texture",
										  "/density", "/spray", "/pan", "/scatter", "/gain", "/wobble"};

		static const string voices[] = {"/1", "/2", "/3", "/4"};

		static const int num_addrs = 12;
		static const int num_voices = 4;

		for (int j = 0; j < num_voices; j++)
			for (int i = 0; i < num_addrs; i++)
				if (address.compare(voices[j] + addresses[i]) == 0)
				{
					double value = tosc_getNextDouble(&msg);
					if (report)
						cout << voices[j] + addresses[i] << " = " << value << endl;

					granaries[j].instruct(value, i);
				}
	}
}

int main()
{
	// bind keyboard interrupt to program exit
	signal(SIGINT, interrupt);

	A.startup(1, 2, true); // startup audio engine; 1 input, 2 outputs, console output on

	MI.startup(); // startup midi in
	MI.ignore(true, true, false); // listen for aftertouch messages
	MI.getports(false); // get ports but don't print anything
	MI.open("IAC Driver Bus 1");

	UDPReceive udp;
	udp.prepare(true); // listen on localhost (127.0.0.1) port 9000; console output on

	while (running)
	{
		process_midi(&MI);
		// process_osc(&udp, true);
		Pa_Sleep(1);
	}

	MI.shutdown(); // shutdown midi in
	A.shutdown(); // shutdown audio engine
	udp.shutdown(); // close socket

	return 0;
}