#include "../src/audio.h"
#include "../src/fourier.h"
#include "../src/wave.h"
#include "../src/filter.h"
#include "../src/delay.h"
#include "../src/synth.h"
#include "../src/subtractive.h"
#include "../src/noise.h"
#include "../src/metro.h"
#include "../src/midi.h"
#include "../src/buffer.h"
#include "../src/granulator.h"
#include "../src/udpreceive.h"

#include <signal.h>

using namespace std;
using namespace soundmath;

#define BSIZE 256
#define N 512
#define LAPS 64

static bool running = true;
void interrupt(int ignore)
{
	cout << " [keyboard interrupt, exiting ...]" << endl;
	std::fclose(stdin);
    std::cin.setstate(std::ios::badbit); // To differenciate from EOF input
	running = false;
}

inline int f_process(const complex<double>* in, complex<double>* out)
{
	long double average = 0;
	for (int i = 0; i < N; i++)
	{
		out[i] = 0;
		average += abs(in[i]);
	}

	average /= N;

	for (int i = 0; i < N; i++)
	{
		if (norm(in[i]) < 100 * average * average)
			out[i] = 0;

		else
			out[i] = in[i];
	}

	return 0;
}

Fourier F1 = Fourier(f_process, N, LAPS);
Delay<double> delay({1, N});

Buffer<double> source(3 * SR);
Granulator<double> granny(&hann, &source);
Granary<double> granaries[4];
Filter<double> bandpass({1,0,-1}, {0});
double offset, the_size, speed, gain, pan;

Metro<double> physics(2000);
Subtractive<double> sub(7, 7, 0.7);
Noise<double> excitation;

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		source.write(in[2 * i]);
		F1.write(granny());

		static double real, imag;
		F1.read(&real, &imag);
		out[2 * i] = out[2 * i + 1] = limiter(real);

		for (int j = 0; j < 4; j++)
			if (granaries[j].parameters(&offset, &the_size, &speed, &gain, &pan))
				granny.request(0, the_size / 1000.0, speed, gain, 0);

		// if (physics())
		// 	sub.physics();

		delay.tick();
		source.tick();
		granny.tick();
		for (int j = 0; j < 4; j++)
			granaries[j].tick();
		// bandpass.tick();
		// excitation.tick();
		// sub.tick();

	}

	return 0;
}

void process_osc(UDPReceive* listener, bool report = false)
{
	static Stream<tosc_message> osc; // buffer of osc messages

	listener->read(&osc);
	while (osc.nonempty())
	{
		tosc_message msg = osc.pop();
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
				}
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
	MI.startup(); // startup midi in
	MI.ignore(true, true, false); // listen for aftertouch messages
	MI.getports(false); // get ports but don't print anything
	MI.open("IAC Driver Bus 1");

	delay.coefficients({{8 * N, 1}}, {});
	A.startup(2, 2, true); // startup audio engine; 2 input, 2 outputs, console output on

	UDPReceive udp;
	udp.prepare(true); // listen on localhost (127.0.0.1) port 9000; console output on

	while (running)
	{
		process_osc(&udp, true);
		process_midi(&MI);
		Pa_Sleep(1);
	}

	A.shutdown(); // shutdown audio engine
	MI.shutdown();

	return 0;
}