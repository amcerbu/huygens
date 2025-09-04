#include "../src/audio.h"
#include "../src/subtractive.h"
#include "../src/noise.h"
#include "../src/metro.h"
#include "../src/midi.h"
#include "../src/delay.h"
#include "../src/argparse.h"
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

// parameters configurable with command-line args
int OUTPUT_OFFSET; // ""
int INDEVICE; // input device
int OUTDEVICE; // ""

void args(int argc, char *argv[])
{
	argparse::ArgumentParser program("subtractive");

	int def_in = 0;
	int def_out = 0;
	Audio::initialize(false, &def_in, &def_out);

	program.add_argument("-i", "--input")
		.default_value<int>((int)def_in)
		.required()
		.scan<'i', int>()
		.help("device id for audio in");

	program.add_argument("-o", "--output")
		.default_value<int>((int)def_out)
		.required()
		.scan<'i', int>()
		.help("device id for audio out");

	program.add_argument("-oc", "--outchan")
		.default_value<int>(1)
		.required()
		.scan<'i', int>()
		.help("output channel offset");

	program.add_argument("-d", "--devices")
		.help("list audio device names")
		.default_value(false)
		.implicit_value(true);

	try
	{
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err)
	{
		std::cerr << err.what() << std::endl;
		std::cerr << program;
		std::exit(1);
	}

	INDEVICE = program.get<int>("-i");
	OUTDEVICE = program.get<int>("-o");
	OUTPUT_OFFSET = program.get<int>("-oc");

	Audio::initialize(!(program.is_used("-i") && program.is_used("-o") && program.is_used("-oc")) || program.is_used("-d"));
}


Metro<double> physics(SR / 32);
Subtractive<double, 2> sub(7, 7, 0.7, 0.999995, 1, 1);
Noise<double> excitation1;
Noise<double> excitation2;

double boost = 1;
double attenuation = 1;
double air = 0.5;
double gain;
double the_sample;

Filter<double> smoother({0.0001},{0,-0.9999});
Filter<double> nyquist({1,0.5}, {0});
Delay<double> chandelay(1, SR);

double width = 2;

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		std::array<double, 2> the_sample = sub({excitation1(), excitation2()});
		out[OUTPUT_OFFSET + (OUTPUT_OFFSET + 2) * i]     = (the_sample[0] + the_sample[1]) / 2 - width * (the_sample[0] - the_sample[1]) / 2; // limiter(air * the_sample) + attenuation * limiter(boost * smoother(gain) * the_sample);
		out[OUTPUT_OFFSET + (OUTPUT_OFFSET + 2) * i + 1] = (the_sample[0] + the_sample[1]) / 2 + width * (the_sample[0] - the_sample[1]) / 2;
		// out[2 * i + 1] = chandelay(the_sample);

		if (physics())
			sub.physics();

		sub.tick();
		physics.tick();
		excitation1.tick();
		excitation2.tick();
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
	// static double stamp = device->get(&message);

	device->get(&message);

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

int main(int argc, char* argv[])
{
	args(argc, argv);

	memset(pressures, 0, 128 * sizeof(unsigned char));
	chandelay.coefficients({{2000,1}},{});
	// bind keyboard interrupt to program exit
	signal(SIGINT, interrupt);

	MI.startup(); // startup midi in
	MI.ignore(true, true, true); // ignore sysex, time, but not aftertouch
	MI.getports(false); // get ports but don't print anything
	MI.open("IAC Driver Bus 1");

	A.startup(1, 2 + OUTPUT_OFFSET, true, INDEVICE, OUTDEVICE); // startup audio engine; 4 inputs, 1 outputs, console output on
	// A.startup(1, 2, true); // startup audio engine; 1 input, 1 output, console output on

	while (running)
	{
		process_midi(&MI);

		Pa_Sleep(1);
	}

	A.shutdown(); // shutdown audio engine
	MI.shutdown();

	return 0;
}