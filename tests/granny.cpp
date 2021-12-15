#include "../src/buffer.h"
#include "../src/granulator.h"
#include "../src/metro.h"
#include "../src/filter.h"
#include "../src/synth.h"
#include "../src/audio.h"
#include "../src/udpreceive.h"
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


Buffer<double> source(3 * SR);
Granulator<double> granny(&hann, &source);
Granary<double> granaries[4];
Metro<double> metro(2);
Filter<double> bandpass({1,0,-1}, {0});
Synth<double> lfo2(&cycle, 0.05);

double offset, the_size, speed, gain, pan;

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		source.write(in[i]);
		float sample = bandpass(limiter(granny()));
		// metro.freqmod(3 + 2 * lfo2());

		out[2 * i] = sample;
		out[2 * i + 1] = sample;

		for (int j = 0; j < 4; j++)
			if (granaries[j].parameters(&offset, &the_size, &speed, &gain, &pan))
			{
				// cout << "size = " << the_size / 1000.0 << "; speed = " << speed << endl;
				granny.request(0, the_size / 1000.0, speed, gain, 0);
			}

		source.tick();
		granny.tick();
		for (int j = 0; j < 4; j++)
			granaries[j].tick();
		metro.tick();
		lfo2.tick();
		bandpass.tick();
	}

	return 0;
}

Audio A = Audio(process, BSIZE);

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
	UDPReceive udp;
	udp.prepare(true); // listen on localhost (127.0.0.1) port 9000; console output on

	while (running)
	{
		process_osc(&udp, true);
		Pa_Sleep(1);
	}

	A.shutdown(); // shutdown audio engine

	return 0;
}

