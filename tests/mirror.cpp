#include "../src/audio.h"
#include "../src/synth.h"
#include "../src/filter.h"
#include <signal.h>
#include <curses.h>
#include <map>

using namespace std;
using namespace soundmath;

#define BSIZE 2

static Synth<double> reference = Synth<double>(&cycle, 0);
static double amp = 0;
static double frequency = 0;

static Filter<double> smoother({0.01, 0}, {0,-0.99});

static bool running = true;
void interrupt(int ignore)
{
	amp = 0;
	cout << " [keyboard interrupt, exiting ...]" << endl;
	running = false;
}

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		out[2 * i] = limiter(1 * (in[i] + 0.1 * smoother(amp) * reference()));
		out[2 * i + 1] = limiter(1 * (in[i] + 0.1 * smoother(amp) * reference()));

		amp *= 0.9999;

		reference.freqmod(frequency);
		reference.tick();
		smoother.tick();
	}

	return 0;
}

static Audio A = Audio(process, BSIZE);

std::map<char, int> keymapping = {{'z', 52}, {'x', 53}, {'c', 54}, {'v', 55}, {'b', 56}, {'n', 57}, {'m', 58}, {',', 59}, {'.', 60}, {'/', 61},
								  {'a', 57}, {'s', 58}, {'d', 59}, {'f', 60}, {'g', 61}, {'h', 62}, {'j', 63}, {'k', 64}, {'l', 65}, {';', 66}, {'\'', 67},
								  {'q', 62}, {'w', 63}, {'e', 64}, {'r', 65}, {'t', 66}, {'y', 67}, {'u', 68}, {'i', 69}, {'o', 70}, {'p', 71}, {'[', 72}, {']', 73}, {'\\', 74},
								  {'1', 67}, {'2', 68}, {'3', 69}, {'4', 70}, {'5', 71}, {'6', 72}, {'7', 73}, {'8', 74}, {'9', 75}, {'0', 76}, {'-', 77}, {'=', 78}};

int main()
{
	initscr();
	timeout(-1);

	A.startup(1, 2, true); // startup audio engine; 1 input, 2 outputs, console output on

	char input;

	while (running)
	{
		input = getch();
		switch (input)
		{
			case ' ':
				amp = 1 - amp;
				break;

			case 27:
				interrupt(0);
				Pa_Sleep(500);
				break;

			default:
				break;
		}

		if (keymapping.find(input) != keymapping.end())
		{
			amp = 1;
			frequency = mtof(keymapping[input]);
		}

		Pa_Sleep(1);
	}

	A.shutdown(); // shutdown audio engine
	endwin();

	return 0;
}