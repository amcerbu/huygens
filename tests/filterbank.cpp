#include "../src/audio.h"
#include "../src/noise.h"
#include "../src/filterbank.h"
#include "../src/wave.h"
#include "../src/delay.h"
#include "../src/midi.h"
#include "../src/rms.h"

using namespace soundmath;

using namespace std;
using namespace soundmath;

#define BSIZE 128

static bool running = true;
void interrupt(int ignore)
{
	cout << "\n [keyboard interrupt, exiting ...]" << endl;
	running = false;
}

// compute transfer function of our bandpass filter with params freq, r at incoming frequency f
std::complex<double> transfer(double frequency, double r, double f)
{
	std::complex<double> z(cos(2 * PI * f / SR), -sin(2 * PI * f / SR));
	return (1.0 - z * z) / (1.0 - 2 * r * cos(2 * PI * frequency / SR) * z + r * r * z * z);
}

double softclip(double sample, double width)
{
	if (abs(sample) < width)
		return sample;

	int sign = sgn(sample);
	double gap = sample - sign * width;
	return sign * width + (1 - width) * 2.0 / PI * atan(PI * gap / (2 * (1 - width)));
}

double softclip(double sample)
{
	return softclip(sample, 0.125);
}

double saturate(double sample)
{
	return 2.0 / PI * atan(2 * PI * sample / 2.0);
}

const int courses = 9; // "strings" per notes coursed,
const int octaves = 8; // number of octaves
const double division = 12; // edo
// const double detune = 0.125; // tuning margin of error (fraction of edo)
const double detune = 0.125;
const int N = division * octaves * courses; // lots of filters!
double frequency = 27.5; // A0
int A0 = 21;
double tail = 100;
double r = relaxation(tail);

double rolloff = 1;
double gains[N];
double amplitudes[N];

Filterbank<double> F(2, N, 0.001, 1);

Noise<double> noise;
RMS<double> rms;
// double air = 0;
double noisefloor = 0;
double air = 0;
// double airmod = 0.125;
double airmod = 0;
double dry = 0;
double distorted = 0.1;
double drive = 5;
double follow = 0;

double the_sample = 0;

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		the_sample = softclip(dry * in[i] +
							 	((1 - follow) + follow * rms(in[i])) * (1 + rms(in[i]) * airmod * noise()) * 
							 		F(softclip(noisefloor * noise() + drive * in[i] + rms(in[i]) * air * noise()), &softclip), 0.9);

		// the_sample = in[i] - (softclip(1000 * (the_sample - in[i]))) * (the_sample - in[i]);
		// the_sample = softclip(F(noise(), &softclip));
		out[i] = the_sample;
		F.tick();
		rms.tick();
	}

	return 0;
}

void process_midi(MidiIn* device)
{
	int pitch, velocity, index;
	device->get();

	if (device->bytes)
	{
		switch ((Status)(device->message[0]))
		{
			case note_on:
			case note_off:
				pitch = (int)device->message[1];
				velocity = (int)device->message[2];
				index = pitch - A0;
				if (index >= 0 and index < N)
				{
					if (velocity && (Status)device->message[0] != note_off)
						for (int j = 0; j < courses; j++)
						{
							F.boost(courses * index + j, 1);
							F.mix(courses * index + j, gains[courses * index + j]);
						}
					else
						for (int j = 0; j < courses; j++)
						{
							F.boost(courses * index + j, 0);
							F.mix(courses * index + j, 0);
						}
				}
				break;

			default:
				break;
		}
	}
}


void coefficients()
{
	for (int i = 0; i < division * octaves; i++)
	{
		for (int j = 0; j < courses; j++)
		{
			double partial = frequency * pow(2, (i + detune * pow((2 * (j + 0.5) / (double)courses - 1), 1)) / division);

			double angle = partial * (2 * PI / SR);
			gains[courses * i + j] = 1.0 / abs(transfer(partial, r, partial));
			F.coefficients(courses * i + j, {1.0, 0, -1.0}, {-2 * r * cos(angle), r * r});
		}
	}

	F.mix(std::vector<double>(gains, gains + N));
	F.boost(std::vector<double>(N, 0));
}



Audio A = Audio(process, BSIZE);
MidiIn MI = MidiIn();

int main()
{
	// bind keyboard interrupt to program exit
	signal(SIGINT, interrupt);

	MI.startup(); // startup midi in
	MI.ignore(true, true, true); // ignore sysex, time, but not aftertouch
	MI.getports(false); // get ports but don't print anything
	MI.open("IAC Driver Bus 1");

	coefficients();
	memset(amplitudes, 0, sizeof(double) * N);
	A.startup(1, 1, true); // startup audio engine; 1 input, 1 output, console output on

	while (running)
	{
		process_midi(&MI);

		Pa_Sleep(5);
	}

	A.shutdown(); // shutdown audio engine
	MI.shutdown();


	return 0;
}
