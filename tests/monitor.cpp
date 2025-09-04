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
int INCHANS;
int OUTCHANS;
int INPUT;
int OUTPUT;
int INDEVICE;
int OUTDEVICE;


void args(int argc, char *argv[])
{
	argparse::ArgumentParser program("monitor");

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

	program.add_argument("-if", "--framesin")
		.default_value<int>(1)
		.required()
		.scan<'i', int>()
		.help("channels per input frame");

	program.add_argument("-ic", "--inchannel")
		.default_value<int>(0)
		.required()
		.scan<'i', int>()
		.help("input channel");

	program.add_argument("-of", "--oframesut")
		.default_value<int>(1)
		.required()
		.scan<'i', int>()
		.help("channels per output frame");

	program.add_argument("-oc", "--outchannel")
		.default_value<int>(0)
		.required()
		.scan<'i', int>()
		.help("output channel");

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
	INCHANS = program.get<int>("-if");
	INPUT = program.get<int>("-ic");
	OUTCHANS = program.get<int>("-of");
	OUTPUT = program.get<int>("-oc");

	Audio::initialize(!(program.is_used("-i") && program.is_used("-o") && program.is_used("-if") && program.is_used("-ic")) || program.is_used("-d"));
}

static bool running = true;
void interrupt(int ignore)
{
	std::cout << "\n [keyboard interrupt, exiting ...]" << std::endl;
	running = false;
}

double out_sample = 0;

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		out[OUTCHANS * i + OUTPUT] = in[INCHANS * i + INPUT];
	}

	return 0;
}


Audio A = Audio(process, BSIZE);

int main(int argc, char *argv[])
{
	args(argc, argv);

	// bind keyboard interrupt to program exit
	signal(SIGINT, interrupt);

	A.startup(INCHANS, OUTCHANS, true, INDEVICE, OUTDEVICE); // startup audio engine; 4 inputs, 1 outputs, console output on
	while (running)
	{
		Pa_Sleep(5);
	}

	A.shutdown(); // shutdown audio engine

	return 0;
}
