#include "../src/audio.h"
#include "../src/fourier.h"
#include "../src/wave.h"
#include "../src/filter.h"
#include "../src/delay.h"
#include "../src/synth.h"
#include <signal.h>

using namespace std;
using namespace soundmath;

#define BSIZE 2048
// #define N 16384
// #define N 2048
// #define N 4096
// #define N 65536
#define N 262144
// #define LAPS 32
#define LAPS 16

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
		if (norm(in[i]) < 10 * average * average)
		{
			// out[i] = in[i] / 20.0;
			out[i] = 0;
		}

		else
		{
			out[i] = in[i];
		}
	}

	return 0;
}

double the_ratio = 0.5;

Fourier F1 = Fourier(f_process, N, LAPS);
Fourier F2 = Fourier(f_process, N, LAPS);

Delay<double> delay1({1, N});
Delay<double> delay2({1, N});

Delay<double> mixie1({1, N});
Delay<double> mixie2({1, N});

double sample1 = 0;
double sample2 = 0;
double feedback = 0;

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		F1.write(in[2 * i] + in[2 * i + 1] + feedback * mixie1(sample1));
		F2.write(in[2 * i + 1] + in[2 * i] + feedback * mixie2(sample2));
		mixie1.tick();
		mixie2.tick();

		double real, imag;
		F1.read(&real, &imag);
		sample1 = out[2 * i] = limiter(the_ratio * real); // + delay1(in[2 * i]));

		F2.read(&real, &imag);
		sample2 = out[2 * i + 1] = limiter(the_ratio * real); // + delay2(in[2 * i + 1]));;
	}

	return 0;
}

Audio A = Audio(process, BSIZE);

int main()
{
	delay1.coefficients({{8 * N, 1}}, {});
	delay2.coefficients({{8 * N, 1}}, {});
	A.startup(2, 2, true); // startup audio engine; 2 input, 2 outputs, console output on

	while (running)
		Pa_Sleep(10);

	A.shutdown(); // shutdown audio engine

	return 0;
}