#include <iostream>

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Network.hpp>

#include "../../src/audio.h"
#include "../../src/oscbank.h"
#include "../../src/slidebank.h"
#include "../../src/modbank.h"
#include "../../src/mixer.h"

#define CHANS 1
#define INPUT 0

#define WIDTH 1600
#define HEIGHT 1600


#define DARKNESS 255
#define ALPHA 64
#define LINEALPHA 127
#define LIGHTNESS 0

#define BSIZE 64
#define FRAMERATE 60
#define INCREMENT 1

#define CHROMATIC 12
#define OCTAVES 3
// #define HARMONICS 4
#define A0 21

using namespace soundmath;

const int waveSize = (SR / INCREMENT) / FRAMERATE;
sf::Vertex waveforms[2 * waveSize * CHROMATIC * OCTAVES];
int waveOrigin = 0;
double gain = 5.0;
double radius = 0.9; // 
double ratio = 0.75; // 
double offset = 0.0;

bool ready = false;
bool flipped = false;

int pitch = 24;

// double filter_r = 0.999;
double filter_r = 0.996;
int multiplicity = 3;
Oscbank<double, CHROMATIC * OCTAVES> oscbank;
Slidebank<double, CHROMATIC * OCTAVES> slidebank;
Modbank<double, CHROMATIC * OCTAVES> modulators; // modulation

double xs[CHROMATIC * OCTAVES];
double ys[CHROMATIC * OCTAVES];
double rs[CHROMATIC * OCTAVES];

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i += INCREMENT)
	{
		out[i] = 0;
		double the_sample = in[CHANS * i + INPUT];
		// double sign = (the_sample > 0 ? 1 : (the_sample < 0 ? -1 : 0));
		// double normalized = the_sample * sign;

		// slidebank(modulators(offset + sign * sqrt(normalized), oscbank())); // for shapes!
		slidebank(modulators(offset + the_sample, oscbank()));
		for (int j = 0; j < CHROMATIC * OCTAVES; j++)
		{
			// std::complex<double> point = (*slidebank())(j) * (*oscbank())(j); // gives circles
			std::complex<double> point = (*slidebank())(j) * (1.0 + (*oscbank())(j)); // gives spinning circles
			// std::complex<double> point = (*slidebank())(j); // gives stationary points
			// std::complex<double> point = (*slidebank())(j) * (*oscbank())(j) * sqrt(normalized); // gives stationary points
			// double x = ((xs[j] + 1 + rs[j] * gain * point.real()) / 2) * WIDTH; 
			double x = ((xs[j] + 1 + gain * point.real()) / 2) * WIDTH; 
			// double y = ((ys[j] + 1 - rs[j] * gain * point.imag()) / 2) * HEIGHT;
			double y = ((ys[j] + 1 - gain * point.imag()) / 2) * HEIGHT;

			waveforms[2 * waveSize * j + waveSize * flipped + waveOrigin] = 
				sf::Vertex(sf::Vector2f(x, y), sf::Color(LIGHTNESS, LIGHTNESS, LIGHTNESS, LINEALPHA));
		}

		waveOrigin--;
		if (waveOrigin < 0)
		{
			ready = true;
			flipped = !flipped;
			waveOrigin += waveSize;
		}

		oscbank.tick();
		slidebank.tick();
	}

	return 0;
}

void coefficients(int base)
{
	for (int j = 0; j < CHROMATIC * OCTAVES; j++)
	{
		oscbank.freqmod(j, 55 * pow(2, ((double)(base + j - A0)) / CHROMATIC));
	}
}

Audio A = Audio(process, BSIZE);

int main()
{
	sf::ContextSettings settings;
	settings.antialiasingLevel = 3;

	sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "Spiral", sf::Style::Titlebar | sf::Style::Close, settings);
	// window.setFramerateLimit(FRAMERATE);
	// window.clear(sf::Color::White);

	sf::VertexArray triangle(sf::Quads, 4);

	// define the position of the triangle's points
	triangle[0].position = sf::Vector2f(0, 0);
	triangle[2].position = sf::Vector2f(WIDTH, HEIGHT);
	triangle[1].position = sf::Vector2f(0, HEIGHT);
	triangle[3].position = sf::Vector2f(WIDTH, 0);

	// define the color of the triangle's points
	triangle[0].color = sf::Color(DARKNESS, DARKNESS, DARKNESS, ALPHA); // sf::Color(DARKNESS, 0, 0, ALPHA); //::Red; 
	triangle[1].color = sf::Color(DARKNESS, DARKNESS, DARKNESS, ALPHA); // sf::Color(0, 0, DARKNESS, ALPHA); //::Bue; 
	triangle[2].color = sf::Color(DARKNESS, DARKNESS, DARKNESS, ALPHA); // sf::Color(0, DARKNESS, 0, ALPHA); //::Green; 
	triangle[3].color = sf::Color(DARKNESS, DARKNESS, DARKNESS, ALPHA); // sf::Color(DARKNESS, DARKNESS, 0, ALPHA); //::Yellow; 


	for (int j = 0; j < CHROMATIC * OCTAVES; j++)
	{
		// rs[j] = (radius * pow(ratio, (double)j / CHROMATIC)); // logarithmic spiral
		// rs[j] = (radius * pow(ratio, j / CHROMATIC)); // logarithmic spiral
		rs[j] = radius * (1 - 1.0 / (OCTAVES) * ((double)j / CHROMATIC));
		xs[j] = rs[j] * sin((2 * PI * j * 1) / CHROMATIC);
		ys[j] = rs[j] * -cos((2 * PI * j * 1) / CHROMATIC);
	}

	slidebank.setup(multiplicity, std::vector<std::complex<double>>(CHROMATIC * OCTAVES, filter_r));
	oscbank.open();
	slidebank.open();

	coefficients(pitch);
	A.startup(CHANS, 1, true); // startup audio engine; CHANS inputs, 1 outputs, console output on

	while (window.isOpen())
	{
		// event polling
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			else if (event.type == sf::Event::KeyPressed)
			{
				if (event.key.code == sf::Keyboard::Escape)
					window.close();

				else if (event.key.code == sf::Keyboard::Left)
				{
					pitch -= 12;
					coefficients(pitch);
					// osc.freqmod(mtof(pitch));
				}
				else if (event.key.code == sf::Keyboard::Right)
				{
					pitch += 12;
					coefficients(pitch);
					// osc.freqmod(mtof(pitch));
				}
				else if (event.key.code == sf::Keyboard::Down)
				{
					gain -= 0.05;
				}
				else if (event.key.code == sf::Keyboard::Up) 
				{
					gain += 0.05;
				}
			}
		}
		
		
		window.draw(triangle);
		
		while (!ready)
		{
			sf::sleep(sf::microseconds(10));
		}

		for (int j = 0; j < CHROMATIC * OCTAVES; j++)
		{
			window.draw(waveforms + 2 * waveSize * j + waveSize * (!flipped), waveSize, sf::LineStrip);
		}

		ready = false;

		window.display(); // window is done drawing
	}

	A.shutdown(); // shutdown audio engine

	return 0;
}