#include "../src/audio.h"
#include "../src/filter.h"
#include "../src/buffer.h"
#include "../src/midi.h"

using namespace std;
using namespace soundmath;

#define BSIZE 64

static bool running = true;
bool ready = false;
void interrupt(int ignore)
{
	ready = false;
	cout << "\n [keyboard interrupt, exiting ...]" << endl;
	running = false;
}

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

inline int process(const float* in, float* out)
{
	if (ready)
	{
		for (int i = 0; i < BSIZE; i++)
		{
			sixty.write(in[i]);
			value = smoother(sixty(0) * sixty(0) + sixty(SR / 240) * sixty(SR / 240));

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

void process_midi(MidiOut* device)
{
	// if (sustained)
	// 	device->send(&sustain);
	// else
	// 	device->send(&release);
	device->send(sustained ? &sustain : &release);

	changed = false;
}


Audio A = Audio(process, BSIZE);
MidiOut MO = MidiOut();

int main()
{
	// bind keyboard interrupt to program exit
	signal(SIGINT, interrupt);

	MO.startup();
	MO.getports(false);
	MO.open("IAC Driver Bus 2");

	sixty.initialize(SR / 60);

	A.startup(1, 1, true); // startup audio engine; 1 input, 1 output, console output on

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

	// cout << "\nsustain thresh is " << sust_thresh << "; release thresh is " << rel_thresh << endl << endl;
	ready = true;

	while (running)
	{
		if (changed)
			process_midi(&MO);

		Pa_Sleep(10);
	}

	A.shutdown(); // shutdown audio engine

	MO.send(&release);
	MO.shutdown();

	return 0;
}