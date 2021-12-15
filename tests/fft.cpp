#include "../src/audio.h"
#include "../src/fourier.h"
#include "../src/wave.h"
#include "../src/filter.h"
#include "../src/synth.h"
#include <signal.h>

using namespace std;
using namespace soundmath;

#define BSIZE 16
#define N 32768

static bool running = true;
void interrupt(int ignore)
{
	cout << " [keyboard interrupt, exiting ...]" << endl;
	std::fclose(stdin);
    std::cin.setstate(std::ios::badbit); // To differenciate from EOF input
	running = false;
}

double *dct_in, *dct_out;
Cosine dct(N, &dct_in, &dct_out);
int write_origin = 0;

double frequency;
double smoothed;
// Filter<double> smoother({0.5, 0}, {0,-0.5});
Filter<double> smoother({1, 0}, {0,0});

Synth<double> resynth(&cycle, 0);
Filter<double> smooooother({0.001, 0}, {0, -0.999});

inline double ramp(double phase, double slope = 5)
{
	return slope * phase < 0.5 ? slope * phase : ((1 - phase) * slope < 0.5 ? 1 - (1 - phase) * slope : 0.5);
}

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		if (write_origin < N)
		{
			dct_in[write_origin] = hann(ramp(write_origin / (double)N)) * in[i];
			write_origin++;
		}
		else
		{
			dct.forward();
			write_origin = 0;

			double biggest = 0;

			for (int j = 0; j < N; j++)
			{
				static double norm;
				norm = dct_out[j] * dct_out[j];

				if (norm > biggest)
				{
					biggest = norm;
					double angular = j / (2.0 * N);

					frequency = SR * angular;
				}
			}

			smoothed = smoother(frequency);
			smoother.tick();

			double note = ftom(smoothed);
			int center = int(note + 0.5);
			double fraction = note - center;

			cout << notename(center) << (fraction > 0 ? " + " : " - ") << abs(fraction) * 100 << " c" << endl;
			// cout << frequency << endl;
		}

		out[2 * i] = resynth() / 2;
		out[2 * i + 1] = resynth() / 2;

		resynth.freqmod(smooooother(smoothed));
		resynth.tick();
		smooooother.tick();

	}

	return 0;
}

Audio A = Audio(process, BSIZE);

int main()
{	
	A.startup(1, 2, true); // startup audio engine; 1 input, 2 outputs, console output on

	while (running)
	{
		Pa_Sleep(10);
	}

	A.shutdown(); // shutdown audio engine

	return 0;
}