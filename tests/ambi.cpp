#include "../src/audio.h"
#include "../src/oscillator.h"
#include "../src/synth.h"
#include "../src/delay.h"
#include "../src/metro.h"
#include "../src/filter.h"
#include "../src/noise.h"
#include <signal.h>

using namespace soundmath;
#define BSIZE 32
#define DISTANCE 10
#define RADIUS 5
#define DIFFUSION 1.0
#define SPEED 0.01

static bool running = true;
void interrupt(int ignore)
{
	std::cout << " [keyboard interrupt, exiting ...]" << std::endl;
	std::fclose(stdin);
    std::cin.setstate(std::ios::badbit); // To differenciate from EOF input
	running = false;
}

typedef struct
{
	double x;
	double y;
	double z;
} Point;

double sqdist(Point& a, Point& b)
{
	double dx, dy, dz;
	dx = a.x - b.x;
	dy = a.y - b.y;
	dz = a.z - b.z;

	return dx * dx + dy * dy + dz * dz;
}

Oscillator<double> lfo(0.1);
Metro<double>** metros;
Filter<double>** smoothers;
Filter<double> damper({0.05},{0,-0.95});
Noise<double> noise;

Synth<double>** waves;
Delay<double>** lefts;
Delay<double>** rights;

Point* positions;
Point* ears;

double gain = 1;

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		positions[0] = {DISTANCE, RADIUS * cos(2 * PI * lfo()), RADIUS * sin(2 * PI * lfo())};
		positions[1] = {-DISTANCE, RADIUS * cos(-2 * PI * lfo()), RADIUS * sin(-2 * PI * lfo())};
		positions[2] = {RADIUS * sin(2 * PI * lfo()), DISTANCE, RADIUS * cos(2 * PI * lfo())};
		positions[3] = {RADIUS * sin(-2 * PI * lfo()), -DISTANCE, RADIUS * cos(-2 * PI * lfo())};
		positions[4] = {RADIUS * cos(2 * PI * lfo()), RADIUS * sin(2 * PI * lfo()), DISTANCE};
		positions[5] = {RADIUS * cos(-2 * PI * lfo()), RADIUS * sin(-2 * PI * lfo()), -DISTANCE};

		for (int i = 0; i < 6; i++)
		{
			double distance = sqdist(ears[0], positions[i]);
			lefts[i]->coefficients({{sqrt(distance) / SPEED, DIFFUSION / distance}}, {});

			distance = sqdist(ears[1], positions[i]);
			rights[i]->coefficients({{sqrt(distance) / SPEED, DIFFUSION / distance}}, {});
		}

		double l_sample = 0;
		double r_sample = 0;

		for (int i = 0; i < 6; i++)
		{
			l_sample += (*lefts[i])(  (*waves[i])()  + 8 * (damper(noise())) * (*smoothers[i])((*metros[i])()));	
			r_sample += (*rights[i])(  (*waves[i])()  + 8 * (damper(noise())) * (*smoothers[i])((*metros[i])()));

			// if ((*metros[i])())
			// 	std::cout << i << ": fired" << std::endl;
		}
		out[2 * i] = l_sample * gain;
		out[2 * i + 1] = r_sample * gain;

		for (int i = 0; i < 6; i++)
		{
			waves[i]->tick();
			lefts[i]->tick();
			rights[i]->tick();
			metros[i]->tick();
			smoothers[i]->tick();
		}

		lfo.tick();
		// metro.tick();
		// smoother.tick();
		damper.tick();
	}

	return 0;
}


Audio A = Audio(process, BSIZE);

int main()
{
	waves = new Synth<double>*[6];
	lefts = new Delay<double>*[6];
	rights = new Delay<double>*[6];
	metros = new Metro<double>*[6];
	smoothers = new Filter<double>*[6];
	positions = new Point[6];
	ears = new Point[2];

	ears[0] = {1,1,1};
	ears[1] = {-1,-1,-1};

	positions[0] = {DISTANCE, RADIUS * cos(2 * PI * lfo()), RADIUS * sin(2 * PI * lfo())};
	positions[1] = {-DISTANCE, RADIUS * cos(2 * PI * lfo()), RADIUS * sin(2 * PI * lfo())};
	positions[2] = {RADIUS * sin(2 * PI * lfo()), DISTANCE, RADIUS * cos(2 * PI * lfo())};
	positions[3] = {RADIUS * sin(2 * PI * lfo()), -DISTANCE, RADIUS * cos(2 * PI * lfo())};
	positions[4] = {RADIUS * cos(2 * PI * lfo()), RADIUS * sin(2 * PI * lfo()), DISTANCE};
	positions[5] = {RADIUS * cos(2 * PI * lfo()), RADIUS * sin(2 * PI * lfo()), -DISTANCE};

	for (int i = 0; i < 6; i++)
	{
		waves[i] = new Synth<double>(&triangle, 220 + 55 * i);
		lefts[i] = new Delay<double>(1, SR); // 1-second maximum delay times
		rights[i] = new Delay<double>(1, SR); // 1-second maximum delay times
		metros[i] = new Metro<double>(3, (double)i / 6.0);
		smoothers[i] = new Filter<double>({1},{0,-0.999});

		double distance = sqdist(ears[0], positions[i]);
		lefts[i]->coefficients({{sqrt(distance) / SPEED, DIFFUSION / distance}}, {});

		distance = sqdist(ears[1], positions[i]);
		rights[i]->coefficients({{sqrt(distance) / SPEED, DIFFUSION / distance}}, {});
	}

	A.startup(2, 2, true); // startup audio engine; 1 input, 2 outputs, console output on

	while (running)
	{
		Pa_Sleep(100);
	}

	A.shutdown(); // shutdown audio engine

	for (int i = 0; i < 6; i++)
	{
		delete waves[i];
		delete lefts[i];
		delete rights[i];
		delete metros[i];
		delete smoothers[i];
	}
	delete [] waves;
	delete [] lefts;
	delete [] rights;
	delete [] metros;
	delete [] smoothers;
	delete [] positions;
	delete [] ears;

	return 0;
}