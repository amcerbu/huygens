#include "../src/audio.h"
#include "../src/noise.h"
#include "../src/filterbank.h"
#include "../src/wave.h"
#include "../src/delay.h"
#include "../src/midi.h"
#include "../src/rms.h"
#include "../src/argparse.h"

using namespace soundmath;

#define BSIZE 64

// parameters configurable with command-line args
int CHANELS; // number of parallel chanels to run
int INPUT_OFFSET; // self-explanatory
int OUTPUT_OFFSET; // ""
int INDEVICE; // input device
int OUTDEVICE; // ""

double tail;
double r;

double noisefloor;
double air;
double dry;
double drive;

// obsolete parameters
double follow = 0;
double airmod = 0;

void args(int argc, char *argv[])
{
	argparse::ArgumentParser program("filterbank");

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

	program.add_argument("-f", "--frames")
		.default_value<int>(1)
		.required()
		.scan<'i', int>()
		.help("channels per input / output frame");

	program.add_argument("-ic", "--inchan")
		.default_value<int>(0)
		.required()
		.scan<'i', int>()
		.help("input channel offset");

	program.add_argument("-oc", "--outchan")
		.default_value<int>(1)
		.required()
		.scan<'i', int>()
		.help("output channel offset");

	program.add_argument("-d", "--devices")
		.help("list audio device names")
		.default_value(false)
		.implicit_value(true);


	program.add_argument("--tail")
		.default_value<double>(50)
		.required()
		.scan<'f', double>()
		.help("decay time of filter impulse responses");

	program.add_argument("--floor")
		.default_value<double>(0)
		.required()
		.scan<'f', double>()
		.help("\"noise floor\" of filterbank");

	program.add_argument("--air")
		.default_value<double>(5)
		.required()
		.scan<'f', double>()
		.help("amount of noisy excitation using rms of input signal");

	program.add_argument("--dry")
		.default_value<double>(0)
		.required()
		.scan<'f', double>()
		.help("amount of dry signal");

	program.add_argument("--drive")
		.default_value<double>(5)
		.required()
		.scan<'f', double>()
		.help("boost to dry signal before filtering");

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
	CHANELS = program.get<int>("-f");
	INPUT_OFFSET = program.get<int>("-ic");
	OUTPUT_OFFSET = program.get<int>("-oc");

	Audio::initialize(!(program.is_used("-i") && program.is_used("-o") && program.is_used("-f") && program.is_used("-ic") && program.is_used("-oc")) || program.is_used("-d"));


	tail = program.get<double>("--tail");
	r = relaxation(tail);

	noisefloor = program.get<double>("--floor");
	air = program.get<double>("--air");
	dry = program.get<double>("--dry");
	drive = program.get<double>("--drive");
}

static bool running = true;
void interrupt(int ignore)
{
	std::cout << "\n [keyboard interrupt, exiting ...]" << std::endl;
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


// signal parameters
const int courses = 9; // "strings" per notes coursed,
const int octaves = 8; // number of octaves
const int division = 12; // edo
const double detune = 0.125; // tuning margin of error (fraction of edo)
const int N = division * octaves * courses; // lots of filters!


double gains[N];
double amplitudes[N];

double frequency = 27.5; // A0
int A0 = 21;

FFilterbank<double, N, 2>** Fs; // arrays of filterbanks
RMS<double>** rmss; // and rms signals
Noise<double> noise;

double out_sample = 0;

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		for (int j = 0; j < CHANELS; j++)
		{
			double in_sample = in[(CHANELS + INPUT_OFFSET) * i + INPUT_OFFSET + j];
			FFilterbank<double, N, 2>* F = Fs[j];
			RMS<double>* rms = rmss[j];

			out_sample = softclip(dry * in_sample +
								 	((1 - follow) + follow * (*rms)(in_sample)) * (1 + (*rms)(in_sample) * airmod * noise()) * 
								 		(*F)(softclip(noisefloor * noise() + drive * in_sample + (*rms)(in_sample) * air * noise()), &softclip), 0.9);

			out[(CHANELS + OUTPUT_OFFSET) * i + OUTPUT_OFFSET + j] = out_sample;
			F->tick();
			rms->tick();
		}
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

				for (int k = 0; k < CHANELS; k++)
				{
					FFilterbank<double, N, 2>* F = Fs[k];
					if (index >= 0 and index < N)
					{
						if (velocity && (Status)device->message[0] != note_off)
							for (int j = 0; j < courses; j++)
							{
								F->boost(courses * index + j, 1);
								F->mix(courses * index + j, gains[courses * index + j]);
							}
						else
							for (int j = 0; j < courses; j++)
							{
								F->boost(courses * index + j, 0);
								F->mix(courses * index + j, 0);
							}
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
	for (int k = 0; k < CHANELS; k++)
	{
		FFilterbank<double, N, 2>* F = Fs[k];
		for (int i = 0; i < division * octaves; i++)
		{
			for (int j = 0; j < courses; j++)
			{
				double partial = frequency * pow(2, (i + detune * pow((2 * (j + 0.5) / (double)courses - 1), 1)) / division);

				double angle = partial * (2 * PI / SR);
				gains[courses * i + j] = 1.0 / abs(transfer(partial, r, partial));
				F->coefficients(courses * i + j, {1.0, 0, -1.0}, {-2 * r * cos(angle), r * r});
			}
		}

		F->mix(std::vector<double>(gains, gains + N));
		F->boost(std::vector<double>(N, 0));
	}
}



Audio A = Audio(process, BSIZE);
MidiIn MI = MidiIn();

int main(int argc, char *argv[])
{
	args(argc, argv);

	Fs = new FFilterbank<double, N, 2>*[CHANELS];
	rmss = new RMS<double>*[CHANELS];

	for (int j = 0; j < CHANELS; j++)
	{
		Fs[j] = new FFilterbank<double, N, 2>(0.001, 1);
		rmss[j] = new RMS<double>;
	}

	// bind keyboard interrupt to program exit
	signal(SIGINT, interrupt);

	MI.startup(); // startup midi in
	MI.ignore(true, true, true); // ignore sysex, time, but not aftertouch
	MI.getports(false); // get ports but don't print anything
	MI.open("IAC Driver Bus 1");

	coefficients();
	memset(amplitudes, 0, sizeof(double) * N);
	A.startup(CHANELS + INPUT_OFFSET, CHANELS + OUTPUT_OFFSET, true, INDEVICE, OUTDEVICE); // startup audio engine; 4 inputs, 1 outputs, console output on

	while (running)
	{
		process_midi(&MI);

		Pa_Sleep(5);
	}

	A.shutdown(); // shutdown audio engine
	MI.shutdown();

	for (int j = 0; j < CHANELS; j++)
	{
		delete Fs[j];
		delete rmss[j];
	}

	delete [] Fs;
	delete [] rmss;



	return 0;
}
