#include <iostream>

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Network.hpp>

#include "../../src/audio.h"
#include "../../src/oscbank.h"
#include "../../src/mixer.h"
#include "../../src/synth.h"

#define CHANS 2
#define INPUT 1

#define WIDTH 1600
#define HEIGHT 1600


#define DARKNESS 255
#define ALPHA 64
#define LINEALPHA 96
#define LIGHTNESS 0

#define BSIZE 64
#define FRAMERATE 60
#define INCREMENT 1

#define CHROMATIC 12
#define OCTAVES 2
// #define HARMONICS 4
#define A0 21

using namespace soundmath;

const int waveSize = (SR / INCREMENT) / FRAMERATE;
sf::Vertex waveforms[2 * waveSize * CHROMATIC * OCTAVES];
int waveOrigin = 0;
double gain = 3.0;
double radius = 0.75; // 
double ratio = 0.5; // 
double offset = 0.0;

bool ready = false;
bool flipped = false;

int pitch = 36;

Oscbank<double, CHROMATIC * OCTAVES> oscbank;

double xs[CHROMATIC * OCTAVES];
double ys[CHROMATIC * OCTAVES];
double rs[CHROMATIC * OCTAVES];

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i += INCREMENT)
	{
		out[i] = 0;
		double the_sample = in[CHANS * i + INPUT];

		for (int j = 0; j < CHROMATIC * OCTAVES; j++)
		{
			std::complex<double> point = (*oscbank())(j) * (offset + the_sample);
			double x = ((xs[j] + 1 + rs[j] * gain * point.real()) / 2) * WIDTH; 
			double y = ((ys[j] + 1 - rs[j] * gain * point.imag()) / 2) * HEIGHT;

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
	}

	return 0;
}

void coefficients(int base)
{
	for (int j = 0; j < CHROMATIC * OCTAVES; j++)
	{
		oscbank.freqmod(j, mtof(base + j));
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
		rs[j] = (radius * pow(ratio, j / CHROMATIC)); // logarithmic spiral
		xs[j] = rs[j] * sin((2 * PI * j * 1) / CHROMATIC);
		ys[j] = rs[j] * -cos((2 * PI * j * 1) / CHROMATIC);
	}
	oscbank.open();

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