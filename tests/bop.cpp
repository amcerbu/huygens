#include "../src/audio.h"
#include "../src/subtractive.h"
#include "../src/noise.h"
#include "../src/metro.h"
#include "../src/buffer.h"
#include "../src/midi.h"
#include <signal.h>

using namespace std;
using namespace soundmath;

#define BSIZE 128

static bool running = true;
static bool ready = false;
void interrupt(int ignore)
{
	ready = false;
	cout << " [keyboard interrupt, exiting ...]" << endl;
	running = false;
}


// Subtractive synthesizer parameters
Metro<double> physics(SR / 2);
Subtractive<double> sub(7, 7, 0.7, 0.999995, 1);
Noise<double> excitation;
double the_sample;


// Sustain pedal parameters
Filter<double> smoother({0.01},{0,-0.99}); // low-pass filter
Buffer<double> sixty;

double value = 0;
double old = 0;
double coordinate = 0;

double sust_thresh = 0;
double rel_thresh = 0;
double width = 0;

double tolerance = 0.1;

bool sustained = false;
bool changed = false;
vector<unsigned char> sustain({{176, 64, 127}});
vector<unsigned char> release({{176, 64, 0}});

bool verbose = false;
bool pedaling = false;

inline int process(const float* in, float* out)
{
	if (ready)
	{
		for (int i = 0; i < BSIZE; i++)
		{
			// synthesizer updates
			the_sample = sub(excitation());
			out[i] = the_sample;

			if (physics())
				sub.physics();

			sub.tick();
			physics.tick();
			excitation.tick();
		

			sixty.write(in[i]);
			value = smoother(sixty(0) * sixty(0) + sixty(SR / 240) * sixty(SR / 240));

			// pedal updates
			double coordinate = (value - sust_thresh) / width;
			if (coordinate > 1)
			{
				if (sustained)
				{
					if (verbose)
						cout << "\rreleased...  " << value << flush;
					changed = true;
				}
				sustained = false;
			}
			else if (coordinate < 0)
			{
				if (!sustained)
				{
					if (verbose)
						cout << "\rsustained... " << value << flush;
					changed = true;
				}
				sustained = true;
			}

			smoother.tick();
			sixty.tick();
		}
	}
	else
	{
		for (int i = 0; i < BSIZE; i++)
		{
			sixty.write(in[i]);
			value = smoother(sixty(0) * sixty(0) + sixty(SR / 240) * sixty(SR / 240));

			smoother.tick();
			sixty.tick();
		}
	}


	return 0;
}


void setup_audio()
{
	sixty.initialize(SR / 60);	
}


void process_midi_in(MidiIn* device)
{
	int pitch, velocity;
	device->get();

	if (device->bytes)
	{
		switch ((Status)(device->message[0]))
		{
			case note_on:
			case note_off:
				pitch = (int)device->message[1];
				velocity = (int)device->message[2];
				if (velocity && (Status)device->message[0] != note_off)
					sub.makenote(pitch, dbtoa(-8 * (1 - (double)velocity / 127)));
				else
					sub.endnote(pitch); 

				break;

			default:
				break;
		}
	}
}

void process_midi_out(MidiOut* device)
{
	device->send(sustained ? &sustain : &release);
	changed = false;
}


void calibrate()
{
	// set up- and down-threshholds
	double sust_val, rel_val;

	cout << "\ndepress pedal ... press return to set threshold. ";
	cin.ignore();
	sust_val = value;
	cout <<   "        value  =  " << sust_val << endl;

	cout << "\nrelease pedal ... press return to set threshold. ";
	cin.ignore();
	rel_val = value;
	cout <<   "        value  =  " << rel_val << endl << endl;

	sust_thresh = sust_val * (1 - tolerance) + rel_val * tolerance;
	rel_thresh = rel_val * (1 - tolerance) + sust_val * tolerance;
	width = rel_thresh - sust_thresh;
}


Audio A = Audio(process, BSIZE);
MidiIn MI = MidiIn();
MidiOut MO = MidiOut();

int main(int argc, char* argv[])
{
	pedaling = false;
	for (int i = 0; i < argc; i++)
	{
		if (string(argv[i]) == "-p")
			pedaling = true;
	}

	// bind keyboard interrupt to program exit
	signal(SIGINT, interrupt);

	MI.startup(); // startup midi in
	MI.ignore(true, true, true); // ignore sysex, time, but not aftertouch
	MI.getports(false); // get ports but don't print anything
	MI.open("IAC Driver Bus 1");

	MO.startup();
	MO.getports(false);
	MO.open("IAC Driver Bus 2");

	setup_audio();

	A.startup(1, 1, true); // startup audio engine; 1 input, 1 output, console output on

	if (pedaling)
		calibrate();
	ready = true;

	while (running)
	{
		process_midi_in(&MI);

		if (changed && pedaling)
			process_midi_out(&MO);

		Pa_Sleep(1);
	}

	A.shutdown(); // shutdown audio engine
	MI.shutdown();
	MO.shutdown();

	return 0;
}